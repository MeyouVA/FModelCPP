// Ported from FModel/Constants.cs
#include "Constants.h"

#include <QCoreApplication>
#include <QFileInfo>

namespace FModel::Constants
{
    QString APP_PATH()
    {
        // C#: Path.GetFullPath(Environment.GetCommandLineArgs()[0]).
        return QCoreApplication::applicationFilePath();
    }

    QString APP_VERSION()
    {
#ifdef FMODEL_APP_VERSION
        return QStringLiteral(FMODEL_APP_VERSION);
#else
        return QStringLiteral("0.0.0.0");
#endif
    }

    QString APP_COMMIT_ID()
    {
#ifdef FMODEL_APP_COMMIT_ID
        return QStringLiteral(FMODEL_APP_COMMIT_ID);
#else
        // C# reads this out of ProductVersion's "+<sha>" suffix; with nothing injected there is no honest
        // answer, so say so rather than fabricate a sha.
        return QStringLiteral("unknown");
#endif
    }

    QString APP_SHORT_COMMIT_ID()
    {
        return APP_COMMIT_ID().left(7); // C#: APP_COMMIT_ID[..7]
    }

    QDateTime APP_BUILD_DATE()
    {
        // C#: File.GetLastWriteTime(APP_PATH).
        return QFileInfo(APP_PATH()).lastModified();
    }

    const QList<QVector3D>& COLOR_PALETTE()
    {
        static const QList<QVector3D> palette{
            {0.231f, 0.231f, 0.231f}, // Dark gray
            {0.376f, 0.490f, 0.545f}, // Teal
            {0.957f, 0.263f, 0.212f}, // Red
            {0.196f, 0.804f, 0.196f}, // Green
            {0.957f, 0.647f, 0.212f}, // Orange
            {0.612f, 0.153f, 0.690f}, // Purple
            {0.129f, 0.588f, 0.953f}, // Blue
            {1.000f, 0.920f, 0.424f}, // Yellow
            {0.824f, 0.412f, 0.118f}, // Brown
            {0.612f, 0.800f, 0.922f}  // Light blue
        };
        return palette;
    }

    int PALETTE_LENGTH() { return static_cast<int>(COLOR_PALETTE().size()); }
}
