// Ported from CUE4Parse/UE4/Objects/Core/Math/FRotator.cs — a container for rotation information.
// All rotation values are stored in degrees.
//
// Deferred / deviations:
//   - C#'s FRotator(double,double,double) truncating ctor is omitted for the same reason as FVector's:
//     keeping it next to the float ctor makes every all-int construction ambiguous in C++ (C# instead
//     prefers the float overload). The few double sources cast to float explicitly at the call site.
//   - Serialize(FArchiveWriter) / the System.Numerics Vector3 conversion / GetHashCode — not on the parse path.
//
// RotateVector / UnrotateVector / Quaternion() (which touch FRotationMatrix and FQuat) are declared here
// and defined in FRotator.cpp.
#pragma once

#include <cmath>
#include <cstdio>
#include <string>

#include "FVector.h"
#include "EForceInit.h"
#include "UnrealMathUtility.h"
#include "../../../Readers/FArchive.h"
#include "../../../Versions/EGame.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FQuat; // FQuat.h

    struct FRotator
    {
        static constexpr float KindaSmallNumber = 1e-4f;

        static const FRotator ZeroRotator;

        /** Rotation around the right axis (around Y axis), looking up and down (0=Straight Ahead, +Up, -Down). */
        float Pitch = 0.0f;

        /** Rotation around the up axis (around Z axis), running in circles 0=East, +North, -South. */
        float Yaw = 0.0f;

        /** Rotation around the forward axis (around X axis), tilting your head, 0=Straight, +Clockwise, -CCW. */
        float Roll = 0.0f;

        FRotator() = default;
        explicit FRotator(EForceInit) : FRotator(0.0f, 0.0f, 0.0f) {}
        explicit FRotator(float f) : FRotator(f, f, f) {}
        FRotator(float pitch, float yaw, float roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}

        explicit FRotator(FArchive& Ar)
        {
            if (Ar.Game() < CUE4Parse::UE4::Versions::GAME_UE4_0)
            {
                constexpr float scale = 360.0f / 65536.0f;
                Pitch = Ar.Read<int32_t>() * scale;
                Yaw = Ar.Read<int32_t>() * scale;
                Roll = Ar.Read<int32_t>() * scale;
                return;
            }

            Pitch = Ar.ReadFReal();
            Yaw = Ar.ReadFReal();
            Roll = Ar.ReadFReal();
        }

        friend FRotator operator+(const FRotator& a, const FRotator& b) { return FRotator(a.Pitch + b.Pitch, a.Yaw + b.Yaw, a.Roll + b.Roll); }
        friend FRotator operator-(const FRotator& a, const FRotator& b) { return FRotator(a.Pitch - b.Pitch, a.Yaw - b.Yaw, a.Roll - b.Roll); }
        friend FRotator operator*(const FRotator& r, float scale) { return FRotator(r.Pitch * scale, r.Yaw * scale, r.Roll * scale); }

        // Defined in FRotator.cpp (need FRotationMatrix / FQuat complete).
        FVector RotateVector(const FVector& v) const;
        FVector UnrotateVector(const FVector& v) const;
        FQuat Quaternion() const;

        FVector Vector() const
        {
            const float pitchRadians = CUE4Parse::Utils::ToRadians(Pitch);
            const float sp = std::sin(pitchRadians);
            const float cp = std::cos(pitchRadians);
            const float yawRadians = CUE4Parse::Utils::ToRadians(Yaw);
            const float sy = std::sin(yawRadians);
            const float cy = std::cos(yawRadians);

            return FVector(cp * cy, cp * sy, sp);
        }

        void Normalize()
        {
            Pitch = NormalizeAxis(Pitch);
            Yaw = NormalizeAxis(Yaw);
            Roll = NormalizeAxis(Roll);
        }

        FRotator GetNormalized() const
        {
            FRotator rot = *this;
            rot.Normalize();
            return rot;
        }

        // Returns Angle in the range [0,360).
        static float ClampAxis(float angle)
        {
            angle = std::fmod(angle, 360.0f);
            if (angle < 0.0f)
                angle += 360.0f; // shift to [0,360) range
            return angle;
        }

        // Returns Angle in the range (-180,180].
        static float NormalizeAxis(float angle)
        {
            angle = ClampAxis(angle);
            if (angle > 180.0f)
                angle -= 360.0f; // shift to (-180,180]
            return angle;
        }

        // map [0->360) to [0->256) and mask off any winding
        static uint8_t CompressAxisToByte(float angle) { return static_cast<uint8_t>(CUE4Parse::Utils::RoundToInt(angle * 256.0f / 360.0f) & 0xFF); }
        // map [0->256) to [0->360)
        static float DecompressAxisFromByte(uint8_t angle) { return angle * 360.0f / 256.0f; }

        // map [0->360) to [0->65536) and mask off any winding
        static uint16_t CompressAxisToShort(float angle) { return static_cast<uint16_t>(CUE4Parse::Utils::RoundToInt(angle * 65536.0f / 360.0f) & 0xFFF); }
        // map [0->65536) to [0->360)
        static float DecompressAxisFromShort(uint16_t angle) { return angle * 360.0f / 65536.0f; }

        bool Equals(const FRotator& r, float tolerance) const
        {
            return std::fabs(NormalizeAxis(Pitch - r.Pitch)) <= tolerance &&
                std::fabs(NormalizeAxis(Yaw - r.Yaw)) <= tolerance &&
                std::fabs(NormalizeAxis(Roll - r.Roll)) <= tolerance;
        }
        bool Equals(const FRotator& r) const { return Equals(r, KindaSmallNumber); }

        std::string ToString() const
        {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "P=%g Y=%g R=%g", Pitch, Yaw, Roll);
            return buf;
        }

        friend bool operator==(const FRotator& a, const FRotator& b) { return a.Equals(b); }
        friend bool operator!=(const FRotator& a, const FRotator& b) { return !a.Equals(b); }
    };

    inline const FRotator FRotator::ZeroRotator{0.0f, 0.0f, 0.0f};
}
