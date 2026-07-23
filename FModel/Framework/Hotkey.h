#pragma once
// Ported from FModel/Framework/Hotkey.cs.
//
// IsTriggered's C# body reads the live Keyboard.Modifiers static, which is a WPF input-manager query with no
// Qt equivalent that makes sense outside an event handler. The port takes the modifiers as a parameter instead
// (the caller has them on the QKeyEvent), keeping the same HasFlag semantics.

#include "ViewModel.h"
#include "InputKeys.h"

class QJsonObject;

namespace FModel::Framework
{
    class Hotkey : public ViewModel
    {
        Q_OBJECT

    public:
        explicit Hotkey(Key key, ModifierKeys modifiers = ModifierKeys::None, QObject* parent = nullptr)
            : ViewModel(parent), _key(key), _modifiers(modifiers) {}

        Key key() const { return _key; }
        void setKey(Key value) { setProperty(_key, value, QStringLiteral("Key")); }

        ModifierKeys modifiers() const { return _modifiers; }
        void setModifiers(ModifierKeys value) { setProperty(_modifiers, value, QStringLiteral("Modifiers")); }

        // C#: IsTriggered(Key e) => e == Key && Keyboard.Modifiers.HasFlag(Modifiers);
        bool isTriggered(Key e, ModifierKeys activeModifiers) const
        {
            return e == _key && hasFlag(activeModifiers, _modifiers);
        }

        QString toString() const;

        QJsonObject toJson() const;
        void readJson(const QJsonObject& json);

    private:
        Key _key;
        ModifierKeys _modifiers;
    };
}
