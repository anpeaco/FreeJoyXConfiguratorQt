/**
  ******************************************************************************
  * @file           : firmwareimage.h
  * @brief          : Parser for FreeJoy firmware .bin files. Loads the binary
  *                   into memory and extracts identifying metadata (board,
  *                   firmware version, build id) from the embedded ID footer
  *                   placed by FreeJoyX's image_id.c (issue
  *                   anpeaco/FreeJoyX#23).
  *
  *                   Falls back to a vector-table heuristic on binaries built
  *                   before the footer convention landed -- reset-handler
  *                   addresses live in different ranges for F103 vs F411 app
  *                   regions and identify the target board reliably.
  *
  *                   Sliced as anpeaco/FreeJoyXConfiguratorQt#15. Pure
  *                   plumbing -- consumed by the flasher rework's confirmation
  *                   dialog (#18) and binary-info card (#17). No UI here.
  ******************************************************************************
  */

#ifndef FIRMWAREIMAGE_H
#define FIRMWAREIMAGE_H

#include <QByteArray>
#include <QString>
#include <cstdint>

class FirmwareImage
{
public:
    enum class Board {
        Unknown = 0,
        F103BluePill = 1,    /* mirrors BOARD_ID_F103_BLUEPILL */
        F411BlackPill = 2,   /* mirrors BOARD_ID_F411_BLACKPILL */
    };

    enum class Source {
        None,                /* nothing loaded */
        Footer,              /* metadata read from the embedded ID footer */
        VectorTableHeuristic /* footer absent; reset-handler address inspected */
    };

    FirmwareImage();

    /* Load a .bin file into memory and parse its metadata. Returns true on
     * successful read; false leaves the object empty. A successful read does
     * not guarantee identified metadata -- check source() for whether the
     * footer was found or the heuristic ran. */
    bool loadFromFile(const QString &path);

    /* Clear all state. After clear() the object behaves as a freshly
     * constructed instance. */
    void clear();

    bool isLoaded() const { return !m_bytes.isEmpty(); }
    Source source() const { return m_source; }
    Board board() const { return m_board; }
    uint16_t fwVersion() const { return m_fwVersion; }   /* 0 if unknown */
    uint16_t buildId() const { return m_buildId; }       /* 0 if unknown */
    qint64 size() const { return m_bytes.size(); }
    const QString &path() const { return m_path; }
    const QString &fileName() const { return m_fileName; }
    const QByteArray &bytes() const { return m_bytes; }

    /* Human-readable label for the board ("F103 BluePill", "F411 BlackPill",
     * "Unknown"). Suitable for the confirmation dialog. */
    QString boardName() const;

    /* "v0xNNNN" or "v0xNNNN (build N)" when the build id is known. Empty
     * string when no version was identified. */
    QString versionLabel() const;

private:
    /* Footer magic 'FRJY' little-endian -- matches FREEJOY_IMAGE_MAGIC in
     * FreeJoyX/application/Inc/image_id.h. */
    static constexpr uint32_t kFooterMagic = 0x46524A59u;

    /* Wire-stable footer shape. Mirrors freejoy_image_id_t on the firmware
     * side. struct_size lets the parser tolerate future field additions:
     * we read the first kFooterMinSize bytes and ignore any trailing fields
     * we don't know about. The firmware-side copy is the source of truth;
     * this parser refuses to decode anything claiming a smaller struct_size
     * than what we expect today. */
    static constexpr int kFooterMinSize = 28;

    bool tryParseFooter();
    bool tryHeuristic();

    QString m_path;
    QString m_fileName;
    QByteArray m_bytes;
    Source m_source = Source::None;
    Board m_board = Board::Unknown;
    uint16_t m_fwVersion = 0;
    uint16_t m_buildId = 0;
    /* Human semver from the footer (0.0.0 = pre-semver binary; versionLabel()
     * then falls back to the wire-format fw_version). */
    uint8_t  m_verMajor = 0;
    uint8_t  m_verMinor = 0;
    uint8_t  m_verPatch = 0;
};

#endif /* FIRMWAREIMAGE_H */
