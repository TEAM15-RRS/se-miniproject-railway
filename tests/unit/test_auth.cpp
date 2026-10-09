#include "test_helpers.h"

// ---- Registration (RRS-F-001) ----
TEST(TC_Reg_01_ValidRegistration) {
    Env e;
    auto r = e.auth.registerUser("charlie_9", "Secret123");
    CHECK(r.ok);
    CHECK(findUser(e.db, "charlie_9") != nullptr);
}
TEST(TC_Reg_02_DuplicateUsernameCaseInsensitive) {
    Env e;
    auto r = e.auth.registerUser("ALICE", "Another123");
    CHECK(!r.ok);
    CHECK_CONTAINS(r.message, "taken");
}
TEST(TC_Reg_03_UsernameBoundaries) {
    Env e;
    CHECK(!e.auth.registerUser("ab", "Secret123").ok);                       // 2 chars
    CHECK(e.auth.registerUser("abc", "Secret123").ok);                       // 3 chars
    CHECK(e.auth.registerUser(std::string(20, 'x'), "Secret123").ok);        // 20 chars
    CHECK(!e.auth.registerUser(std::string(21, 'y'), "Secret123").ok);       // 21 chars
    CHECK(!e.auth.registerUser("bad name", "Secret123").ok);                 // space
    CHECK(!e.auth.registerUser("bad|name", "Secret123").ok);                 // file delimiter
}
TEST(TC_Reg_04_PasswordRules) {
    Env e;
    CHECK(!e.auth.registerUser("u1user", "short1").ok);           // < 8
    CHECK(!e.auth.registerUser("u2user", "allletters").ok);       // no digit
    CHECK(!e.auth.registerUser("u3user", "12345678").ok);         // no letter
    CHECK(!e.auth.registerUser("u4user", "has space1").ok);       // whitespace
    CHECK(!e.auth.registerUser("u5user", "Exactlyy").ok);   // 8 chars but no digit
    CHECK(e.auth.registerUser("u6user", "Exact8ch").ok);          // boundary 8, valid
    CHECK(!e.auth.registerUser("u7user", std::string(65, 'a') + "1").ok);  // > 64
}
// ---- Login (RRS-F-002) ----
TEST(TC_Auth_01_ValidLogin) {
    Env e;
    CHECK(e.auth.login("alice", "Passw0rd1").ok);
    CHECK(e.auth.login("ALICE", "Passw0rd1").ok);  // username is case-insensitive
}
TEST(TC_Auth_02_WrongPasswordAndUnknownUserGiveSameMessage) {
    Env e;
    auto a = e.auth.login("alice", "wrongPass1");
    auto b = e.auth.login("nobody", "Passw0rd1");
    CHECK(!a.ok && !b.ok);
    CHECK_EQ(a.message, b.message);  // no user enumeration
}
TEST(TC_Auth_03_PasswordIsCaseSensitive) {
    Env e;
    CHECK(!e.auth.login("alice", "passw0rd1").ok);
}
// ---- Admin login (RRS-F-003, RRS-SR-003) ----
TEST(TC_Auth_04_AdminLoginRequiresAdminAccount) {
    Env e;
    e.db.users.push_back(makeUser("root", "Adm1nPass", true));
    CHECK(e.auth.login("root", "Adm1nPass", true).ok);
    auto r = e.auth.login("alice", "Passw0rd1", true);  // correct password, not an admin
    CHECK(!r.ok);
    CHECK_CONTAINS(r.message, "denied");
}
// ---- Security (RRS-SR-001, SR-005) ----
TEST(TC_Sec_01_PasswordStoredHashedNotPlaintext) {
    Env e;
    const User* u = findUser(e.db, "alice");
    CHECK(u->passwordHash != "Passw0rd1");
    CHECK_EQ(u->passwordHash.size(), size_t(64));
    CHECK(u->salt.size() == 16);
}
TEST(TC_Sec_02_SamePasswordDifferentSalts) {
    Env e;
    e.auth.registerUser("carol", "Passw0rd1");  // same password as alice
    CHECK(findUser(e.db, "carol")->passwordHash != findUser(e.db, "alice")->passwordHash);
}
TEST(TC_Sec_05_LockoutAfterFiveFailures) {
    Env e;
    for (int i = 1; i <= 4; ++i) {
        auto r = e.auth.login("alice", "badPass" + std::to_string(i));
        CHECK(!r.ok);
        CHECK(!tf::contains(r.message, "locked"));
    }
    auto fifth = e.auth.login("alice", "badPass5");
    CHECK_CONTAINS(fifth.message, "locked");
    // even the CORRECT password is refused while locked
    auto r = e.auth.login("alice", "Passw0rd1");
    CHECK(!r.ok);
    CHECK_CONTAINS(r.message, "locked");
}
TEST(TC_Sec_06_LockoutReleasesAfterCooldown) {
    Env e;
    for (int i = 0; i < 5; ++i) e.auth.login("alice", "wrong");
    e.now += AuthService::kLockMinutes - 1;
    CHECK(!e.auth.login("alice", "Passw0rd1").ok);   // 14 min: still locked
    e.now += 1;
    CHECK(e.auth.login("alice", "Passw0rd1").ok);    // 15 min: released
}
TEST(TC_Sec_07_SuccessfulLoginResetsFailureCounter) {
    Env e;
    for (int i = 0; i < 4; ++i) e.auth.login("alice", "wrong");
    CHECK(e.auth.login("alice", "Passw0rd1").ok);
    for (int i = 0; i < 4; ++i) CHECK(!tf::contains(e.auth.login("alice", "wrong").message, "locked"));
}
