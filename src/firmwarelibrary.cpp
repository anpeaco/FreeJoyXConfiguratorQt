#include "firmwarelibrary.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QDateTime>

namespace {

const QStringList kTrackedRepos = {
    QStringLiteral("FreeJoy-Team/FreeJoy"),
    QStringLiteral("anpeaco/FreeJoyX"),
};

QString releasesUrlFor(const QString &repoSlug)
{
    return QStringLiteral("https://api.github.com/repos/%1/releases").arg(repoSlug);
}

constexpr const char *kCacheFileName = ".firmwarelibrary.json";

} // namespace


FirmwareLibrary::FirmwareLibrary(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    /* Load any cached data immediately so the dropdown can populate
     * without waiting for the network. fetchReleases() then refreshes
     * in the background. */
    loadCache();
}

QString FirmwareLibrary::recoveryDir() const
{
    return QCoreApplication::applicationDirPath() + "/recovery";
}

QString FirmwareLibrary::firmwareDir() const
{
    return QCoreApplication::applicationDirPath() + "/firmware";
}

QString FirmwareLibrary::cacheFilePath() const
{
    return recoveryDir() + "/" + QString::fromLatin1(kCacheFileName);
}

QString FirmwareLibrary::safeFilename(const QString &repoSlug,
                                      const QString &tag,
                                      const QString &assetName)
{
    /* Encode "owner/repo" into a single underscore-joined segment so the
     * filename is filesystem-safe and uniquely identifies the source.
     * e.g. "FreeJoy-Team_FreeJoy-v1.7.3b0-FreeJoy.bin" */
    QString safeRepo = repoSlug;
    safeRepo.replace('/', '_');
    return QStringLiteral("%1-%2-%3").arg(safeRepo, tag, assetName);
}

QString FirmwareLibrary::localPathFor(const Release &release, const Asset &asset) const
{
    return recoveryDir() + "/" + safeFilename(release.repo, release.tag, asset.name);
}


/* ============================================================================
 * Releases fetch
 * ============================================================================
 */

void FirmwareLibrary::fetchReleases()
{
    /* If a fetch is already running, ignore. The user-visible refresh
     * action can call fetchReleases() repeatedly; this guards against
     * double-counting m_pendingReleaseFetches. */
    if (m_pendingReleaseFetches > 0) {
        qDebug() << "FirmwareLibrary: fetch already in progress, skipping";
        return;
    }

    /* Reset the in-memory release list so we don't have stale entries
     * mixed with fresh ones. The cache on disk stays as-is until both
     * fetches finish; if both fail we keep the cached data via
     * loadCache() at startup. */
    m_releases.clear();
    m_pendingReleaseFetches = kTrackedRepos.size();
    for (const QString &repo : kTrackedRepos) {
        fetchOneRepo(repo);
    }
}

void FirmwareLibrary::fetchOneRepo(const QString &repoSlug)
{
    QNetworkRequest req;
    req.setUrl(QUrl(releasesUrlFor(repoSlug)));
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("FreeJoyXConfigurator/1.0"));
    /* GitHub recommends an explicit Accept header to pin the API
     * version. v3 is current as of 2025. */
    req.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply *reply = m_nam->get(req);
    /* Tag the reply with the repo slug so onReleasesReplyFinished knows
     * which one came back. */
    reply->setProperty("repoSlug", repoSlug);
    connect(reply, &QNetworkReply::finished,
            this, &FirmwareLibrary::onReleasesReplyFinished);
}

void FirmwareLibrary::onReleasesReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QString repoSlug = reply->property("repoSlug").toString();
    --m_pendingReleaseFetches;

    if (reply->error() != QNetworkReply::NoError) {
        const QString msg = QStringLiteral("Failed to fetch %1 releases: %2")
                                .arg(repoSlug, reply->errorString());
        qWarning() << "FirmwareLibrary:" << msg;
        emit releasesError(msg);
    } else {
        const QByteArray body = reply->readAll();
        parseReleasesJson(repoSlug, body);
    }

    if (m_pendingReleaseFetches <= 0) {
        m_pendingReleaseFetches = 0;
        sortReleases();
        saveCache();
        emit releasesUpdated();
    }
}

