#include <fstream>

#include "test_helpers.h"

TEST(TC_Store_01_RoundTripPreservesEverything) {  // RRS-F-050
    Env e;
    bookN(e, "alice", "SL", 2);
    auto c = bookN(e, "bob", "3A", 1);
    e.cancel.cancel(c.ticket.pnr, "bob");
    e.auth.login("alice", "wrong");  // failedAttempts = 1

    std::string dir = freshTempDir("rt");
    Storage st(dir);
    CHECK(st.save(e.db));
    Database loaded;
    LoadReport rep;
    CHECK(st.load(loaded, &rep));
    CHECK_EQ(rep.skippedLines, 0);
    CHECK_EQ(loaded.users.size(), e.db.users.size());
    CHECK_EQ(loaded.trains.size(), size_t(1));
    CHECK_EQ(loaded.trains[0].classes.size(), size_t(2));
    CHECK_EQ(loaded.trains[0].classes[0].fare, 500);
    CHECK_EQ(loaded.tickets.size(), size_t(2));
    CHECK_EQ(loaded.tickets[0].passengers.size(), size_t(2));
    CHECK_EQ(loaded.tickets[0].passengers[1].name, std::string("Passenger B"));
    CHECK(loaded.tickets[1].status == Status::Cancelled);
    CHECK_EQ(loaded.tickets[1].refundAmount, e.db.tickets[1].refundAmount);
    CHECK_EQ(findUser(loaded, "alice")->failedAttempts, 1);
    CHECK_EQ(findUser(loaded, "alice")->passwordHash, findUser(e.db, "alice")->passwordHash);
}
TEST(TC_Store_02_MissingFilesLoadAsEmpty) {
    Storage st(freshTempDir("empty"));
    Database db;
    LoadReport rep;
    CHECK(st.load(db, &rep));
    CHECK(db.users.empty() && db.trains.empty() && db.tickets.empty());
    CHECK_EQ(rep.skippedLines, 0);
}
TEST(TC_Store_03_CorruptLinesSkippedValidKept) {  // RRS-NF-002/003
    Env e;
    bookN(e, "alice", "SL", 1);
    bookN(e, "bob", "SL", 1);
    std::string dir = freshTempDir("corrupt");
    Storage st(dir);
    st.save(e.db);
    {
        std::ofstream out(st.bookingsPath(), std::ios::app);
        out << "garbage line without separators\n";
        out << "1|2|x|notanumber|SL|2026-10-10|CONFIRMED|1|1|0|0|A,30,M\n";
        out << "\n";
    }
    Database db;
    LoadReport rep;
    CHECK(st.load(db, &rep));
    CHECK_EQ(db.tickets.size(), size_t(2));
    CHECK_EQ(rep.skippedLines, 2);
}
TEST(TC_Store_04_SaveLeavesNoTempFiles) {
    Env e;
    std::string dir = freshTempDir("tmp");
    Storage st(dir);
    CHECK(st.save(e.db));
    for (const auto& f : std::filesystem::directory_iterator(dir))
        CHECK(f.path().extension() != ".tmp");
}
TEST(TC_Store_05_DelimiterCharactersCannotCorruptFiles) {
    Env e;
    e.db.trains[0].name = "Bad|Name;With:Chars";
    std::string dir = freshTempDir("delim");
    Storage st(dir);
    st.save(e.db);
    Database db;
    LoadReport rep;
    st.load(db, &rep);
    CHECK_EQ(rep.skippedLines, 0);
    CHECK_EQ(db.trains.size(), size_t(1));
}
TEST(TC_Store_06_SeedDataIsLoadable) {
    Database db;
    seedDatabase(db);
    CHECK(db.trains.size() >= 3);
    CHECK(findUser(db, "admin") != nullptr && findUser(db, "admin")->isAdmin);
    Storage st(freshTempDir("seed"));
    CHECK(st.save(db));
    Database back;
    st.load(back);
    CHECK_EQ(back.trains.size(), db.trains.size());
}
