#include "rrs/util.h"

#include <cctype>
#include <ctime>

namespace rrs {

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool iequals(const std::string& a, const std::string& b) { return toLower(a) == toLower(b); }

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    size_t start = 0;
    while (true) {
        size_t p = s.find(sep, start);
        if (p == std::string::npos) { out.push_back(s.substr(start)); break; }
        out.push_back(s.substr(start, p - start));
        start = p + 1;
    }
    return out;
}

std::string sanitizeField(std::string s) {
    for (auto& c : s)
        if (c == '|' || c == '\n' || c == '\r' || c == ';' || c == ':') c = ' ';
    return s;
}

bool isLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int daysInMonth(int y, int m) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeapYear(y)) ? 29 : d[m - 1];
}

static bool allDigits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

bool parseDate(const std::string& s, int& y, int& m, int& d) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    std::string ys = s.substr(0, 4), ms = s.substr(5, 2), ds = s.substr(8, 2);
    if (!allDigits(ys) || !allDigits(ms) || !allDigits(ds)) return false;
    int yy = std::stoi(ys), mm = std::stoi(ms), dd = std::stoi(ds);
    if (yy < 2000 || yy > 2100 || mm < 1 || mm > 12 || dd < 1 || dd > daysInMonth(yy, mm)) return false;
    y = yy; m = mm; d = dd;
    return true;
}

bool parseTime(const std::string& s, int& h, int& m) {
    if (s.size() != 5 || s[2] != ':') return false;
    std::string hs = s.substr(0, 2), ms = s.substr(3, 2);
    if (!allDigits(hs) || !allDigits(ms)) return false;
    int hh = std::stoi(hs), mm = std::stoi(ms);
    if (hh > 23 || mm > 59) return false;
    h = hh; m = mm;
    return true;
}

long long daysFromCivil(int y, int m, int d) {  // days since 1970-01-01
    y -= m <= 2;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const long long yoe = y - era * 400;
    const long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

bool toEpochMinutes(const std::string& date, const std::string& time, long long& out) {
    int y, mo, d, h, mi;
    if (!parseDate(date, y, mo, d) || !parseTime(time, h, mi)) return false;
    out = daysFromCivil(y, mo, d) * 1440 + h * 60 + mi;
    return true;
}

long long currentMinutes() {
    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);
    return daysFromCivil(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday) * 1440 +
           lt->tm_hour * 60 + lt->tm_min;
}

std::string validateJourneyDate(const std::string& date, long long nowMinutes) {
    int y, m, d;
    if (!parseDate(date, y, m, d)) return "Invalid date. Use YYYY-MM-DD.";
    long long day = daysFromCivil(y, m, d);
    long long today = nowMinutes / 1440;
    if (day < today) return "Journey date is in the past.";
    if (day - today > kMaxAdvanceDays)
        return "Bookings open only " + std::to_string(kMaxAdvanceDays) + " days in advance.";
    return "";
}

}  // namespace rrs
