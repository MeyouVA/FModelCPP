// Ported from CUE4Parse/UE4/Objects/Core/Misc/FDateTime.cs
// A .NET-tick timestamp (100-nanosecond intervals since 0001-01-01 00:00:00).
//
// Deliberate difference: C# formats via `new DateTime(Ticks):F`, a culture-dependent long date/time.
// That is not reproducible without the .NET culture machinery, so ToString() emits a fixed,
// deterministic UTC "yyyy.MM.dd-HH.mm.ss" rendering computed straight from the tick count.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Misc
{
    struct FDateTime : public UE4::IUStruct
    {
        int64_t Ticks = 0;

        FDateTime() = default;
        explicit FDateTime(int64_t ticks) : Ticks(ticks) {}

        std::string ToString() const
        {
            constexpr int64_t TICKS_PER_SECOND = 10000000LL;
            constexpr int64_t TICKS_PER_DAY = TICKS_PER_SECOND * 86400LL;
            // Days from 0001-01-01 (tick epoch) to 1970-01-01, the epoch civil_from_days uses.
            constexpr int64_t DAYS_TO_UNIX_EPOCH = 719162;

            int64_t totalDays = Ticks / TICKS_PER_DAY;
            int64_t remTicks = Ticks % TICKS_PER_DAY;
            if (remTicks < 0) { remTicks += TICKS_PER_DAY; totalDays -= 1; }

            const int64_t secondsOfDay = remTicks / TICKS_PER_SECOND;
            const int hour = static_cast<int>(secondsOfDay / 3600);
            const int minute = static_cast<int>((secondsOfDay % 3600) / 60);
            const int second = static_cast<int>(secondsOfDay % 60);

            int year, month, day;
            CivilFromDays(totalDays - DAYS_TO_UNIX_EPOCH, year, month, day);

            char buf[32];
            std::snprintf(buf, sizeof(buf), "%04d.%02d.%02d-%02d.%02d.%02d",
                          year, month, day, hour, minute, second);
            return buf;
        }

    private:
        // Howard Hinnant's days-from-civil inverse: civil date from a day number relative to 1970-01-01.
        static void CivilFromDays(int64_t z, int& year, int& month, int& day)
        {
            z += 719468;
            const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
            const int64_t doe = z - era * 146097;                     // [0, 146096]
            const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
            const int64_t y = yoe + era * 400;
            const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
            const int64_t mp = (5 * doy + 2) / 153;                    // [0, 11]
            const int64_t d = doy - (153 * mp + 2) / 5 + 1;            // [1, 31]
            const int64_t m = mp < 10 ? mp + 3 : mp - 9;              // [1, 12]
            year = static_cast<int>(y + (m <= 2 ? 1 : 0));
            month = static_cast<int>(m);
            day = static_cast<int>(d);
        }
    };
}
