// Ported from CUE4Parse/UE4/Assets/Objects/Unversioned/FIterator.cs
// Walks an FUnversionedHeader's fragments, yielding (schema index, is-non-zero) for each serialized value.
//
// Deliberate difference from C#: the List<FFragment> IEnumerator becomes an index; C# reads
// enumerator.Current *before* the first MoveNext (a default FFragment — List enumerators don't throw),
// which acts as a zero sentinel the Skip() loop steps past, so index -1 maps to a default FFragment here.
#pragma once

#include <utility>
#include <vector>

#include "FFragment.h"
#include "FUnversionedHeader.h"
#include "../../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Objects::Unversioned
{
    class FIterator
    {
    public:
        explicit FIterator(const FUnversionedHeader& header)
            : _header(&header)
        {
            if (header.HasValues())
                Skip();
        }

        bool MoveNext()
        {
            _schemaIt++;
            _remainingFragmentValues--;
            if (Cur().HasAnyZeroes)
                _zeroMaskIndex++;

            if (_remainingFragmentValues == 0)
            {
                if (Cur().IsLast)
                    return false;

                _fragmentIndex++;
                Skip();
            }
            return true;
        }

        bool IsNonZero() const
        {
            return !Cur().HasAnyZeroes || !_header->ZeroMaskGetOrFalse(static_cast<size_t>(_zeroMaskIndex));
        }

        // C#'s Current: (schema index, value must be deserialized rather than zero-defaulted).
        std::pair<int, bool> Current() const { return { _schemaIt, IsNonZero() }; }

    private:
        FFragment Cur() const
        {
            const auto& fragments = _header->Fragments;
            return _fragmentIndex >= 0 && _fragmentIndex < static_cast<int>(fragments.size())
                ? fragments[static_cast<size_t>(_fragmentIndex)]
                : FFragment();
        }

        void Skip()
        {
            _schemaIt += Cur().SkipNum;

            while (Cur().ValueNum == 0)
            {
                if (Cur().IsLast)
                    throw Exceptions::ParserException("Cannot receive last fragment in Skip()");
                _fragmentIndex++;
                _schemaIt += Cur().SkipNum;
            }

            _remainingFragmentValues = Cur().ValueNum;
        }

        const FUnversionedHeader* _header;
        int _schemaIt = 0;
        int _zeroMaskIndex = 0;
        int _fragmentIndex = -1; // "before first" (see header note)
        int _remainingFragmentValues = 0;
    };
}