void FirmwareLibrary::parseReleasesJson(const QString &repoSlug,
                                        const QByteArray &json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        emit releasesError(QStringLiteral("JSON parse error for %1: %2")
                               .arg(repoSlug, err.errorString()));
        return;
    }
    if (!doc.isArray()) {
        emit releasesError(QStringLiteral("Unexpected JSON shape for %1").arg(repoSlug));
        return;
    }

    int kept = 0;
    int skipped = 0;
    for (const QJsonValue &v : doc.array()) {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();

        const QString tag = obj.value("tag_name").toString();
        if (!tagIsCompatible(tag)) {
            ++skipped;
            continue;
        }

        Release rel;
        rel.repo = repoSlug;
        rel.tag = tag;
        rel.publishedAt = obj.value("published_at").toString();

        const QJsonArray assetsArr = obj.value("assets").toArray();
        for (const QJsonValue &av : assetsArr) {
            if (!av.isObject()) continue;
            const QJsonObject ao = av.toObject();
            const QString name = ao.value("name").toString();
            if (!name.endsWith(".bin", Qt::CaseInsensitive)) continue;

            Asset a;
            a.name = name;
            a.downloadUrl = ao.value("browser_download_url").toString();
            a.size = static_cast<qint64>(ao.value("size").toDouble());
            rel.assets.append(a);
        }

        if (rel.assets.isEmpty()) {
            /* Releases with no .bin asset are useless to us (e.g. older
             * tags that only published source archives). Skip silently. */
            ++skipped;
            continue;
        }

        m_releases.append(rel);
        ++kept;
    }

    qDebug() << "FirmwareLibrary: fetched" << repoSlug
             << "->" << kept << "kept," << skipped << "skipped";
}

bool FirmwareLibrary::tagIsCompatible(const QString &tag) const
{
    /* Accept v<major>.<minor>.<patch> with optional b<build> suffix.
     * Filter to >= v1.7.0 per the user's "anything after v1.7.0"
     * requirement. */
    QRegularExpression re(QStringLiteral("^v(\\d+)\\.(\\d+)\\.(\\d+)(?:b\\d+)?$"));
    const QRegularExpressionMatch m = re.match(tag);
    if (!m.hasMatch()) return false;

    const int major = m.captured(1).toInt();
    const int minor = m.captured(2).toInt();
    /* patch unused in the comparison since we accept v1.7.0 as the floor */

    if (major > 1) return true;
    if (major < 1) return false;
    return minor >= 7;
}

void FirmwareLibrary::sortReleases()
{
    /* Sort by repo first (so the FreeJoyX entries cluster together),
     * then by tag in descending order so newest releases bubble to the
     * top of the dropdown. Tag comparison uses major/minor/patch ints
     * to handle "v1.10" beating "v1.9" correctly even though we don't
     * have those tags today. */
    std::sort(m_releases.begin(), m_releases.end(),
        [](const Release &a, const Release &b) {
            if (a.repo != b.repo) return a.repo < b.repo;
            QRegularExpression re(QStringLiteral("^v(\\d+)\\.(\\d+)\\.(\\d+)(?:b(\\d+))?$"));
            const auto ma = re.match(a.tag);
            const auto mb = re.match(b.tag);
            if (!ma.hasMatch() || !mb.hasMatch()) return a.tag > b.tag;
            for (int i = 1; i <= 4; ++i) {
                const int ai = ma.captured(i).toInt();
                const int bi = mb.captured(i).toInt();
                if (ai != bi) return ai > bi;
            }
            return false;
        });
}


/* ============================================================================
 * Cache persistence
 * ============================================================================
 */

