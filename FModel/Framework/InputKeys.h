#pragma once
// Stand-in for System.Windows.Input.Key and System.Windows.Input.ModifierKeys (WPF), which Hotkey — and
// therefore the persisted settings file — is defined in terms of.
//
// These are NOT free to redefine: UserSettings serialises each Hotkey as {"Key": <int>, "Modifiers": <int>},
// so the numeric values here are part of the on-disk format and must match WPF's enum exactly. The values are
// anchored by real settings files (e.g. Key.A == 44, Key.T == 63, ModifierKeys.Control == 2); test_settings
// asserts those anchors so a mistyped constant cannot silently corrupt someone's hotkeys.
//
// Qt::Key is deliberately not reused: its values differ, so persisting it would break format compatibility.
// The eventual key-handling code maps Qt::Key -> Key at the edge instead.
//
// keyName() has to reproduce C#'s Key.ToString(), which means a name for every enumerator. Rather than
// maintain a second table by hand (or generate one with an X-macro, which MSVC's preprocessor mangles when
// the callback is invoked indirectly), the enum is registered with the meta-object system: moc derives the
// names from this very declaration, so they cannot drift from the enumerators.

#include <QMetaEnum>
#include <QObject>
#include <QString>

namespace FModel::Framework
{
    Q_NAMESPACE

    // Mirrors System.Windows.Input.Key. WPF aliases (Enter == Return, CapsLock == Capital, ...) are declared
    // after the primary names so QMetaEnum::valueToKey keeps returning the primary one, as ToString() does.
    enum class Key
    {
        None = 0,
        Cancel = 1,
        Back = 2,
        Tab = 3,
        LineFeed = 4,
        Clear = 5,
        Return = 6,
        Pause = 7,
        Capital = 8,
        KanaMode = 9,
        JunjaMode = 10,
        FinalMode = 11,
        HanjaMode = 12,
        Escape = 13,
        ImeConvert = 14,
        ImeNonConvert = 15,
        ImeAccept = 16,
        ImeModeChange = 17,
        Space = 18,
        Prior = 19,
        Next = 20,
        End = 21,
        Home = 22,
        Left = 23,
        Up = 24,
        Right = 25,
        Down = 26,
        Select = 27,
        Print = 28,
        Execute = 29,
        PrintScreen = 30,
        Insert = 31,
        Delete = 32,
        Help = 33,

        D0 = 34, D1 = 35, D2 = 36, D3 = 37, D4 = 38,
        D5 = 39, D6 = 40, D7 = 41, D8 = 42, D9 = 43,

        A = 44, B = 45, C = 46, D = 47, E = 48, F = 49, G = 50,
        H = 51, I = 52, J = 53, K = 54, L = 55, M = 56, N = 57,
        O = 58, P = 59, Q = 60, R = 61, S = 62, T = 63, U = 64,
        V = 65, W = 66, X = 67, Y = 68, Z = 69,

        LWin = 70,
        RWin = 71,
        Apps = 72,
        Sleep = 73,

        NumPad0 = 74, NumPad1 = 75, NumPad2 = 76, NumPad3 = 77, NumPad4 = 78,
        NumPad5 = 79, NumPad6 = 80, NumPad7 = 81, NumPad8 = 82, NumPad9 = 83,

        Multiply = 84,
        Add = 85,
        Separator = 86,
        Subtract = 87,
        Decimal = 88,
        Divide = 89,

        F1 = 90,   F2 = 91,   F3 = 92,   F4 = 93,   F5 = 94,   F6 = 95,
        F7 = 96,   F8 = 97,   F9 = 98,   F10 = 99,  F11 = 100, F12 = 101,
        F13 = 102, F14 = 103, F15 = 104, F16 = 105, F17 = 106, F18 = 107,
        F19 = 108, F20 = 109, F21 = 110, F22 = 111, F23 = 112, F24 = 113,

        NumLock = 114,
        Scroll = 115,
        LeftShift = 116,
        RightShift = 117,
        LeftCtrl = 118,
        RightCtrl = 119,
        LeftAlt = 120,
        RightAlt = 121,
        BrowserBack = 122,
        BrowserForward = 123,
        BrowserRefresh = 124,
        BrowserStop = 125,
        BrowserSearch = 126,
        BrowserFavorites = 127,
        BrowserHome = 128,
        VolumeMute = 129,
        VolumeDown = 130,
        VolumeUp = 131,
        MediaNextTrack = 132,
        MediaPreviousTrack = 133,
        MediaStop = 134,
        MediaPlayPause = 135,
        LaunchMail = 136,
        SelectMedia = 137,
        LaunchApplication1 = 138,
        LaunchApplication2 = 139,
        Oem1 = 140,
        OemPlus = 141,
        OemComma = 142,
        OemMinus = 143,
        OemPeriod = 144,
        Oem2 = 145,
        Oem3 = 146,
        AbntC1 = 147,
        AbntC2 = 148,
        Oem4 = 149,
        Oem5 = 150,
        Oem6 = 151,
        Oem7 = 152,
        Oem8 = 153,
        Oem102 = 154,
        ImeProcessed = 155,
        System = 156,
        OemAttn = 157,
        OemFinish = 158,
        OemCopy = 159,
        OemAuto = 160,
        OemEnlw = 161,
        OemBackTab = 162,
        Attn = 163,
        CrSel = 164,
        ExSel = 165,
        EraseEof = 166,
        Play = 167,
        Zoom = 168,
        NoName = 169,
        Pa1 = 170,
        OemClear = 171,
        DeadCharProcessed = 172,

        // WPF aliases, for call sites that read better with them.
        Enter = Return,
        CapsLock = Capital,
        HangulMode = KanaMode,
        KanjiMode = HanjaMode,
        PageUp = Prior,
        PageDown = Next,
        Snapshot = PrintScreen,
        OemSemicolon = Oem1,
        OemQuestion = Oem2,
        OemTilde = Oem3,
        OemOpenBrackets = Oem4,
        OemPipe = Oem5,
        OemCloseBrackets = Oem6,
        OemQuotes = Oem7,
        OemBackslash = Oem102,
    };
    Q_ENUM_NS(Key)

    // Equivalent to Key.ToString() in C#. Falls back to the number for a value with no name, which is what
    // C# does too.
    inline QString keyName(Key key)
    {
        const char* name = QMetaEnum::fromType<Key>().valueToKey(static_cast<int>(key));
        return name ? QString::fromLatin1(name) : QString::number(static_cast<int>(key));
    }

    // System.Windows.Input.ModifierKeys ([Flags]).
    enum class ModifierKeys
    {
        None    = 0,
        Alt     = 1,
        Control = 2,
        Shift   = 4,
        Windows = 8
    };
    Q_ENUM_NS(ModifierKeys)

    constexpr ModifierKeys operator|(ModifierKeys a, ModifierKeys b)
    {
        return static_cast<ModifierKeys>(static_cast<int>(a) | static_cast<int>(b));
    }
    constexpr ModifierKeys& operator|=(ModifierKeys& a, ModifierKeys b) { a = a | b; return a; }

    // C#'s Enum.HasFlag. Note HasFlag(x, None) is true in C# too (0 is a subset of everything).
    constexpr bool hasFlag(ModifierKeys value, ModifierKeys flag)
    {
        return (static_cast<int>(value) & static_cast<int>(flag)) == static_cast<int>(flag);
    }
}
