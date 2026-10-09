// Integration tests: several modules working together, using real files on disk.
#include <fstream>
#include <sstream>

#include "test_helpers.h"

// Re-creates a full "application session" from the files in `dir` (what happens on program restart).
struct Session {
    Database db;
    Storage st;
    long long now;
    Clock clock;
    AuthService auth;
    PaymentService pay;
    BookingService booking;
    CancelService cancel;
    SearchService search;
    explicit Session(const std::string& dir, long long t = M("2026-10-01", "10:00"))
        : st(dir), now(t), clock([this] { return now; }), auth(db, clock), booking(db, pay, clock),
          cancel(db, clock), search(db, clock) {
        st.load(db);
    }
    void save() { CHECK(st.save(db)); }
};

static std::string seedDir(const char* tag) {
    std::string dir = freshTempDir(tag);
    Env e;  // gives us the small test train + alice + bob
    Storage(dir).save(e.db);
    return dir;
}

static std::string slurp(const std::string& path) {
    std::ifstream in(path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

TEST(IT_01_RegisterSaveReloadLogin) {  // Auth <-> Storage
    std::string dir = seedDir("it01");
    {
        Session s(dir);
        CHECK(s.auth.registerUser("dave_01", "Str0ngPass").ok);
        s.save();
    }
    Session s2(dir);
    CHECK(s2.auth.login("dave_01", "Str0ngPass").ok);
    CHECK(!s2.auth.login("dave_01", "Wr0ngPass").ok);
}
TEST(IT_02_PasswordNeverWrittenInPlaintext) {  // Auth <-> Storage (RRS-SR-001)
    std::string dir = seedDir("it02");
    Session s(dir);
    s.auth.registerUser("erin_77", "Unique-Pass-Phrase9");
    s.save();
    std::string users = slurp(Storage(dir).usersPath());
    CHECK(users.find("Unique-Pass-Phrase9") == std::string::npos);
    CHECK(users.find("erin_77") != std::string::npos);
}
TEST(IT_03_LockoutSurvivesRestart) {  // Auth <-> Storage (RRS-SR-005)
    std::string dir = seedDir("it03");
    {
        Session s(dir);
        for (int i = 0; i < 5; ++i) s.auth.login("alice", "bad");
        s.save();
    }
    Session s2(dir);  // simulates closing and reopening the program to dodge the lock
    auto r = s2.auth.login("alice", "Passw0rd1");
    CHECK(!r.ok);
    CHECK_CONTAINS(r.message, "locked");
    s2.now += AuthService::kLockMinutes;
    CHECK(s2.auth.login("alice", "Passw0rd1").ok);
}
TEST(IT_04_BookingPersistsAndSearchReflectsIt) {  // Search <-> Booking <-> Storage
    std::string dir = seedDir("it04");
    {
        Session s(dir);
        CHECK(s.booking.book(req("alice", "SL", 3)).ok);
        s.save();
    }
    Session s2(dir);
    CHECK_EQ(s2.db.tickets.size(), size_t(1));
    auto r = s2.search.search({"Bengaluru", "New Delhi", kDate, "SL"});
    CHECK_EQ(r.rows[0].confirmedFree, 1);
}
TEST(IT_05_NoPnrCollisionAfterReload) {  // STP risk: "PNR collisions after file reload"
    std::string dir = seedDir("it05");
    std::vector<std::string> pnrs;
    for (int round = 0; round < 3; ++round) {
        Session s(dir);
        auto b = s.booking.book(req("alice", "SL", 1, kGoodCard, round == 2 ? "2026-10-12" : kDate));
        CHECK(b.ok);
        pnrs.push_back(b.ticket.pnr);
        s.save();
    }
    CHECK(pnrs[0] != pnrs[1] && pnrs[1] != pnrs[2] && pnrs[0] != pnrs[2]);
}
TEST(IT_06_CancelledPnrNotReusedAfterReload) {
    std::string dir = seedDir("it06");
    std::string first;
    {
        Session s(dir);
        first = s.booking.book(req("alice", "SL", 1)).ticket.pnr;
        s.cancel.cancel(first, "alice");
        s.save();
    }
    Session s2(dir);
    auto b = s2.booking.book(req("bob", "SL", 1));
    CHECK(b.ticket.pnr != first);
    CHECK(lookupPnr(s2.db, first)->status == Status::Cancelled);  // history retained
}
TEST(IT_07_WaitlistPromotionAcrossRestart) {  // Booking <-> Cancel <-> Storage
    std::string dir = seedDir("it07");
    std::string a, rac1;
    {
        Session s(dir);
        a = s.booking.book(req("alice", "SL", 4)).ticket.pnr;
        rac1 = s.booking.book(req("bob", "SL", 1)).ticket.pnr;
        CHECK(lookupPnr(s.db, rac1)->status == Status::RAC);
        s.save();
    }
    Session s2(dir);
    auto r = s2.cancel.cancel(a, "alice");
    CHECK(r.ok);
    CHECK_EQ(r.promotedPnrs.size(), size_t(1));
    s2.save();
    Session s3(dir);
    CHECK(lookupPnr(s3.db, rac1)->status == Status::Confirmed);
}
TEST(IT_08_FailedPaymentLeavesSeatsAndFilesUntouched) {  // Booking <-> Payment
    std::string dir = seedDir("it08");
    Session s(dir);
    CHECK(!s.booking.book(req("alice", "SL", 2, kDeclineCard)).ok);
    s.save();
    Session s2(dir);
    CHECK(s2.db.tickets.empty());
    CHECK_EQ(s2.search.search({"Bengaluru", "New Delhi", kDate, "SL"}).rows[0].confirmedFree, 4);
}
TEST(IT_09_EndToEnd_Register_Search_Book_Cancel_Refund) {  // full user journey
    std::string dir = seedDir("it09");
    Session s(dir);
    CHECK(s.auth.registerUser("frank_5", "Tr@inLover5").ok);
    CHECK(s.auth.login("frank_5", "Tr@inLover5").ok);

    auto found = s.search.search({"bengaluru", "new delhi", kDate, "3A"});
    CHECK_EQ(found.rows.size(), size_t(1));
    CHECK_EQ(found.rows[0].availability, std::string("AVAILABLE 2"));

    auto b = s.booking.book(req("frank_5", "3A", 2));
    CHECK(b.ok);
    CHECK_EQ(b.ticket.totalFare, 2000);
    CHECK_EQ(s.search.search({"bengaluru", "new delhi", kDate, "3A"}).rows[0].availability, std::string("RAC 1"));

    s.now = M(kDate, "19:20") - 24 * 60;  // cancel 24 h before departure -> 75 %
    auto c = s.cancel.cancel(b.ticket.pnr, "frank_5");
    CHECK(c.ok);
    CHECK_EQ(c.refund, 1500);
    s.save();

    Session after(dir, s.now);
    const Ticket* t = lookupPnr(after.db, b.ticket.pnr);
    CHECK(t != nullptr && t->status == Status::Cancelled && t->refundAmount == 1500);
    CHECK_CONTAINS(formatReceipt(after.db, *t), "Refund    : Rs.1500");
    CHECK_EQ(after.search.search({"bengaluru", "new delhi", kDate, "3A"}).rows[0].availability, std::string("AVAILABLE 2"));
}
TEST(IT_10_AbruptTerminationLeavesDataReadable) {  // Storage (RRS-NF-003)
    std::string dir = seedDir("it10");
    {
        Session s(dir);
        s.booking.book(req("alice", "SL", 1));
        s.save();
    }
    // Simulate a crash in the middle of the *next* save: a half-written .tmp file is left behind.
    { std::ofstream t(Storage(dir).bookingsPath() + ".tmp"); t << "1000000099|99|alice|126"; }
    Session s2(dir);
    CHECK_EQ(s2.db.tickets.size(), size_t(1));       // real file untouched, temp ignored
    CHECK(s2.booking.book(req("bob", "SL", 1)).ok);  // and the system keeps working
    s2.save();                                       // next save simply overwrites the stale temp
    Session s3(dir);
    CHECK_EQ(s3.db.tickets.size(), size_t(2));
}
TEST(IT_11_ForeignUserCannotCancel) {  // Auth/ownership <-> Cancel
    std::string dir = seedDir("it11");
    Session s(dir);
    auto b = s.booking.book(req("alice", "SL", 1));
    CHECK(!s.cancel.cancel(b.ticket.pnr, "bob").ok);
    CHECK(lookupPnr(s.db, b.ticket.pnr)->status == Status::Confirmed);
}
TEST(IT_12_CorruptRecordDoesNotBlockStartup) {  // Storage <-> everything (RRS-NF-002)
    std::string dir = seedDir("it12");
    {
        Session s(dir);
        s.booking.book(req("alice", "SL", 1));
        s.save();
        std::ofstream out(Storage(dir).bookingsPath(), std::ios::app);
        out << "#### corrupted ####\n";
    }
    Session s2(dir);
    CHECK_EQ(s2.db.tickets.size(), size_t(1));
    CHECK(s2.booking.book(req("bob", "SL", 1)).ok);
}
