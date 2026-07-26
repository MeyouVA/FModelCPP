#pragma once
// Ported from FModel/Views/Resources/Controls/DictionaryEditor.xaml (+ .xaml.cs) — the raw-JSON editor the
// settings window opens for the three versioning collections.
//
// Deliberate differences from C#:
//   * AvalonEdit's TextEditor becomes a QPlainTextEdit. Syntax highlighting is dropped with it: C# sets
//     `AvalonExtensions.HighlighterSelector("")`, which for an empty extension resolves to no highlighter
//     anyway, so nothing visible is lost here.
//   * Newtonsoft is replaced by QJsonDocument. Rather than re-deriving the JSON shape, both directions go
//     through a scratch VersioningSettings, so what the editor shows is exactly what AppSettings.json stores
//     (the FGuid converter included) — the same coupling C# gets from sharing Newtonsoft's converters.
//   * Each C# constructor writes `collection ?? _defaultX`, i.e. it shows a sample document when the game has
//     no versioning configured — VersioningSettings leaves all three collections null until something assigns
//     them. The C++ collections are values and start empty rather than null, so emptiness is what stands in
//     for null here.
//   * C# dispatches OnClick on the window Title, and throws NotImplementedException for a title it does not
//     know. The port keeps the string switch (windowTitle()), because the title is genuinely the only thing
//     distinguishing the three modes upstream — but it records which constructor ran, so a caller that
//     renames the window gets its own collection back rather than the exception.

#include <QDialog>
#include <QHash>
#include <QList>
#include <QString>

#include "../../../Settings/VersioningSettings.h"

#include "UE4/Objects/Core/Serialization/FCustomVersion.h"

class QLabel;
class QPlainTextEdit;

namespace FModel::Views::Resources::Controls
{
    class DictionaryEditor : public QDialog
    {
        Q_OBJECT

    public:
        using FCustomVersion = CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersion;
        using MapStructType = Settings::MapStructType;

        // C#'s three window titles, which are also its dispatch keys.
        static const QString CustomVersionsTitle;
        static const QString OptionsTitle;
        static const QString MapStructTypesTitle;

        explicit DictionaryEditor(const QString& title, QWidget* parent = nullptr);
        DictionaryEditor(const QList<FCustomVersion>& customVersions, const QString& title,
                         QWidget* parent = nullptr);
        DictionaryEditor(const QHash<QString, bool>& options, const QString& title, QWidget* parent = nullptr);
        DictionaryEditor(const QHash<QString, MapStructType>& options, const QString& title,
                         QWidget* parent = nullptr);

        // Filled by an accepted OK, in whichever flavour the editor was opened.
        const QList<FCustomVersion>& customVersions() const { return _customVersions; }
        const QHash<QString, bool>& options() const { return _options; }
        const QHash<QString, MapStructType>& mapStructTypes() const { return _mapStructTypes; }

        // The editor's current text — the seam the tests drive OK through without a visible window.
        QString text() const;
        void setText(const QString& value);

        // C#'s OnClick / OnReset button handlers.
        void onClick();
        void onReset();

    private:
        // Which collection this editor edits. C# infers it from the title on every click; the port resolves it
        // once, from the title first (so a caller passing one of the three titles behaves exactly as upstream)
        // and from the constructor otherwise.
        enum class Kind { CustomVersions, Options, MapStructTypes };

        static Kind kindForTitle(const QString& title, Kind fallback);
        static QString serialize(const QList<FCustomVersion>& customVersions);
        static QString serialize(const QHash<QString, bool>& options);
        static QString serialize(const QHash<QString, MapStructType>& mapStructTypes);
        void showBrokenFormat();

        QPlainTextEdit* _editor = nullptr;
        QLabel* _heBrokeIt = nullptr;

        Kind _kind = Kind::CustomVersions;

        QList<FCustomVersion> _defaultCustomVersions;
        QHash<QString, bool> _defaultOptions;
        QHash<QString, MapStructType> _defaultMapStructTypes;

        QList<FCustomVersion> _customVersions;
        QHash<QString, bool> _options;
        QHash<QString, MapStructType> _mapStructTypes;
    };
}
