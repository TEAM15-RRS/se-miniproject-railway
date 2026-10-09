#include "rrs/auth.h"

#include <cctype>
#include <random>

#include "rrs/sha256.h"
#include "rrs/util.h"

namespace rrs {

static std::string randomSalt() {
    static std::random_device rd;
    static const char* hex = "0123456789abcdef";
    std::string s;
    for (int i = 0; i < 16; ++i) s.push_back(hex[rd() % 16]);
    return s;
}

User makeUser(const std::string& username, const std::string& password, bool isAdmin) {
    User u;
    u.username = username;
    u.salt = randomSalt();
    u.passwordHash = sha256Hex(u.salt + password);
    u.isAdmin = isAdmin;
    return u;
}

std::string AuthService::validateUsername(const std::string& u) {
    if (u.size() < 3 || u.size() > 20) return "Username must be 3-20 characters.";
    for (char c : u)
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return "Username may contain only letters, digits and underscore.";
    return "";
}

std::string AuthService::validatePassword(const std::string& p) {
    if (p.size() < 8 || p.size() > 64) return "Password must be 8-64 characters.";
    bool letter = false, digit = false;
    for (char c : p) {
        if (std::isspace(static_cast<unsigned char>(c))) return "Password must not contain spaces.";
        if (std::isalpha(static_cast<unsigned char>(c))) letter = true;
        if (std::isdigit(static_cast<unsigned char>(c))) digit = true;
    }
    if (!letter || !digit) return "Password must contain at least one letter and one digit.";
    return "";
}

AuthResult AuthService::registerUser(const std::string& username, const std::string& password) {
    std::string err = validateUsername(username);
    if (err.empty()) err = validatePassword(password);
    if (!err.empty()) return {false, err};
    if (findUser(db_, username)) return {false, "Username already taken."};
    db_.users.push_back(makeUser(username, password, false));
    return {true, "Registration successful."};
}

AuthResult AuthService::login(const std::string& username, const std::string& password, bool adminLogin) {
    const std::string generic = "Invalid username or password.";  // no user enumeration
    User* u = findUser(db_, username);
    if (!u) return {false, generic};

    const long long now = clock_();
    if (u->lockedUntil > now) {
        long long left = u->lockedUntil - now;
        return {false, "Account locked due to repeated failed logins. Try again in " +
                           std::to_string(left) + " minute(s)."};
    }

    if (sha256Hex(u->salt + password) != u->passwordHash) {
        if (++u->failedAttempts >= kMaxFailures) {
            u->failedAttempts = 0;
            u->lockedUntil = now + kLockMinutes;
            return {false, "Account locked for " + std::to_string(kLockMinutes) +
                               " minutes after " + std::to_string(kMaxFailures) + " failed attempts."};
        }
        return {false, generic};
    }

    u->failedAttempts = 0;
    u->lockedUntil = 0;
    if (adminLogin && !u->isAdmin) return {false, "Access denied: administrator account required."};
    return {true, "Login successful."};
}

}  // namespace rrs
