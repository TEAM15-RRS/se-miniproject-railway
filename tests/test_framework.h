#pragma once
// Minimal dependency-free unit-test framework (works offline, in CI and on student laptops).
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tf {
struct Case { const char* name; void (*fn)(); };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int& failures() { static int f = 0; return f; }
struct Registrar { Registrar(const char* n, void (*f)()) { registry().push_back({n, f}); } };
template <typename A, typename B>
inline void checkEq(const A& a, const B& b, const char* ea, const char* eb, const char* file, int line) {
    if (!(a == b)) {
        std::ostringstream o;
        o << "  FAIL " << file << ":" << line << "  " << ea << " == " << eb << "  (got [" << a << "] vs [" << b << "])";
        std::cout << o.str() << "\n";
        ++failures();
    }
}
inline bool contains(const std::string& h, const std::string& n) { return h.find(n) != std::string::npos; }
inline int runAll() {
    int failedCases = 0;
    for (const auto& c : registry()) {
        int before = failures();
        c.fn();
        bool ok = failures() == before;
        std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << c.name << "\n";
        if (!ok) ++failedCases;
    }
    std::cout << "\n" << registry().size() << " tests, " << failedCases << " failed\n";
    return failedCases == 0 ? 0 : 1;
}
}  // namespace tf

#define TEST(name) \
    static void name(); \
    static ::tf::Registrar reg_##name(#name, name); \
    static void name()
#define CHECK(cond) \
    do { if (!(cond)) { std::cout << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; ++::tf::failures(); } } while (0)
#define CHECK_EQ(a, b) ::tf::checkEq((a), (b), #a, #b, __FILE__, __LINE__)
#define CHECK_CONTAINS(hay, needle) \
    do { if (!::tf::contains((hay), (needle))) { std::cout << "  FAIL " << __FILE__ << ":" << __LINE__ << "  [" << (hay) << "] lacks [" << (needle) << "]\n"; ++::tf::failures(); } } while (0)
