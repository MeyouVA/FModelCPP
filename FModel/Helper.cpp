// Ported from FModel/Helper.cs
#include "Helper.h"

#include <QDir>
#include <QFileInfo>

namespace FModel::Helper
{
    QString getGameName(const QString& path)
    {
        // install_folder/
        //     ├─ Engine/
        //     ├─ GameName/
        //     │  ├─ Binaries/
        //     │  ├─ Content/
        //     │  │  ├─ Paks/
        // our goal is to get the GameName folder
        QDir dir(path);
        const QString leaf = dir.dirName();

        // C# compares with StringComparison.InvariantCulture, i.e. case-SENSITIVE: a "paks" directory does
        // not take this branch.
        if (leaf == QStringLiteral("Paks"))
        {
            QDir content(dir);
            if (content.cdUp() && content.dirName() == QStringLiteral("Content"))
            {
                QDir project(content);
                if (project.cdUp() && project.exists(QStringLiteral("Binaries")))
                    return project.dirName();
            }
        }

        return leaf;
    }
}
