#include <fstream>
#include <sstream>

#include "test_helpers.h"

// Owner: Samaresh - extra Storage tests (Sprint 1)

TEST(TC_Store_07_SaveCreatesMissingFolder) {
    std::string base = freshTempDir("nested");
    std::string dir = base + "/a/b/c";
    Env e;
    Storage st(dir);
    CHECK(st.save(e.db));
    CHECK(std::filesystem::exists(st.usersPath()));
}
TEST(TC_Store_08_WindowsLineEndingsAreAccepted) {
    Env e;
    std::string dir = freshTempDir("crlf");
    Storage st(dir);
    st.save(e.db);
    std::ifstream in(st.trainsPath());
    std::stringstream ss; ss << in.rdbuf(); in.close();
    std::string text, line;
    std::stringstream lines(ss.str());
    while (std::getline(lines, line)) text += line + "\r\n";
    { std::ofstream out(st.trainsPath(), std::ios::binary | std::ios::trunc); out << text; }
    Database db; LoadReport rep;
    st.load(db, &rep);
    CHECK_EQ(db.trains.size(), size_t(1));
    CHECK_EQ(rep.skippedLines, 0);
}
TEST(TC_Store_09_InactiveTrainFlagPersists) {
    Env e;
    e.db.trains[0].active = false;
    Storage st(freshTempDir("inactive"));
    st.save(e.db);
    Database db;
    st.load(db);
    CHECK(!db.trains[0].active);
}
TEST(TC_Store_10_SavingTwiceKeepsDataIdentical) {
    Env e;
    bookN(e, "alice", "SL", 2);
    Storage st(freshTempDir("twice"));
    st.save(e.db);
    Database a; st.load(a);
    st.save(a);
    Database b; st.load(b);
    CHECK_EQ(b.tickets.size(), a.tickets.size());
    CHECK_EQ(b.tickets[0].pnr, a.tickets[0].pnr);
    CHECK_EQ(b.users.size(), a.users.size());
}
