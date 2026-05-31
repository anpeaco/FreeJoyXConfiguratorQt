/**
  ******************************************************************************
  * @file           : firmwareimage.cpp
  * @brief          : See firmwareimage.h. Loads a .bin, scans for the
  *                   FREEJOY_IMAGE_MAGIC footer, falls back to vector-table
  *                   reset-handler inspection on legacy binaries.
  ******************************************************************************
  */

#include "firmwareimage.h"

#include <QFile>
#include <QFileInfo>
#include <cstring>

FirmwareImage::FirmwareImage() = default;

void FirmwareImage::clear()
{
    m_path.clear();
    m_fileName.clear();
    m_bytes.clear();
    m_source = Source::None;
    m_board = Board::Unknown;
    m_fwVersion = 0;
    m_buildId = 0;
    m_verMajor = 0;
    m_verMinor = 0;
    m_verPatch = 0;
}

bool FirmwareImage::loadFromFile(const QString &path)
{
    clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    m_bytes = f.readAll();
    f.close();
    if (m_bytes.isEmpty()) {
        clear();
        return false;
    }

    m_path = path;
    m_fileName = QFileInfo(path).fileName();

    /* Order matters: footer is authoritative when present, heuristic is a
     * compatibility shim for older shipped binaries that don't carry one. */
    if (!tryParseFooter()) {
        tryHeuristic();
    }
    return true;
}

bool FirmwareImage::tryParseFooter()
{
    /* Scan for the magic. We don't pin the footer to a known offset --
     * vector-table sizes differ between F103 and F411 startup files, and
     * keeping the footer placement loose lets the linker put it wherever
     * .rodata ends up. The whole .bin is <500 KB so a linear scan is
     * negligible. */
    const char *data = m_bytes.constData();
    const int len = m_bytes.size();

    /* Magic is 4 bytes, aligned to 4 by the linker. Walk in 4-byte
     * steps -- 4x fewer comparisons than byte-by-byte and we won't
     * miss anything the linker would emit. */
    for (int off = 0; off + kFooterMinSize <= len; off += 4) {
        uint32_t magic = 0;
        std::memcpy(&magic, data + off, sizeof(magic));
        if (magic != kFooterMagic) {
            continue;
        }
        /* Verify struct_size makes sense -- protects against a stray
         * literal `0x46524A59` that happens to appear in some other
         * .rodata blob. A real footer always reports a struct_size
         * >= our minimum, and never something absurdly large. */
        uint16_t structSize = 0;
        std::memcpy(&structSize, data + off + 4, sizeof(structSize));
        if (structSize < kFooterMinSize || structSize > 256) {
            continue;
        }
        /* Looks like a real footer. Decode. */
        uint16_t boardId = 0, fwVersion = 0, buildId = 0;
        std::memcpy(&boardId,   data + off + 6,  sizeof(boardId));
        std::memcpy(&fwVersion, data + off + 8,  sizeof(fwVersion));
        std::memcpy(&buildId,   data + off + 10, sizeof(buildId));

        switch (boardId) {
            case 1: m_board = Board::F103BluePill;  break;
            case 2: m_board = Board::F411BlackPill; break;
            default: m_board = Board::Unknown;      break;
        }
        m_fwVersion = fwVersion;
        m_buildId = buildId;
        /* Human semver at off+12..14 (added after the wire token; reclaimed
         * from the reserved tail, struct_size unchanged). Pre-semver footers
         * have zeros there -> treated as "not present" and we fall back to the
         * wire token in versionLabel(). */
        m_verMajor = static_cast<uint8_t>(data[off + 12]);
        m_verMinor = static_cast<uint8_t>(data[off + 13]);
        m_verPatch = static_cast<uint8_t>(data[off + 14]);
        m_source = Source::Footer;
        return true;
    }
    return false;
}

bool FirmwareImage::tryHeuristic()
{
    /* Vector table layout (Cortex-M):
     *   offset 0: initial stack pointer
     *   offset 4: Reset_Handler address
     * Reset_Handler always points into the application's flash region.
     * F103 app region: 0x08002000 - 0x0800FFFF
     * F411 app region: 0x08020000 - 0x0807FFFF
     * These don't overlap, so a single read identifies the target.
     *
     * Reset_Handler addresses always have the thumb-bit (LSB) set, so we
     * mask it off before comparing. */
    if (m_bytes.size() < 8) {
        return false;
    }
    uint32_t resetHandler = 0;
    std::memcpy(&resetHandler, m_bytes.constData() + 4, sizeof(resetHandler));
    resetHandler &= ~1u;

    if (resetHandler >= 0x08002000u && resetHandler < 0x08010000u) {
        m_board = Board::F103BluePill;
        m_source = Source::VectorTableHeuristic;
        return true;
    }
    if (resetHandler >= 0x08020000u && resetHandler < 0x08080000u) {
        m_board = Board::F411BlackPill;
        m_source = Source::VectorTableHeuristic;
        return true;
    }
    /* Outside known app regions -- could be a bootloader binary (which
     * targets 0x08000000 on both boards) or something unrelated. Leave
     * board unset; consumer can decide whether to refuse or accept. */
    m_source = Source::VectorTableHeuristic;
    return false;
}

QString FirmwareImage::boardName() const
{
    switch (m_board) {
        case Board::F103BluePill:  return QStringLiteral("F103 BluePill");
        case Board::F411BlackPill: return QStringLiteral("F411 BlackPill");
        case Board::Unknown:       break;
    }
    return QStringLiteral("Unknown");
}

QString FirmwareImage::versionLabel() const
{
    /* Prefer the human semver (footer >= the semver addition). All-zero means a
     * pre-semver binary -> fall back to the wire-format token below. */
    if (m_verMajor != 0 || m_verMinor != 0 || m_verPatch != 0) {
        QString v = QStringLiteral("%1.%2.%3")
            .arg(m_verMajor).arg(m_verMinor).arg(m_verPatch);
        if (m_buildId != 0) {
            v += QStringLiteral(" (b%1)").arg(m_buildId);
        }
        return v;
    }
    if (m_fwVersion == 0) {
        return QString();
    }
    if (m_buildId != 0) {
        return QStringLiteral("v0x%1 (build %2)")
            .arg(m_fwVersion, 4, 16, QChar('0'))
            .arg(m_buildId);
    }
    return QStringLiteral("v0x%1").arg(m_fwVersion, 4, 16, QChar('0'));
}
