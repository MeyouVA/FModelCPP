// Ported from CUE4Parse/UE4/Wwise/Objects/Setting.cs
#pragma once

namespace CUE4Parse::UE4::Wwise::Objects
{
    // A typed key paired with a float. T is always an enum in practice (see GlobalSettings and
    // CAkEnvironmentsMgr), and C#'s WriteJson uses SettingType.ToString() as the property name.
    // The JSON half is not ported (CUE4Parse-CPP has no serializer layer yet).
    template <typename T>
    struct Setting
    {
        T SettingType;
        float SettingValue;

        Setting(T settingType, float settingValue)
            : SettingType(settingType), SettingValue(settingValue) {}
    };
}
