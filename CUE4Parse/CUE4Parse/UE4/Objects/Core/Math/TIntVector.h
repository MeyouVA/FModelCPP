// Ported from CUE4Parse/UE4/Objects/Core/Math/TIntVector.cs — the generic N-component vectors.
// These are the element types of TBox2/TBox3 and of the packed vertex streams; they are plain layout structs
// read straight out of an archive, so they intentionally have no arithmetic.
//
// C#'s `where T : struct` on TVector is expressed here as a plain template parameter: any trivially readable
// type works, and Ar.Read<T> already constrains it at the point of use.
#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    template <typename T>
    struct TIntVector1
    {
        T Value{};

        TIntVector1() = default;
        explicit TIntVector1(T value) : Value(value) {}
    };

    template <typename T>
    struct TIntVector2
    {
        T X{};
        T Y{};

        TIntVector2() = default;
        TIntVector2(T x, T y) : X(x), Y(y) {}

        std::string ToString() const
        {
            return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y);
        }
    };

    template <typename T>
    struct TIntVector3
    {
        T X{};
        T Y{};
        T Z{};

        TIntVector3() = default;
        TIntVector3(T x, T y, T z) : X(x), Y(y), Z(z) {}

        std::string ToString() const
        {
            return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y) + ", Z: " + std::to_string(Z);
        }
    };

    template <typename T>
    struct TIntVector4
    {
        T X{};
        T Y{};
        T Z{};
        T W{};

        TIntVector4() = default;
        TIntVector4(T x, T y, T z, T w) : X(x), Y(y), Z(z), W(w) {}

        std::string ToString() const
        {
            return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y) +
                   ", Z: " + std::to_string(Z) + ", W: " + std::to_string(W);
        }
    };

    // The runtime-sized vector (SerializeReal in Vector.h). C# backs it with T[]; std::vector is the equivalent
    // and gives the same value semantics on copy.
    template <typename T>
    class TVector
    {
    public:
        explicit TVector(int dimension) : _values(static_cast<size_t>(dimension)) {}
        TVector(int dimension, T initialValue) : _values(static_cast<size_t>(dimension), initialValue) {}
        TVector(FArchive& Ar, int dimension) : _values(static_cast<size_t>(dimension))
        {
            for (int i = 0; i < dimension; ++i) _values[static_cast<size_t>(i)] = Ar.Read<T>();
        }
        explicit TVector(std::vector<T> values) : _values(std::move(values)) {}

        int Dimension() const { return static_cast<int>(_values.size()); }

        const T& operator[](int i) const
        {
            if (i < 0 || static_cast<size_t>(i) >= _values.size()) throw std::out_of_range("TVector index");
            return _values[static_cast<size_t>(i)];
        }

    private:
        std::vector<T> _values;
    };
}
