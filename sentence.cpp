#include <time.h>
#include "sentence.h"

void nmea::builder::updateCrc (char *sentence) {
    uint8_t crc = sentence [1];
    char *chr;
    for (chr = sentence + 2; *chr != '*'; ++ chr) {
        crc ^= *chr;
    }
    auto hex2chr = [] (int digit) -> char {
        if (digit < 10) return '0' + digit;
        if (digit < 16) return 'A' + digit - 10;
        return '0';
    };
    chr [1] = hex2chr (crc >> 4);
    chr [2] = hex2chr (crc & 15);
}

nmea::builder::Sentence::Sentence (
    bool _isSixBitEncoded,
    const char *_talkerID,
    const char *_type
): isSixBitEncoded (_isSixBitEncoded), type (_type), talkerID (_talkerID) {
    resize (1);
    (*this) [0] += isSixBitEncoded ? '!' : '$';
    (*this) [0] += talkerID;
    (*this) [0] += type;
}

void nmea::builder::Sentence::setAsUtc (size_t index, time_t utc) {
    if (!utc) utc = time (nullptr);
    tm *now = localtime (& utc);
    char value [20];
    sprintf (value, "%02d%02d%02d00", now->tm_hour, now->tm_min, now->tm_sec);
    (*this) [index] = value;
}

void nmea::builder::Sentence::setAsInt (size_t index, int value) {
    (*this) [index] = std::to_string (value);
}

void nmea::builder::Sentence::setAsChar (size_t index, char value) {
    (*this) [index].clear ();
    (*this) [index] += value;
}

void nmea::builder::Sentence::setAsAngle (size_t index, double value) {
    char angle [10];
    sprintf (angle, "%05.1f", value);
    (*this) [index] = angle;
}

void nmea::builder::Sentence::setAsCoord (size_t start, double value, bool lat) {
    char angle [20];
    double absValue = value >= 0.0 ? value : - value;
    int deg = (int) absValue;
    size_t hsIndex = value >= 0.0 ? 0 : 1;
    double min = (absValue - (double) deg) * 60.0;
    auto fmt = lat ? "%02d%06.3f" : "%03d%06.3f";
    sprintf (angle, fmt, deg, min);
    (*this) [start] = angle;
    (*this) [start+1] = (lat ? "NS" : "EW") [hsIndex];
}

void nmea::builder::Sentence::setAsFloat (size_t index, double value) {
    char angle [10];
    sprintf (angle, "%.1f", value);
    (*this) [index] = angle;
}

std::string nmea::builder::Sentence::compose () {
    auto int2hex = [] (uint8_t digit) -> char {
        if (digit < 10) return '0'+ digit;
        if (digit < 16) return 'A'+ digit - 10;
        return '\0';
    };
    std::string result;
    for (auto& field: *this) {
        if (!result.empty ()) result += ',';
        result += field;
    }
    uint8_t crc = result [0];
    for (size_t i = 1; i < result.length(); ++ i) {
        crc ^= (uint8_t) result [i];
    }
    result += '*';
    result += int2hex (crc >> 4);
    result += int2hex (crc & 15);
    result += "\r\n";
    return result;
}

nmea::builder::Sentence *nmea::parse (const char *source) {
    const char *chr = source;
    for (; *chr && *chr != '!' && *chr != '$'; ++ chr);
    if (!*chr && *chr != '!' && *chr != '$') return nullptr;
    auto result = new nmea::builder::Sentence (*chr == '!', "", "");
    std::string field { *chr };
    for (++ chr; *chr; ++ chr) {
        switch (*chr) {
            case ',':
            case '*':
                result->emplace_back (field.c_str ());
                field.clear ();
                break;
            default:
                field += *chr;
        }
    }
    if (!result->empty ()) result->erase (result->begin ());
    return result;
}

std::optional<char> nmea::builder::Sentence::getAsChar (size_t index) {
    std::optional<char> result;
    if (index < size () && !at (index).empty ()) {
        result = at (index).front ();
    }
    return result;
}

std::optional<int> nmea::builder::Sentence::getAsInt (size_t index) {
    std::optional<int> result;
    if (index < size () && !at (index).empty ()) {
        char first = at (index).front ();
        if (isdigit (first) || first == '-') result = std::stoi (at (index));
    }
    return result;
}

std::optional<double> nmea::builder::Sentence::getAsFloat (size_t index) {
    std::optional<double> result;
    if (index < size () && !at (index).empty ()) {
        char first = at (index).front ();
        if (isdigit (first) || first == '-') result = std::stof (at (index));
    }
    return result;
}
