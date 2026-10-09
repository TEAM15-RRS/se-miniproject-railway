#pragma once
#include <string>

#include "rrs/types.h"

namespace rrs {

struct AuthResult {
    bool ok = false;
    std::string message;
};

// Creates a user record with a random salt and SHA-256(salt+password) hash.
User makeUser(const std::string& username, const std::string& password, bool isAdmin);

class AuthService {
public:
    static constexpr int kMaxFailures = 5;      // RRS-SR-005
    static constexpr int kLockMinutes = 15;     // cool-down period

    AuthService(Database& db, Clock clock) : db_(db), clock_(std::move(clock)) {}

    AuthResult registerUser(const std::string& username, const std::string& password);
    // adminLogin=true additionally requires the account to be an admin (RRS-F-003, RRS-SR-003).
    AuthResult login(const std::string& username, const std::string& password, bool adminLogin = false);

    static std::string validateUsername(const std::string& u);   // "" = valid
    static std::string validatePassword(const std::string& p);   // "" = valid

private:
    Database& db_;
    Clock clock_;
};

}  // namespace rrs
