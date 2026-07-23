#include "Hotkey.h"

#include <QJsonObject>

namespace FModel::Framework
{
    QString Hotkey::toString() const
    {
        QString str;

        if (hasFlag(_modifiers, ModifierKeys::Control))
            str += QStringLiteral("Ctrl + ");
        if (hasFlag(_modifiers, ModifierKeys::Shift))
            str += QStringLiteral("Shift + ");
        if (hasFlag(_modifiers, ModifierKeys::Alt))
            str += QStringLiteral("Alt + ");
        if (hasFlag(_modifiers, ModifierKeys::Windows))
            str += QStringLiteral("Win + ");

        str += keyName(_key);
        return str;
    }

    // Newtonsoft serialises Hotkey by its public properties, as {"Key": <int>, "Modifiers": <int>}.
    QJsonObject Hotkey::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("Key")] = static_cast<int>(_key);
        json[QStringLiteral("Modifiers")] = static_cast<int>(_modifiers);
        return json;
    }

    void Hotkey::readJson(const QJsonObject& json)
    {
        if (json.contains(QStringLiteral("Key")))
            setKey(static_cast<Key>(json[QStringLiteral("Key")].toInt()));
        if (json.contains(QStringLiteral("Modifiers")))
            setModifiers(static_cast<ModifierKeys>(json[QStringLiteral("Modifiers")].toInt()));
    }
}
