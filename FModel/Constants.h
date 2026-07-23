#pragma once
// Ported from FModel/Constants.cs — the app-wide constants.
//
// Deliberate differences from C#:
//   * The four build-identity values (APP_PATH / APP_VERSION / APP_COMMIT_ID / APP_SHORT_COMMIT_ID) come from
//     the assembly's FileVersionInfo in C#. There is no portable C++ equivalent, so they are resolved from
//     QCoreApplication::applicationFilePath() plus compile-time definitions the build can inject
//     (FMODEL_APP_VERSION / FMODEL_APP_COMMIT_ID); without them they fall back to placeholders rather than
//     inventing a commit id. They are functions, not constants, because they need a live QCoreApplication.
//   * COLOR_PALETTE is a QList<QVector3D> instead of Vector3[]; PALETTE_LENGTH follows from it.
//   * The colour strings stay strings (WPF brush literals) — the app converts them at the point of use.

#include <QDateTime>
#include <QList>
#include <QString>
#include <QVector3D>

#include "UE4/Objects/Core/Misc/FGuid.h"

namespace FModel::Constants
{
    // --- build identity (see the header note) ---
    QString APP_PATH();
    QString APP_VERSION();
    QString APP_COMMIT_ID();
    QString APP_SHORT_COMMIT_ID();
    QDateTime APP_BUILD_DATE();

    inline const QString ZERO_64_CHAR =
        QStringLiteral("0000000000000000000000000000000000000000000000000000000000000000");
    inline const CUE4Parse::UE4::Objects::Core::Misc::FGuid ZERO_GUID{0u};

    inline constexpr float SCALE_DOWN_RATIO = 0.01f;
    inline constexpr int SAMPLES_COUNT = 4;

    inline const QString WHITE = QStringLiteral("#DAE5F2");
    inline const QString GRAY = QStringLiteral("#BBBBBB");
    inline const QString RED = QStringLiteral("#E06C75");
    inline const QString GREEN = QStringLiteral("#98C379");
    inline const QString YELLOW = QStringLiteral("#E5C07B");
    inline const QString BLUE = QStringLiteral("#528BCC");

    inline const QString ISSUE_LINK = QStringLiteral("https://github.com/4sval/FModel/discussions/categories/q-a");
    inline const QString GH_REPO = QStringLiteral("https://api.github.com/repos/4sval/FModel");
    inline const QString GH_COMMITS_HISTORY = GH_REPO + QStringLiteral("/commits");
    inline const QString GH_RELEASES = GH_REPO + QStringLiteral("/releases");
    inline const QString DONATE_LINK = QStringLiteral("https://fmodel.app/donate");
    inline const QString DISCORD_LINK = QStringLiteral("https://fmodel.app/discord");

    inline const QString _FN_LIVE_TRIGGER = QStringLiteral("fortnite-live.manifest");
    inline const QString _VAL_LIVE_TRIGGER = QStringLiteral("valorant-live.manifest");

    inline const QString _NO_PRESET_TRIGGER = QStringLiteral("Hand Made");

    // Common issues
    inline const QString MAPPING_ISSUE_LINK = QStringLiteral("https://github.com/4sval/FModel/discussions/418");
    inline const QString AUDIO_ISSUE_LINK = QStringLiteral("https://github.com/4sval/FModel/discussions/658");
    inline const QString RADA_ISSUE_LINK = QStringLiteral("https://github.com/4sval/FModel/discussions/422");
    inline const QString VERSION_ISSUE_LINK = QStringLiteral("https://github.com/4sval/FModel/discussions/425");

    const QList<QVector3D>& COLOR_PALETTE();
    int PALETTE_LENGTH();
}
