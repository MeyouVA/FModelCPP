// Ported from FModel/ViewModels/Commands/CopyCommand.cs
#include "CopyCommand.h"

#include <QClipboard>
#include <QGuiApplication>

#include "FileProvider/Objects/GameFile.h"

namespace FModel::ViewModels::Commands
{
    using CUE4Parse::FileProvider::Objects::GameFile;

    namespace
    {
        // C#'s `new StringBuilder().AppendLine(x)`: Environment.NewLine on Windows.
        void appendLine(QString& sb, const std::string& value)
        {
            sb += QString::fromStdString(value);
            sb += QStringLiteral("\r\n");
        }

        // C#'s string.TrimEnd(): trailing whitespace only, so the last AppendLine's newline goes away.
        QString trimEnd(QString s)
        {
            while (!s.isEmpty() && s.back().isSpace())
                s.chop(1);
            return s;
        }
    }

    QString CopyCommand::buildText(const QString& trigger, const QList<GameFile*>& entries)
    {
        QString sb;
        if (trigger == QStringLiteral("File_Path"))
        {
            for (const auto* entry : entries) appendLine(sb, entry->Path());
        }
        else if (trigger == QStringLiteral("File_Name"))
        {
            for (const auto* entry : entries) appendLine(sb, entry->Name());
        }
        else if (trigger == QStringLiteral("Directory_Path"))
        {
            for (const auto* entry : entries) appendLine(sb, entry->Directory());
        }
        else if (trigger == QStringLiteral("File_Path_No_Extension"))
        {
            for (const auto* entry : entries) appendLine(sb, entry->PathWithoutExtension());
        }
        else if (trigger == QStringLiteral("File_Name_No_Extension"))
        {
            for (const auto* entry : entries) appendLine(sb, entry->NameWithoutExtension());
        }

        return trimEnd(sb);
    }

    void CopyCommand::execute(ApplicationViewModel* /*contextViewModel*/, const QVariant& parameter)
    {
        // C#: `if (parameter is not object[] parameters || parameters[0] is not string trigger) return;`
        const QVariantList parameters = parameter.toList();
        if (parameters.size() < 2 || parameters[0].typeId() != QMetaType::QString)
            return;

        const QString trigger = parameters[0].toString();

        // C#: `(parameters[1] as IEnumerable)?.OfType<object>().SelectMany(...)` — anything in the selection
        // that is not an asset is dropped rather than failing the whole copy.
        QList<GameFile*> entries;
        for (const QVariant& item : parameters[1].toList())
        {
            if (auto* gf = item.value<GameFile*>())
                entries.append(gf);
        }

        if (entries.isEmpty())
            return;

        QGuiApplication::clipboard()->setText(buildText(trigger, entries));
    }
}
