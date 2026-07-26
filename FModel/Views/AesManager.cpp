// Ported from FModel/Views/AesManager.xaml (+ .xaml.cs)
#include "AesManager.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "../ViewModels/AesManagerViewModel.h"
#include "../ViewModels/ApplicationViewModel.h"
#include "../ViewModels/CUE4ParseViewModel.h"
#include "../ViewModels/GameDirectoryViewModel.h"

namespace FModel::Views
{
    using ViewModels::AesManagerViewModel;
    using ViewModels::ApplicationViewModel;
    using ViewModels::FileItem;

    AesManager::AesManager(ApplicationViewModel* applicationView, QWidget* parent)
        : QDialog(parent), _applicationView(applicationView)
    {
        setWindowTitle(QStringLiteral("AES Manager"));
        resize(720, 420);

        auto* root = new QVBoxLayout(this);
        root->addWidget(new QLabel(
            QStringLiteral("Enter the AES key(s) needed to decrypt this game's archives."), this));

        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        auto* content = new QWidget(scroll);
        _rows = new QVBoxLayout(content);
        _rows->addStretch(1);
        scroll->setWidget(content);
        root->addWidget(scroll, 1);

        auto* buttons = new QDialogButtonBox(this);
        auto* refresh = buttons->addButton(QStringLiteral("Refresh AES"), QDialogButtonBox::ActionRole);
        connect(refresh, &QPushButton::clicked, this, &AesManager::onRefreshAes);
        auto* ok = buttons->addButton(QDialogButtonBox::Ok);
        connect(ok, &QPushButton::clicked, this, &AesManager::onClick);
        root->addWidget(buttons);

        rebuildRows();
    }

    void AesManager::rebuildRows()
    {
        // Drop every row but the trailing stretch.
        while (_rows->count() > 1)
        {
            QLayoutItem* item = _rows->takeAt(0);
            if (item->widget() != nullptr)
                item->widget()->deleteLater();
            delete item;
        }

        AesManagerViewModel* aes = _applicationView != nullptr ? _applicationView->aesManager() : nullptr;
        if (aes == nullptr || aes->aesKeys() == nullptr)
            return;

        int index = 0;
        for (FileItem* item : aes->aesKeysView())
        {
            auto* row = new QWidget(this);
            auto* layout = new QVBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 6);

            auto* caption = new QLabel(item->name(), row);
            caption->setStyleSheet(QStringLiteral("font-weight:600;"));
            layout->addWidget(caption);

            auto* editor = new QLineEdit(item->key(), row);
            editor->setPlaceholderText(QStringLiteral("0x0000000000000000000000000000000000000000000000000000000000000000"));
            // Writing the key back on editingFinished is what raises FileItem's PropertyChanged, which is
            // what AesManagerViewModel's item listener turns into a settings write + HasChange.
            connect(editor, &QLineEdit::editingFinished, this, [item, editor]
            {
                item->setKey(editor->text());
            });
            layout->addWidget(editor);

            _rows->insertWidget(index++, row);
        }
    }

    void AesManager::onRefreshAes()
    {
        // C#: await CUE4Parse.RefreshAes(); await AesManager.InitAes(); AesManager.HasChange = true;
        // RefreshAes pulls the keys from the configured API endpoint, which is unported — the rest runs.
        if (_applicationView == nullptr || _applicationView->aesManager() == nullptr)
            return;

        _applicationView->aesManager()->initAes();
        _applicationView->aesManager()->setHasChange(true); // yes even if nothing actually changed
        rebuildRows();
    }

    void AesManager::onClick()
    {
        close(); // C#: Close() — the remount happens in closeEvent, as OnClosing does upstream
    }

    void AesManager::closeEvent(QCloseEvent* event)
    {
        if (!_providerUpdated && _applicationView != nullptr)
        {
            _providerUpdated = true;
            _applicationView->updateProvider(false);
        }
        QDialog::closeEvent(event);
    }
}
