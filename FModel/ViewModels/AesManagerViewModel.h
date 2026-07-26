#pragma once
// Ported from FModel/ViewModels/AesManagerViewModel.cs — the AES key list: one row for the main key, plus
// one row per distinct encryption GUID found among the registered archives.
//
// The rows ARE the archive rows: EnumerateAesKeys yields the very same FileItem objects the Archives tab
// holds, after stamping each with the key stored in settings. Editing a key in the AES manager therefore
// mutates the archive list's item too, which is deliberate upstream and preserved here.
//
// Deliberate differences from C#:
//   * The `_mainKey` row is owned by this view-model; the dynamic rows are not (they belong to
//     GameDirectoryViewModel). Nothing here deletes them.
//   * `HasChange` drives whether ApplicationViewModel::updateProvider does anything, so its exact
//     accumulation is preserved — including the fact that it latches (nothing ever clears it but a reload).
//   * `AesKeysView` is a plain sorted QList rather than a Framework::CollectionView. The view exists to
//     order the rows by name for the window, and the *membership* only ever changes in InitAes (editing a
//     key changes a row, never the set of rows), so there is nothing for a live view to react to — and
//     CollectionView reads a RangeObservableCollection, which this list deliberately is not.

#include <QObject>
#include <QSet>
#include <QString>

#include "../Framework/CollectionView.h"
#include "../Framework/FullyObservableCollection.h"
#include "../Framework/ViewModel.h"
#include "../ViewModels/ApiEndpoints/Models/AesResponse.h"

namespace FModel::ViewModels
{
    class CUE4ParseViewModel;
    class FileItem;

    class AesManagerViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        // C# takes the CUE4ParseViewModel (for its GameDirectory) and reads the worker off the locator.
        explicit AesManagerViewModel(CUE4ParseViewModel* cue4Parse, QObject* parent = nullptr);

        Framework::FullyObservableCollection<FileItem*>* aesKeys() const { return _aesKeys; }
        const QList<FileItem*>& aesKeysView() const { return _aesKeysView; }

        bool hasChange() const { return _hasChange; }
        void setHasChange(bool value) { _hasChange = value; }

        // C#'s InitAes: re-reads the keys out of settings and rebuilds the row list. Safe to call again
        // (the AES manager window calls it after a refresh).
        void initAes();

        // C#'s SetAesKeys: writes the accumulated key edits back to the current directory's settings.
        void setAesKeys();

    private:
        // C#'s AesKeysOnItemPropertyChanged: index 0 is the main key, everything else is a dynamic key
        // matched by GUID.
        void onItemPropertyChanged(int collectionIndex, const QString& propertyName);
        QList<FileItem*> enumerateAesKeys();

        CUE4ParseViewModel* _cue4Parse = nullptr;
        Framework::FullyObservableCollection<FileItem*>* _aesKeys = nullptr;
        QList<FileItem*> _aesKeysView;
        bool _hasChange = false;

        ApiEndpoints::Models::AesResponse _keysFromSettings;
        QSet<QString> _uniqueGuids; // C#'s HashSet<FGuid>, keyed by the GUID's string form
        // "just so main key gets refreshed in the ui" — a synthetic row with the zero GUID.
        FileItem* _mainKey = nullptr;
    };
}
