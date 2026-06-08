/**
  ******************************************************************************
  * @file           : deviceversion.cpp
  * @brief          : See deviceversion.h.
  ******************************************************************************
  */

#include "deviceversion.h"

#include <QObject>

#include "common_defines.h"           /* FIRMWARE_VERSION */
#include "legacy/legacy_migrator.h"   /* legacy::canMigrate / describeVersion */

QString deviceVersionDisplay(const params_report_t &pr)
{
    const uint16_t devVer = pr.firmware_version;
    const bool compatible =
        (devVer & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0);

    if (compatible) {
        /* reserved_layout is the per-build counter (FIRMWARE_BUILD_ID); "(bN)"
         * lets the user confirm which bin is actually flashed. */
        return QStringLiteral("FreeJoyX %1.%2.%3 (b%4)")
            .arg(pr.freejoyx_version_major)
            .arg(pr.freejoyx_version_minor)
            .arg(pr.freejoyx_version_patch)
            .arg(pr.reserved_layout);
    }
    if (legacy::canMigrate(devVer)) {
        return QStringLiteral("FreeJoy %1")
            .arg(QString::fromLatin1(legacy::describeVersion(devVer)));
    }
    const QString hex = QStringLiteral("0x") +
        QString::number(devVer, 16).toUpper().rightJustified(4, QChar('0'));
    return QObject::tr("Unknown (%1)").arg(hex);
}
