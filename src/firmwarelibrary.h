/**
  ******************************************************************************
  * @file           : firmwarelibrary.h
  * @brief          : Auto-fetch + cache of FreeJoy / FreeJoyX firmware
  *                   releases from GitHub for the Flasher tab dropdown.
  ******************************************************************************
  *
  * Two repos are tracked:
  *   - FreeJoy-Team/FreeJoy   (upstream)
  *   - anpeaco/FreeJoyX       (fork; this project's own builds)
  *
  * Releases are filtered to versions >= v1.7.0 (per the user's "anything
  * after v1.7.0 should be supported" requirement). The migrator currently
  * handles v1.7.0 and v1.7.1 cleanly; v1.7.3+ snapshots are pending but
  * the user can flash any release in the list -- migration only matters
  * if they want to import the *config* before flashing.
  *
  * Fetch strategy:
  *   - On configurator startup, kick off a background fetch of
  *     /repos/.../releases for both repos.
  *   - Cache the parsed results in <recovery>/.firmwarelibrary.json so
  *     subsequent runs see the dropdown populated even before the
  *     network call completes.
  *   - User can trigger a manual refresh from the Flasher tab at any time.
  *
  * Download strategy:
  *   - Asset .bin files download on-demand only -- the dropdown shows
  *     metadata (repo, tag, asset name, size) but doesn't pull bytes
  *     until the user picks a non-cached entry and clicks Flash. After
  *     download lands in <recovery>/, subsequent flashes use the cached
  *     copy instantly.
  ******************************************************************************
  */

#ifndef FIRMWARELIBRARY_H
#define FIRMWARELIBRARY_H

#include <QObject>
#include <QString>
#include <QList>

class QNetworkAccessManager;
class QNetworkReply;

class FirmwareLibrary : public QObject
{
    Q_OBJECT

public:
    struct Asset {
        QString name;          // e.g. "FreeJoy.bin"
        QString downloadUrl;   // direct URL to the .bin
        qint64 size = 0;       // bytes
    };

    struct Release {
        QString repo;          // e.g. "FreeJoy-Team/FreeJoy"
        QString tag;           // e.g. "v1.7.3b0"
        QString publishedAt;   // ISO 8601 string
        QList<Asset> assets;   // .bin assets only -- non-bin assets filtered out
    };

    explicit FirmwareLibrary(QObject *parent = nullptr);

    /* Triggers async fetch of /releases from both tracked repos.
     * releasesUpdated() fires when both fetches finish (success or
     * error). releasesError(msg) fires once per repo-fetch failure.
     * Cache is written on success so subsequent runs have data. */
    void fetchReleases();

    /* Last-known release list (from network or cache). Sorted by repo
     * then tag (newest first). */
    QList<Release> releases() const { return m_releases; }

    /* Async download of a remote asset to the local recovery folder.
     * destPath is computed from the asset metadata (repo + tag + name)
     * so the same release+asset always lands at the same path. Returns
     * the path that the file will eventually live at; emits
     * assetDownloaded(path, success) when the download finishes. */
    QString downloadAsset(const Release &release, const Asset &asset);

    /* Predicted local path for a given release/asset pair. Useful for
     * checking if a download has already happened (file exists on
     * disk). */
    QString localPathFor(const Release &release, const Asset &asset) const;

    /* Recovery folder + cache file paths. Public so the Flasher widget
     * can use the same recovery folder for its "Browse" file picker
     * default and for its "Open recovery folder" action. */
    QString recoveryDir() const;
    QString cacheFilePath() const;

    /* Firmware folder: <exe>/firmware/. Distinct from recovery/ so the
     * user's fresh build outputs don't mix with stable fallbacks.
     * Convention:
     *   recovery/ = known-good builds (downloads + safety nets)
     *   firmware/ = your dev builds (drop FreeJoy.bin from
     *               armgcc/build/f103/app/ here for one-click flashing) */
    QString firmwareDir() const;

signals:
    /* Both repos finished fetching (success or error). The Flasher
     * widget rebuilds the Source dropdown when this fires. */
    void releasesUpdated();

    /* Per-repo fetch failure. msg is human-readable (network error,
     * HTTP status, JSON parse error). */
    void releasesError(const QString &message);

    /* Per-asset download progress. */
    void assetDownloadProgress(const QString &assetName,
                               qint64 bytesReceived,
                               qint64 bytesTotal);

    /* Per-asset download terminal state. localPath == the file the
     * caller passed to downloadAsset(); success == true means it lives
     * at that path now, false means the download failed and the file
     * is absent or partial. */
    void assetDownloaded(const QString &localPath, bool success);

private slots:
    void onReleasesReplyFinished();
    void onDownloadReplyFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QNetworkAccessManager *m_nam;
    QList<Release> m_releases;
    int m_pendingReleaseFetches = 0;

    void fetchOneRepo(const QString &repoSlug);
    void parseReleasesJson(const QString &repoSlug, const QByteArray &json);
    bool tagIsCompatible(const QString &repoSlug, const QString &tag) const;
    void saveCache();
    void loadCache();
    void sortReleases();

    static QString safeFilename(const QString &repoSlug,
                                const QString &tag,
                                const QString &assetName);
};

#endif // FIRMWARELIBRARY_H