void FirmwareLibrary::saveCache()
{
    QDir().mkpath(recoveryDir());

    QJsonArray releasesArr;
    for (const Release &r : m_releases) {
        QJsonObject ro;
        ro["repo"] = r.repo;
        ro["tag"] = r.tag;
        ro["publishedAt"] = r.publishedAt;

        QJsonArray assetsArr;
        for (const Asset &a : r.assets) {
            QJsonObject ao;
            ao["name"] = a.name;
            ao["downloadUrl"] = a.downloadUrl;
            ao["size"] = double(a.size);
            assetsArr.append(ao);
        }
        ro["assets"] = assetsArr;
        releasesArr.append(ro);
    }

    QJsonObject root;
    root["lastUpdated"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["releases"] = releasesArr;

    QFile f(cacheFilePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    } else {
        qWarning() << "FirmwareLibrary: couldn't write cache to" << cacheFilePath();
    }
}

void FirmwareLibrary::loadCache()
{
    QFile f(cacheFilePath());
    if (!f.exists()) return;
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "FirmwareLibrary: couldn't read cache from" << cacheFilePath();
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "FirmwareLibrary: cache JSON parse error" << err.errorString();
        return;
    }
    if (!doc.isObject()) return;

    const QJsonArray releasesArr = doc.object().value("releases").toArray();
    for (const QJsonValue &rv : releasesArr) {
        if (!rv.isObject()) continue;
        const QJsonObject ro = rv.toObject();

        Release rel;
        rel.repo = ro.value("repo").toString();
        rel.tag = ro.value("tag").toString();
        rel.publishedAt = ro.value("publishedAt").toString();

        const QJsonArray assetsArr = ro.value("assets").toArray();
        for (const QJsonValue &av : assetsArr) {
            if (!av.isObject()) continue;
            const QJsonObject ao = av.toObject();
            Asset a;
            a.name = ao.value("name").toString();
            a.downloadUrl = ao.value("downloadUrl").toString();
            a.size = static_cast<qint64>(ao.value("size").toDouble());
            rel.assets.append(a);
        }
        if (!rel.assets.isEmpty()) {
            m_releases.append(rel);
        }
    }
    sortReleases();
    qDebug() << "FirmwareLibrary: loaded" << m_releases.size() << "releases from cache";
}


/* ============================================================================
 * Asset download
 * ============================================================================
 */

QString FirmwareLibrary::downloadAsset(const Release &release, const Asset &asset)
{
    const QString destPath = localPathFor(release, asset);

    /* If the file already exists with the expected size, treat as cached
     * and emit assetDownloaded immediately. The 0 == size check is for
     * old cache entries from before we tracked sizes. */
    QFile dest(destPath);
    if (dest.exists() && (asset.size == 0 || dest.size() == asset.size)) {
        qDebug() << "FirmwareLibrary: asset already cached at" << destPath;
        QMetaObject::invokeMethod(this, "assetDownloaded", Qt::QueuedConnection,
                                  Q_ARG(QString, destPath),
                                  Q_ARG(bool, true));
        return destPath;
    }

    QDir().mkpath(recoveryDir());

    QNetworkRequest req;
    req.setUrl(QUrl(asset.downloadUrl));
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("FreeJoyXConfigurator/1.0"));
    /* GitHub redirects asset URLs to S3 -- tell the NAM to follow
     * automatically rather than us reissuing the request. */
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(req);
    reply->setProperty("destPath", destPath);
    reply->setProperty("assetName", asset.name);
    connect(reply, &QNetworkReply::finished,
            this, &FirmwareLibrary::onDownloadReplyFinished);
    connect(reply, &QNetworkReply::downloadProgress,
            this, &FirmwareLibrary::onDownloadProgress);

    return destPath;
}

void FirmwareLibrary::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    const QString assetName = reply->property("assetName").toString();
    emit assetDownloadProgress(assetName, bytesReceived, bytesTotal);
}

void FirmwareLibrary::onDownloadReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    const QString destPath = reply->property("destPath").toString();
    const QString assetName = reply->property("assetName").toString();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "FirmwareLibrary: download failed for" << assetName
                   << reply->errorString();
        emit assetDownloaded(destPath, false);
        return;
    }

    QFile dest(destPath);
    if (!dest.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "FirmwareLibrary: can't open destination" << destPath;
        emit assetDownloaded(destPath, false);
        return;
    }
    dest.write(reply->readAll());
    dest.close();

    qDebug() << "FirmwareLibrary: downloaded" << assetName << "to" << destPath
             << "(" << dest.size() << "bytes )";
    emit assetDownloaded(destPath, true);
}
