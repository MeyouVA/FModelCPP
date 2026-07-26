#pragma once
// Ported from FModel/Views/Resources/Controls/EndpointEditor.xaml (+ .xaml.cs) — the per-endpoint editor the
// settings window opens for the AES and mapping endpoints.
//
// Deliberate differences from C#:
//   * The two AvalonEdit response panes become read-only QPlainTextEdits, and the "Send" / "Test" buttons are
//     DEFERRED: both go through ApplicationService.ApiEndpointView.DynamicApi, and neither the API-endpoint
//     view-models nor the HTTP layer is ported (EndpointSettings.h says the same about TryValidate). The
//     buttons exist and are wired, but each reports through the `deferred` signal instead of firing a request
//     — the same seam MenuCommand uses for its unported arms. _isTested therefore only ever holds the
//     endpoint's stored IsValid, which is what OnClick reads.
//   * The two "open the JSONPath docs" buttons keep working; they route through the same openUrl seam
//     MenuCommand exposes so a test can observe them without launching a browser.
//   * The editor does not own the EndpointSettings it edits — it points at the live object inside
//     UserSettings.Default.CurrentDir.Endpoints, exactly as C#'s DataContext does, so edits are visible to the
//     settings view-model as they happen (that is how MappingEndpoint's PropertyChanged hook sees them).

#include <QDialog>
#include <QString>
#include <QUrl>

#include "../../../Enums.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

namespace FModel::Settings { class EndpointSettings; }

namespace FModel::Views::Resources::Controls
{
    class EndpointEditor : public QDialog
    {
        Q_OBJECT

    public:
        // The instruction blocks C# selects on the endpoint type. Exposed so a test can assert the right one
        // is shown without reading the widget tree.
        static QString instructions(EEndpointType type);

        EndpointEditor(Settings::EndpointSettings* endpoint, const QString& title, EEndpointType type,
                       QWidget* parent = nullptr);

        Settings::EndpointSettings* endpoint() const { return _endpoint; }

        // C#'s OnClick: accepts only when the endpoint has been tested AND reports itself valid.
        void onClick();
        // C#'s OnSend / OnTest — see the deferral note above.
        void onSend();
        void onTest();
        // C#'s OnSyntax / OnEvaluator: the two documentation links.
        void onSyntax();
        void onEvaluator();

        static const QString SyntaxLink;
        static const QString EvaluatorLink;

    signals:
        // Emitted in place of the request the C# handler would fire, naming the arm and what it waits on.
        void deferred(const QString& arm, const QString& waitingOn);

    private:
        Settings::EndpointSettings* _endpoint = nullptr;
        EEndpointType _type = EEndpointType::Aes;
        bool _isTested = false;

        QLineEdit* _url = nullptr;
        QLineEdit* _path = nullptr;
        QLineEdit* _filePath = nullptr;
        QCheckBox* _overwrite = nullptr;
        QPlainTextEdit* _targetResponse = nullptr;
        QPlainTextEdit* _endpointResponse = nullptr;
    };
}
