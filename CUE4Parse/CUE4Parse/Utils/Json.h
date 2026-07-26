// No C# counterpart file: this stands in for Newtonsoft.Json, which CUE4Parse takes as a NuGet dependency
// and reaches for through `JsonConvert.DeserializeObject<T>(...)`. C# gets both the parser and the
// reflective object mapping for free; C++ has neither, so this is the parser half and each ported type
// writes its own FromJson (the mapping half) by hand.
//
// Scope is deliberately "read what UE ships": RFC 8259 values, UTF-8 in and out, `\uXXXX` escapes folded
// back to UTF-8 (surrogate pairs included). It is a reader only — nothing here writes JSON, because nothing
// ported so far needs to. Numbers are kept as double, matching what Newtonsoft hands a `float`/`int`
// property after a round trip.
//
// Structural note: object members are stored as array elements that carry a Name, so one std::vector<JValue>
// backs both kinds. That keeps JValue self-referential without a pointer indirection, since std::vector is
// one of the few standard containers allowed to hold an incomplete type.
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace CUE4Parse::Utils::Json
{
    enum class EJsonType
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    class JValue
    {
    public:
        EJsonType Type() const { return _type; }

        bool IsNull() const { return _type == EJsonType::Null; }
        bool IsObject() const { return _type == EJsonType::Object; }
        bool IsArray() const { return _type == EJsonType::Array; }
        bool IsString() const { return _type == EJsonType::String; }

        // Newtonsoft leaves a property at its default when the key is absent or the type does not match, so
        // every accessor takes the fallback rather than throwing.
        bool AsBool(bool fallback = false) const { return _type == EJsonType::Boolean ? _boolean : fallback; }
        double AsNumber(double fallback = 0.0) const { return _type == EJsonType::Number ? _number : fallback; }
        int AsInt(int fallback = 0) const { return _type == EJsonType::Number ? static_cast<int>(_number) : fallback; }
        const std::string& AsString(const std::string& fallback = EmptyString()) const
        { return _type == EJsonType::String ? _text : fallback; }

        // Elements of an array, or the members of an object (each carrying its Name).
        const std::vector<JValue>& Values() const { return _values; }
        size_t Count() const { return _values.size(); }

        // The member's key, empty for array elements and for the root.
        const std::string& Name() const { return _name; }

        // Member lookup. Missing keys give the shared null value, so `v["a"]["b"].AsBool()` is safe on any
        // document — the same "just gives you the default" shape as Newtonsoft binding to a POCO.
        const JValue& operator[](const std::string& key) const;

        // Array/object element by index; out of range gives the shared null value.
        const JValue& operator[](size_t index) const;

        static const JValue& Null();

    private:
        friend class JsonParser;

        static const std::string& EmptyString();

        EJsonType _type = EJsonType::Null;
        bool _boolean = false;
        double _number = 0.0;
        std::string _text;
        std::string _name;
        std::vector<JValue> _values;
    };

    // Parses a whole document. Returns nullopt on malformed input (C# would throw JsonReaderException; every
    // caller so far treats a bad plugin descriptor as "skip this file", so nullopt is the more useful shape).
    std::optional<JValue> Parse(const std::string& text);
}
