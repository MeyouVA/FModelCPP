// See Json.h.
#include "Json.h"

#include <cstdint>
#include <cstdlib>

namespace CUE4Parse::Utils::Json
{
    const std::string& JValue::EmptyString()
    {
        static const std::string empty;
        return empty;
    }

    const JValue& JValue::Null()
    {
        static const JValue null;
        return null;
    }

    const JValue& JValue::operator[](const std::string& key) const
    {
        for (const JValue& member : _values)
            if (member._name == key) return member;
        return Null();
    }

    const JValue& JValue::operator[](size_t index) const
    {
        return index < _values.size() ? _values[index] : Null();
    }

    // Recursive-descent reader over the raw text. One instance per Parse call; `_pos` is the only state.
    class JsonParser
    {
    public:
        explicit JsonParser(const std::string& text) : _text(text) {}

        bool ParseDocument(JValue& out)
        {
            SkipWhitespace();
            if (!ParseValue(out)) return false;
            SkipWhitespace();
            // Trailing garbage is an error, so a truncated-then-concatenated file cannot silently half-parse.
            return _pos == _text.size();
        }

    private:
        bool AtEnd() const { return _pos >= _text.size(); }
        char Peek() const { return _text[_pos]; }

        void SkipWhitespace()
        {
            while (!AtEnd())
            {
                const char c = Peek();
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++_pos;
                else break;
            }
        }

        bool Literal(const char* word)
        {
            const size_t n = std::char_traits<char>::length(word);
            if (_text.compare(_pos, n, word) != 0) return false;
            _pos += n;
            return true;
        }

        bool ParseValue(JValue& out)
        {
            if (AtEnd()) return false;
            switch (Peek())
            {
                case '{': return ParseObject(out);
                case '[': return ParseArray(out);
                case '"':
                {
                    if (!ParseString(out._text)) return false;
                    out._type = EJsonType::String;
                    return true;
                }
                case 't':
                    if (!Literal("true")) return false;
                    out._type = EJsonType::Boolean;
                    out._boolean = true;
                    return true;
                case 'f':
                    if (!Literal("false")) return false;
                    out._type = EJsonType::Boolean;
                    out._boolean = false;
                    return true;
                case 'n':
                    if (!Literal("null")) return false;
                    out._type = EJsonType::Null;
                    return true;
                default: return ParseNumber(out);
            }
        }

        bool ParseObject(JValue& out)
        {
            ++_pos; // '{'
            out._type = EJsonType::Object;
            SkipWhitespace();
            if (!AtEnd() && Peek() == '}') { ++_pos; return true; }

            while (true)
            {
                SkipWhitespace();
                if (AtEnd() || Peek() != '"') return false;

                JValue member;
                if (!ParseString(member._name)) return false;
                SkipWhitespace();
                if (AtEnd() || Peek() != ':') return false;
                ++_pos;
                SkipWhitespace();
                if (!ParseValue(member)) return false;

                // Newtonsoft's last-one-wins for a duplicated key; the parse cannot fail on one.
                out._values.push_back(std::move(member));

                SkipWhitespace();
                if (AtEnd()) return false;
                if (Peek() == ',') { ++_pos; continue; }
                if (Peek() == '}') { ++_pos; return true; }
                return false;
            }
        }

        bool ParseArray(JValue& out)
        {
            ++_pos; // '['
            out._type = EJsonType::Array;
            SkipWhitespace();
            if (!AtEnd() && Peek() == ']') { ++_pos; return true; }

            while (true)
            {
                SkipWhitespace();
                JValue element;
                if (!ParseValue(element)) return false;
                out._values.push_back(std::move(element));

                SkipWhitespace();
                if (AtEnd()) return false;
                if (Peek() == ',') { ++_pos; continue; }
                if (Peek() == ']') { ++_pos; return true; }
                return false;
            }
        }

        bool ParseNumber(JValue& out)
        {
            const size_t start = _pos;
            if (!AtEnd() && (Peek() == '-' || Peek() == '+')) ++_pos;
            while (!AtEnd())
            {
                const char c = Peek();
                if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '-' || c == '+') ++_pos;
                else break;
            }
            if (_pos == start) return false;

            const std::string token = _text.substr(start, _pos - start);
            char* end = nullptr;
            const double value = std::strtod(token.c_str(), &end);
            if (end != token.c_str() + token.size()) return false;

            out._type = EJsonType::Number;
            out._number = value;
            return true;
        }

        bool ParseString(std::string& out)
        {
            ++_pos; // opening quote
            out.clear();
            while (true)
            {
                if (AtEnd()) return false;
                const char c = _text[_pos++];
                if (c == '"') return true;
                if (c != '\\') { out.push_back(c); continue; }

                if (AtEnd()) return false;
                const char esc = _text[_pos++];
                switch (esc)
                {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u':
                    {
                        uint32_t code = 0;
                        if (!ParseHex4(code)) return false;
                        // A high surrogate must be followed by \uDC00-\uDFFF; anything else is left as the
                        // lone code point, which is what .NET's reader does with an unpaired surrogate.
                        if (code >= 0xD800 && code <= 0xDBFF && _text.compare(_pos, 2, "\\u") == 0)
                        {
                            const size_t save = _pos;
                            _pos += 2;
                            uint32_t low = 0;
                            if (ParseHex4(low) && low >= 0xDC00 && low <= 0xDFFF)
                                code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                            else
                                _pos = save;
                        }
                        AppendUtf8(out, code);
                        break;
                    }
                    default: return false;
                }
            }
        }

        bool ParseHex4(uint32_t& out)
        {
            if (_pos + 4 > _text.size()) return false;
            out = 0;
            for (int i = 0; i < 4; ++i)
            {
                const char c = _text[_pos++];
                uint32_t digit;
                if (c >= '0' && c <= '9') digit = static_cast<uint32_t>(c - '0');
                else if (c >= 'a' && c <= 'f') digit = static_cast<uint32_t>(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') digit = static_cast<uint32_t>(c - 'A' + 10);
                else return false;
                out = (out << 4) | digit;
            }
            return true;
        }

        static void AppendUtf8(std::string& out, uint32_t code)
        {
            if (code < 0x80)
            {
                out.push_back(static_cast<char>(code));
            }
            else if (code < 0x800)
            {
                out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            else if (code < 0x10000)
            {
                out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            else
            {
                out.push_back(static_cast<char>(0xF0 | (code >> 18)));
                out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
        }

        const std::string& _text;
        size_t _pos = 0;
    };

    std::optional<JValue> Parse(const std::string& text)
    {
        // A UTF-8 BOM is not part of the grammar but .uplugin files written by the editor carry one.
        const size_t start = text.compare(0, 3, "\xEF\xBB\xBF") == 0 ? 3 : 0;
        const std::string body = start == 0 ? text : text.substr(start);

        JsonParser parser(body);
        JValue root;
        if (!parser.ParseDocument(root)) return std::nullopt;
        return root;
    }
}
