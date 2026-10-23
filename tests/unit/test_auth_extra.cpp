#include "test_helpers.h"

// Owner: Manapriya - extra Auth tests (Sprint 1)

TEST(TC_Auth_08_LockoutIsPerUser) {
    Env e;
    for (int i = 0; i < 5; ++i) e.auth.login("alice", "wrong");
    CHECK(!e.auth.login("alice", "Passw0rd1").ok);   // alice is locked
    CHECK(e.auth.login("bob", "Passw0rd2").ok);      // bob is not affected
}
TEST(TC_Auth_09_PasswordOfExactly64CharsAccepted) {
    Env e;
    std::string pw = std::string(63, 'a') + "1";
    CHECK_EQ(pw.size(), size_t(64));
    CHECK(e.auth.registerUser("longpw_user", pw).ok);
    CHECK(e.auth.login("longpw_user", pw).ok);
}
TEST(TC_Auth_10_FailedLoginOnUnknownUserCreatesNothing) {
    Env e;
    size_t before = e.db.users.size();
    for (int i = 0; i < 6; ++i) e.auth.login("ghost_user", "Whatever1");
    CHECK_EQ(e.db.users.size(), before);
}
TEST(TC_Auth_11_AdminLoginAttemptByPassengerDoesNotLockAccount) {
    Env e;
    for (int i = 0; i < 6; ++i) CHECK(!e.auth.login("alice", "Passw0rd1", true).ok);
    CHECK(e.auth.login("alice", "Passw0rd1").ok);    // normal passenger login still works
}
