// Ported from FModel/Views/Resources/Controls/DictionaryEditor.xaml.cs
#include "DictionaryEditor.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../Constants.h"

#include "UE4/Objects/Core/Misc/FGuid.h"

namespace FModel::Views::Resources::Controls
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using Settings::VersioningSettings;

    const QString DictionaryEditor::CustomVersionsTitle =
        QStringLiteral("Versioning Configuration (Custom Versions)");
    const QString DictionaryEditor::OptionsTitle = QStringLiteral("Versioning Configuration (Options)");
    const QString DictionaryEditor::MapStructTypesTitle =
        QStringLiteral("Versioning Configuration (MapStructTypes)");

    namespace
    {
        // Both directions go through VersioningSettings so the editor speaks the settings file's dialect —
        // in particular FGuid's "00000000-00000000-00000000-00000000" form. `member` is the key that
        // VersioningSettings::toJson uses for the collection being edited.
        QString toIndentedJson(const VersioningSettings& settings, const QString& member)
        {
            const QJsonValue value = settings.toJson()[member];
            const QJsonDocument document = value.isArray() ? QJsonDocument(value.toArray())
                                                           : QJsonDocument(value.toObject());
            return QString::fromUtf8(document.toJson(QJsonDocument::Indented)).trimmed();
        }

        // Parses `text` as the given member and hands back a VersioningSettings holding it. Returns false on
        // malformed JSON — C#'s DeserializeObject would throw, and OnClick catches.
        bool fromJson(const QString& text, const QString& member, VersioningSettings& out)
        {
            QJsonParseError error{};
            const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError)
                return false;

            QJsonObject wrapper;
            if (document.isArray())
                wrapper[member] = document.array();
            else if (document.isObject())
                wrapper[member] = document.object();
            else
                return false;

            out.readJson(wrapper);
            return true;
        }
    }

    DictionaryEditor::DictionaryEditor(const QString& title, QWidget* parent) : QDialog(parent)
    {
        _defaultCustomVersions = {FCustomVersion(FGuid(), 0)};
        _defaultOptions = {{QStringLiteral("key1"), true}, {QStringLiteral("key2"), false}};
        _defaultMapStructTypes = {{QStringLiteral("MapName"),
                                   MapStructType(QStringLiteral("KeyType"), QStringLiteral("ValueType"))}};

        setWindowTitle(title);
        resize(560, 420);

        auto* layout = new QVBoxLayout(this);

        _editor = new QPlainTextEdit;
        _editor->setObjectName(QStringLiteral("MyAvalonEditor"));
        _editor->setFont(QFont(QStringLiteral("Consolas"), 11));
        layout->addWidget(_editor, 1);

        auto* bottom = new QHBoxLayout;
        _heBrokeIt = new QLabel(QStringLiteral("IF YOU DON'T KNOW WHAT THIS DOES, DON'T TOUCH IT!"));
        _heBrokeIt->setObjectName(QStringLiteral("HeBrokeIt"));
        bottom->addWidget(_heBrokeIt, 1, Qt::AlignRight | Qt::AlignVCenter);

        auto* ok = new QPushButton(QStringLiteral("OK"));
        ok->setDefault(true);
        auto* reset = new QPushButton(QStringLiteral("Reset"));
        auto* cancel = new QPushButton(QStringLiteral("Cancel"));
        connect(ok, &QPushButton::clicked, this, &DictionaryEditor::onClick);
        connect(reset, &QPushButton::clicked, this, &DictionaryEditor::onReset);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        bottom->addWidget(ok);
        bottom->addWidget(reset);
        bottom->addWidget(cancel);
        layout->addLayout(bottom);
    }

    DictionaryEditor::DictionaryEditor(const QList<FCustomVersion>& customVersions, const QString& title,
                                       QWidget* parent)
        : DictionaryEditor(title, parent)
    {
        _kind = kindForTitle(title, Kind::CustomVersions);
        setText(serialize(customVersions.isEmpty() ? _defaultCustomVersions : customVersions));
    }

    DictionaryEditor::DictionaryEditor(const QHash<QString, bool>& options, const QString& title,
                                       QWidget* parent)
        : DictionaryEditor(title, parent)
    {
        _kind = kindForTitle(title, Kind::Options);
        setText(serialize(options.isEmpty() ? _defaultOptions : options));
    }

    DictionaryEditor::DictionaryEditor(const QHash<QString, MapStructType>& options, const QString& title,
                                       QWidget* parent)
        : DictionaryEditor(title, parent)
    {
        _kind = kindForTitle(title, Kind::MapStructTypes);
        setText(serialize(options.isEmpty() ? _defaultMapStructTypes : options));
    }

    QString DictionaryEditor::text() const { return _editor->toPlainText(); }

    void DictionaryEditor::setText(const QString& value) { _editor->setPlainText(value); }

    DictionaryEditor::Kind DictionaryEditor::kindForTitle(const QString& title, Kind fallback)
    {
        if (title == CustomVersionsTitle) return Kind::CustomVersions;
        if (title == OptionsTitle) return Kind::Options;
        if (title == MapStructTypesTitle) return Kind::MapStructTypes;
        return fallback;
    }

    QString DictionaryEditor::serialize(const QList<FCustomVersion>& customVersions)
    {
        VersioningSettings settings;
        settings.setCustomVersions(customVersions);
        return toIndentedJson(settings, QStringLiteral("CustomVersions"));
    }

    QString DictionaryEditor::serialize(const QHash<QString, bool>& options)
    {
        VersioningSettings settings;
        settings.setOptions(options);
        return toIndentedJson(settings, QStringLiteral("Options"));
    }

    QString DictionaryEditor::serialize(const QHash<QString, MapStructType>& mapStructTypes)
    {
        VersioningSettings settings;
        settings.setMapStructTypes(mapStructTypes);
        return toIndentedJson(settings, QStringLiteral("MapStructTypes"));
    }

    void DictionaryEditor::showBrokenFormat()
    {
        // C#'s catch-all: the window stays open with the message turned red, and DialogResult is left unset.
        _heBrokeIt->setText(QStringLiteral("GG YOU BROKE THE FORMAT, FIX THE JSON OR RESET THE CHANGES!"));
        _heBrokeIt->setStyleSheet(QStringLiteral("color:%1;").arg(Constants::RED));
    }

    void DictionaryEditor::onClick()
    {
        VersioningSettings parsed;

        switch (_kind)
        {
            case Kind::CustomVersions:
                if (!fromJson(text(), QStringLiteral("CustomVersions"), parsed))
                    return showBrokenFormat();
                _customVersions = parsed.customVersions();
                // C#: `// DialogResult = !CustomVersions.SequenceEqual(_defaultCustomVersions);` — commented
                // out upstream in all three arms, so OK always accepts.
                accept();
                break;
            case Kind::Options:
                if (!fromJson(text(), QStringLiteral("Options"), parsed))
                    return showBrokenFormat();
                _options = parsed.options();
                accept();
                break;
            case Kind::MapStructTypes:
                if (!fromJson(text(), QStringLiteral("MapStructTypes"), parsed))
                    return showBrokenFormat();
                _mapStructTypes = parsed.mapStructTypes();
                accept();
                break;
        }
    }

    void DictionaryEditor::onReset()
    {
        switch (_kind)
        {
            case Kind::CustomVersions: setText(serialize(_defaultCustomVersions)); break;
            case Kind::Options:        setText(serialize(_defaultOptions)); break;
            case Kind::MapStructTypes: setText(serialize(_defaultMapStructTypes)); break;
        }
    }
}
