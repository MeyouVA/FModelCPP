#pragma once
// Ported from FModel/Helper.cs — the two static helpers the loading path needs.
//
// Only FixKey and GetGameName are ported: they are what the AES manager and the directory selector call.
// The rest of the C# file is the WPF window registry (OpenWindow / GetWindow / IsWindowOpen /
// CloseWindow<T>), which has no counterpart here — MenuCommand's openWindowHandler seam covers the one
// behaviour the port needed from it — plus two trig helpers used by the unported 3D viewer.

#include <QString>

namespace FModel::Helper
{
    // Normalises a user-typed AES key to "0x" + uppercase hex.
    //
    // Two upstream quirks are preserved:
    //   * The length guard is `keySpan.Length > sizeof(char) * (2 + 32)`. `sizeof(char)` is 2 in C#, so the
    //     limit is 68 characters, not the 66 a 256-bit key with its "0x" prefix actually occupies. A 67- or
    //     68-character key therefore passes and comes back longer than any key can be, rather than being
    //     rejected as "bullshit key". The comparison is reproduced exactly.
    //   * The prefix is written as lowercase 'x' after the body has been uppercased, so a key that arrived
    //     as "0X..." comes back as "0x...".
    inline QString fixKey(const QString& key)
    {
        if (key.isEmpty())
            return QString();

        const QString trimmed = key.trimmed();
        if (trimmed.size() > 2 * (2 /* 0x */ + 32 /* FAES = 256 bit */))
            return QString(); // bullshit key

        QString body = trimmed.toUpper();
        if (body.startsWith(QStringLiteral("0X")))
            body = body.mid(2);

        return QStringLiteral("0x") + body;
    }

    // Given a Paks directory, walks up to the project folder:
    //   install_folder/GameName/Content/Paks  ->  "GameName"
    // Anything else keeps its own leaf name. The "Binaries" sibling check is what distinguishes a real
    // project layout from a directory that merely happens to be called Paks.
    QString getGameName(const QString& path);
}
