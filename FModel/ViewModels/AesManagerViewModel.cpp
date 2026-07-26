// Ported from FModel/ViewModels/AesManagerViewModel.cs
#include "AesManagerViewModel.h"

#include <algorithm>

#include "../Constants.h"
#include "../Helper.h"
#include "../Settings/DirectorySettings.h"
#include "../Settings/UserSettings.h"
#include "CUE4ParseViewModel.h"
#include "GameDirectoryViewModel.h"

namespace FModel::ViewModels
{
    using ApiEndpoints::Models::DynamicKey;

    namespace
    {
        bool byName(FileItem* a, FileItem* b) { return a->name() < b->name(); }
    }

    AesManagerViewModel::AesManagerViewModel(CUE4ParseViewModel* cue4Parse, QObject* parent)
        : ViewModel(parent), _cue4Parse(cue4Parse)
    {
        _hasChange = false;
        _mainKey = new FileItem(QStringLiteral("Main Static Key"), 0, this);
        _mainKey->setGuid(Constants::ZERO_GUID);
    }

    void AesManagerViewModel::initAes()
    {
        auto* currentDir = Settings::UserSettings::Default()->currentDir();
        if (currentDir == nullptr)
            return;

        _keysFromSettings = currentDir->aesKeys();
        _mainKey->setKey(Helper::fixKey(_keysFromSettings.MainKey));

        // A second call rebuilds the list; the old wrapper goes with it, but the rows it held belong to
        // GameDirectoryViewModel (bar _mainKey, which is reused).
        delete _aesKeys;

        _aesKeys = new Framework::FullyObservableCollection<FileItem*>(enumerateAesKeys(), this);
        connect(_aesKeys, &Framework::FullyObservableCollectionBase::itemPropertyChanged,
                this, &AesManagerViewModel::onItemPropertyChanged);

        // SortDescription("Name", Ascending) — see the header for why this is a plain list.
        _aesKeysView = _aesKeys->items();
        std::stable_sort(_aesKeysView.begin(), _aesKeysView.end(), byName);
    }

    QList<FileItem*> AesManagerViewModel::enumerateAesKeys()
    {
        QList<FileItem*> rows;
        rows.append(_mainKey);
        _uniqueGuids.clear();
        _uniqueGuids.insert(QString::fromStdString(Constants::ZERO_GUID.ToString()));

        const bool hasDynamicKeys = _keysFromSettings.hasDynamicKeys();
        if (_cue4Parse == nullptr || _cue4Parse->gameDirectory() == nullptr)
            return rows;

        for (FileItem* file : _cue4Parse->gameDirectory()->directoryFiles().items())
        {
            const QString guid = QString::fromStdString(file->guid().ToString());
            if (file->guid() == Constants::ZERO_GUID || _uniqueGuids.contains(guid))
                continue;
            _uniqueGuids.insert(guid);

            QString k;
            if (hasDynamicKeys)
            {
                for (const DynamicKey& dynamicKey : _keysFromSettings.DynamicKeys)
                {
                    if (dynamicKey.Guid == guid)
                    {
                        k = dynamicKey.Key;
                        break;
                    }
                }
            }

            file->setKey(Helper::fixKey(k));
            rows.append(file);
        }
        return rows;
    }

    void AesManagerViewModel::onItemPropertyChanged(int collectionIndex, const QString& propertyName)
    {
        if (propertyName != QStringLiteral("Key") || _aesKeys == nullptr)
            return;

        FileItem* item = (*_aesKeys)[collectionIndex];
        const QString key = Helper::fixKey(item->key());
        const QString guid = QString::fromStdString(item->guid().ToString());

        if (collectionIndex == 0)
        {
            if (!_hasChange)
                _hasChange = Helper::fixKey(_keysFromSettings.MainKey) != key;

            _keysFromSettings.MainKey = key;
            return;
        }

        if (!_keysFromSettings.hasDynamicKeys())
        {
            // Note upstream's shape: the FIRST dynamic key edit REPLACES the list with a single entry
            // rather than appending to an empty one. Same result, kept as written.
            _hasChange = true;
            DynamicKey created;
            created.Key = key;
            created.Name = item->name();
            created.Guid = guid;
            _keysFromSettings.DynamicKeys = {created};
            return;
        }

        for (DynamicKey& d : _keysFromSettings.DynamicKeys)
        {
            if (d.Guid != guid)
                continue;

            if (!_hasChange)
                _hasChange = Helper::fixKey(d.Key) != key;

            d.Key = key;
            return;
        }

        _hasChange = true;
        DynamicKey added;
        added.Key = key;
        added.Name = item->name();
        added.Guid = guid;
        _keysFromSettings.DynamicKeys.append(added);
    }

    void AesManagerViewModel::setAesKeys()
    {
        if (auto* currentDir = Settings::UserSettings::Default()->currentDir())
            currentDir->setAesKeys(_keysFromSettings);
    }
}
