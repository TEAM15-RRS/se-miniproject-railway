#pragma once
#include <string>
#include <vector>

namespace rrs {

constexpr int kMaxAdvanceDays = 120;

std::string trim(const std::string& s);
std::string toLower(std::string s);
bool iequals(const std::string& a, const std::string& b);
// Keeps empty fields: split("a||b",'|') -> {"a","","b"}
std::vector<std::string> split(const std::string& s, char sep);
// Replaces characters that would break the pipe-delimited file format.
std::string sanitizeField(std::string s);

bool isLeapYear(int y);
int daysInMonth(int y, int m);
// Strict "YYYY-MM-DD", real calendar date, year 2000..2100.
bool parseDate(const std::string& s, int& y, int& m, int& d);
// Strict "HH:MM" 24h.
bool parseTime(const std::string& s, int& h, int& m);
long long daysFromCivil(int y, int m, int d);
bool toEpochMinutes(const std::string& date, const std::string& time, long long& out);
long long currentMinutes();  // local time, same unit as Clock
// "" when OK, otherwise a user-facing error message.
std::string validateJourneyDate(const std::string& date, long long nowMinutes);

}  // namespace rrs
