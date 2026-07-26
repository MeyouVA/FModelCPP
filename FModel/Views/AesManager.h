#pragma once
// Ported from FModel/Views/AesManager.xaml (+ .xaml.cs) — the window where AES keys are entered.
//
// One row per key: the main static key first, then one per distinct encryption GUID among the registered
// archives. Editing a row writes straight through to the FileItem, which is what AesManagerViewModel
// listens to.
//
// Deliberate differences from C#:
//   * `OnRefreshAes` calls CUE4Parse.RefreshAes() first, which pulls keys from the configured API endpoint;
//     that half is unported, so the button re-reads the keys from settings and still sets HasChange — which
//     is C#'s own behaviour ("yes even if nothing actually changed").
//   * C#'s OnClosing is `async void` and awaits UpdateProvider(false); here closeEvent calls it directly.
//     The remount therefore happens while the window is closing, exactly as upstream intends.
//   * Bindings are explicit: each row's editor writes FileItem::setKey on editing-finished.

#include <QDialog>

class QVBoxLayout;

namespace FModel::ViewModels { class ApplicationViewModel; }

namespace FModel::Views
{
    class AesManager : public QDialog
    {
        Q_OBJECT

    public:
        explicit AesManager(ViewModels::ApplicationViewModel* applicationView, QWidget* parent = nullptr);

    protected:
        void closeEvent(QCloseEvent* event) override;

    private slots:
        void onRefreshAes();
        void onClick(); // OK

    private:
        void rebuildRows();

        ViewModels::ApplicationViewModel* _applicationView = nullptr;
        QVBoxLayout* _rows = nullptr;
        bool _providerUpdated = false;
    };
}
