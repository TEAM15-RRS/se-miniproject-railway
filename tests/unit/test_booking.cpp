#include "test_helpers.h"

TEST(TC_Book_01_ConfirmedBookingBasics) {  // RRS-F-020, F-022
    Env e;
    auto r = bookN(e, "alice", "SL", 2);
    CHECK(r.ok);
    CHECK(r.ticket.status == Status::Confirmed);
    CHECK_EQ(r.ticket.totalFare, 1000);  // 2 x 500
    CHECK_EQ(e.db.tickets.size(), size_t(1));
    CHECK_CONTAINS(r.message, r.ticket.pnr);
}
TEST(TC_Book_02_PnrIsUniqueAnd10Digits) {  // RRS-F-021
    Env e;
    std::string a = bookN(e, "alice", "SL", 1).ticket.pnr;
    std::string b = bookN(e, "bob", "SL", 1).ticket.pnr;
    CHECK(a != b);
    CHECK_EQ(a.size(), size_t(10));
    CHECK_EQ(b.size(), size_t(10));
    CHECK(a.find_first_not_of("0123456789") == std::string::npos);
}
TEST(TC_Book_03_SeatsDecrementAfterBooking) {  // RRS-F-022
    Env e;
    bookN(e, "alice", "SL", 3);
    Seats s = computeSeats(e.db, 12627, *e.db.trains[0].findClass("SL"), kDate);
    CHECK_EQ(s.confirmedFree, 1);
    CHECK_EQ(s.confirmedUsed, 3);
}
TEST(TC_Book_04_RacThenWaitlistThenRejected) {  // RRS-F-023
    Env e;
    for (int i = 0; i < 4; ++i) CHECK(bookN(e, "alice", "SL", 1).ticket.status == Status::Confirmed);
    for (int i = 0; i < 2; ++i) CHECK(bookN(e, "alice", "SL", 1).ticket.status == Status::RAC);
    for (int i = 0; i < 2; ++i) CHECK(bookN(e, "alice", "SL", 1).ticket.status == Status::Waitlist);
    auto full = bookN(e, "alice", "SL", 1);
    CHECK(!full.ok);
    CHECK_CONTAINS(full.message, "No seats");
}
TEST(TC_Book_05_GroupBookingIsAllOrNothing) {
    Env e;
    bookN(e, "alice", "SL", 3);                       // 1 berth left
    auto r = bookN(e, "bob", "SL", 2);                // does not fit berths -> RAC (2 slots)
    CHECK(r.ticket.status == Status::RAC);
    auto w = bookN(e, "bob", "SL", 2);                // RAC full -> WL (2 slots)
    CHECK(w.ticket.status == Status::Waitlist);
    CHECK_EQ(computeSeats(e.db, 12627, *e.db.trains[0].findClass("SL"), kDate).confirmedFree, 1);
}
TEST(TC_Book_06_PassengerCountBoundaries) {
    Env e;
    CHECK(!e.booking.book(req("alice", "SL", 0)).ok);
    CHECK(!e.booking.book(req("alice", "SL", 7)).ok);
    Env e2;
    e2.db.trains[0].classes[0].totalSeats = 6;
    CHECK(e2.booking.book(req("alice", "SL", 6)).ok);  // max allowed
}
TEST(TC_Book_07_PassengerAgeBoundaries) {
    Env e;
    for (int age : {0, -1, 121, 1000}) {
        auto r = e.booking.book([&] { auto q = req("alice", "SL", 1); q.passengers[0].age = age; return q; }());
        CHECK(!r.ok);
        CHECK_CONTAINS(r.message, "age");
    }
    for (int age : {1, 60, 120}) {
        auto q = req("alice", "SL", 1); q.passengers[0].age = age;
        CHECK(e.booking.book(q).ok);
    }
}
TEST(TC_Book_08_PassengerNameAndGenderValidation) {
    Env e;
    for (const char* bad : {"", "   ", "R@vi", "Ravi|Kumar", "Ravi,Kumar", "A1"}) {
        auto q = req("alice", "SL", 1); q.passengers[0].name = bad;
        CHECK(!e.booking.book(q).ok);
    }
    auto q = req("alice", "SL", 1); q.passengers[0].name = std::string(31, 'a');
    CHECK(!e.booking.book(q).ok);
    q.passengers[0].name = "Mary O'Neil Jr.";
    CHECK(e.booking.book(q).ok);
    q = req("alice", "SL", 1); q.passengers[0].gender = 'X';
    CHECK(!e.booking.book(q).ok);
}
TEST(TC_Book_09_InvalidTrainClassUserDate) {
    Env e;
    auto q = req("alice", "SL", 1); q.trainNumber = 99999;
    CHECK(!e.booking.book(q).ok);
    CHECK(!e.booking.book(req("alice", "1A", 1)).ok);                 // class not on train
    CHECK(!e.booking.book(req("ghost", "SL", 1)).ok);                 // unknown user
    CHECK(!e.booking.book(req("alice", "SL", 1, kGoodCard, "2026-09-30")).ok);  // past
    CHECK(!e.booking.book(req("alice", "SL", 1, kGoodCard, "bogus")).ok);
    e.db.trains[0].active = false;
    CHECK(!e.booking.book(req("alice", "SL", 1)).ok);                 // deactivated train
}
TEST(TC_Book_10_CannotBookDepartedTrain) {
    Env e;
    e.now = M(kDate, "19:20");  // exactly departure time
    auto r = e.booking.book(req("alice", "SL", 1));
    CHECK(!r.ok);
    CHECK_CONTAINS(r.message, "departed");
    e.now = M(kDate, "19:19");  // one minute before -> allowed
    CHECK(e.booking.book(req("alice", "SL", 1)).ok);
}
TEST(TC_Book_11_PaymentFailureStoresNothing) {  // RRS-F-024
    Env e;
    CHECK(!e.booking.book(req("alice", "SL", 1, kDeclineCard)).ok);
    CHECK(!e.booking.book(req("alice", "SL", 1, "1234")).ok);
    CHECK(e.db.tickets.empty());
}
TEST(TC_Book_12_TicketsForUserNewestFirst) {
    Env e;
    bookN(e, "alice", "SL", 1); bookN(e, "bob", "SL", 1); bookN(e, "alice", "3A", 1);
    auto v = e.booking.ticketsFor("alice");
    CHECK_EQ(v.size(), size_t(2));
    CHECK(v[0]->seq > v[1]->seq);
}
TEST(TC_Book_13_ReceiptContainsKeyFields) {  // RRS-F-025
    Env e;
    auto r = bookN(e, "alice", "SL", 2);
    std::string rc = formatReceipt(e.db, r.ticket, r.txnId);
    CHECK_CONTAINS(rc, r.ticket.pnr);
    CHECK_CONTAINS(rc, "Karnataka Express");
    CHECK_CONTAINS(rc, "CONFIRMED");
    CHECK_CONTAINS(rc, "Rs.1000");
    CHECK_CONTAINS(rc, r.txnId);
}
TEST(TC_Book_14_PnrLookup) {  // RRS-F-034
    Env e;
    auto r = bookN(e, "alice", "SL", 1);
    CHECK(lookupPnr(e.db, "  " + r.ticket.pnr + " ") != nullptr);
    CHECK(lookupPnr(e.db, "0000000000") == nullptr);
    CHECK(lookupPnr(e.db, "") == nullptr);
}
TEST(TC_Book_15_WaitlistPositionShownInReceipt) {
    Env e;
    for (int i = 0; i < 6; ++i) bookN(e, "alice", "SL", 1);
    auto w1 = bookN(e, "alice", "SL", 1), w2 = bookN(e, "bob", "SL", 1);
    CHECK_EQ(queuePosition(e.db, *lookupPnr(e.db, w1.ticket.pnr)), 1);
    CHECK_EQ(queuePosition(e.db, *lookupPnr(e.db, w2.ticket.pnr)), 2);
    CHECK_CONTAINS(formatReceipt(e.db, *lookupPnr(e.db, w2.ticket.pnr)), "position 2");
}
