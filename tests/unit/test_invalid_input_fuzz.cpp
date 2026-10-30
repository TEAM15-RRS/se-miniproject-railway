
#include "test_helpers.h"
#include "rrs/admin.h"

#include <climits>
#include <random>
#include <string>
#include <vector>

namespace {

template <typename Result>
void expectRejected(const Result& result) {
    CHECK(!result.ok);
    CHECK(!result.message.empty());
}

std::string randomSymbols(std::mt19937& rng, size_t length) {
    static const std::string characters =
        "!@#$%^&*()[]{}?/\\|~+=<>";

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        result.push_back(characters[rng() % characters.size()]);
    }

    return result;
}

}  // namespace


// ============================================================
// 1. REGISTRATION INVALID-INPUT TESTING
// ============================================================

TEST(TC_Fuzz_01_RegistrationRejectsMalformedInputs) {
    Env e;

    const size_t originalUsers = e.db.users.size();

    const std::vector<std::string> badUsernames = {
        "", " ", "ab", "bad name", "!@#$%",
        std::string(10000, 'u')
    };

    for (const auto& username : badUsernames) {
        expectRejected(e.auth.registerUser(username, "Valid123"));
    }

    const std::vector<std::string> badPasswords = {
        "", "short1", "abcdefgh", "12345678",
        "Pass word1", "!@#$%^&*",
        std::string(65, 'A'),
        std::string(10000, '1')
    };

    for (size_t i = 0; i < badPasswords.size(); ++i) {
        auto username = "fuzzpw" + std::to_string(i);
        expectRejected(e.auth.registerUser(username, badPasswords[i]));
    }

    std::mt19937 rng(0x22F022);

    for (int i = 0; i < 30; ++i) {
        std::string username =
            "!" + randomSymbols(rng, 5 + rng() % 30);

        expectRejected(e.auth.registerUser(username, "Valid123"));

        username = "random" + std::to_string(i);
        std::string password =
            randomSymbols(rng, 8 + rng() % 30);

        expectRejected(e.auth.registerUser(username, password));
    }

    CHECK_EQ(e.db.users.size(), originalUsers);
}


// ============================================================
// 2. SEARCH INVALID-INPUT TESTING
// ============================================================

TEST(TC_Fuzz_02_SearchRejectsMalformedInputs) {
    Env e;

    const std::vector<SearchQuery> badQueries = {
        {"", "New Delhi", kDate, ""},
        {"Bengaluru", "", kDate, ""},
        {"", "", kDate, ""},
        {"Bengaluru", "Bengaluru", kDate, ""},
        {"Bengaluru", "New Delhi", "", ""},
        {"Bengaluru", "New Delhi", "31/10/2026", ""},
        {"Bengaluru", "New Delhi", "2026-99-99", ""},
        {"Bengaluru", "New Delhi", std::string(10000, '9'), ""}
    };

    for (const auto& query : badQueries) {
        expectRejected(e.search.search(query));
    }

    // A very long class filter is handled as a no-result search.
    auto result = e.search.search({
        "Bengaluru", "New Delhi", kDate, std::string(10000, '!')
    });

    CHECK(result.ok);
    CHECK(result.rows.empty());
    CHECK(!result.message.empty());

    // Very long and symbolic route inputs.
    result = e.search.search({
        std::string(10000, '!'),
        std::string(10000, '?'),
        kDate,
        ""
    });

    CHECK(result.ok);
    CHECK(result.rows.empty());
    CHECK(!result.message.empty());

    std::mt19937 rng(0x220022);

    for (int i = 0; i < 30; ++i) {
        std::string source =
            "SRC!" + randomSymbols(rng, 10 + rng() % 30);

        std::string destination =
            "DST!" + randomSymbols(rng, 10 + rng() % 30);

        auto searchResult = e.search.search({
            source, destination, kDate, ""
        });

        CHECK(searchResult.ok);
        CHECK(searchResult.rows.empty());
        CHECK(!searchResult.message.empty());

        searchResult = e.search.search({
            "Bengaluru",
            "New Delhi",
            randomSymbols(rng, 10 + rng() % 30),
            ""
        });

        expectRejected(searchResult);
    }
}


// ============================================================
// 3. BOOKING INVALID-INPUT TESTING
// ============================================================

TEST(TC_Fuzz_03_BookingRejectsMalformedInputs) {
    Env e;

    const size_t originalTickets = e.db.tickets.size();

    auto checkInvalidBooking = [&](const BookingRequest& request) {
        auto result = e.booking.book(request);

        expectRejected(result);
        CHECK_EQ(e.db.tickets.size(), originalTickets);
    };

    checkInvalidBooking(req("", "SL", 1));
    checkInvalidBooking(req("unknown_user", "SL", 1));
    checkInvalidBooking(req(std::string(10000, '!'), "SL", 1));

    auto request = req("alice", "SL", 1);
    request.trainNumber = INT_MIN;
    checkInvalidBooking(request);

    request.trainNumber = INT_MAX;
    checkInvalidBooking(request);

    request = req("alice", "", 1);
    checkInvalidBooking(request);

    request = req("alice", std::string(10000, 'S'), 1);
    checkInvalidBooking(request);

    request = req("alice", "SL", 1);
    request.journeyDate = "";
    checkInvalidBooking(request);

    request.journeyDate = "99/99/9999";
    checkInvalidBooking(request);

    request.journeyDate = std::string(10000, '9');
    checkInvalidBooking(request);

    checkInvalidBooking(req("alice", "SL", 0));
    checkInvalidBooking(req("alice", "SL", 7));

    request = req("alice", "SL", 1);
    request.passengers[0].name = "";
    checkInvalidBooking(request);

    request.passengers[0].name = std::string(10000, 'A');
    checkInvalidBooking(request);

    request = req("alice", "SL", 1);
    request.passengers[0].name = "!@#$%^&*";
    checkInvalidBooking(request);

    request = req("alice", "SL", 1);
    request.passengers[0].age = INT_MIN;
    checkInvalidBooking(request);

    request = req("alice", "SL", 1);
    request.passengers[0].age = INT_MAX;
    checkInvalidBooking(request);

    request = req("alice", "SL", 1);
    request.passengers[0].gender = '?';
    checkInvalidBooking(request);

    request = req("alice", "SL", 1);
    request.cardNumber = "invalid-card";
    checkInvalidBooking(request);

    request.cardNumber = std::string(10000, '9');
    checkInvalidBooking(request);

    std::mt19937 rng(0x220033);

    for (int i = 0; i < 30; ++i) {
        request = req(
            "!" + randomSymbols(rng, 10 + rng() % 30),
            "SL", 1
        );
        checkInvalidBooking(request);

        request = req(
            "alice",
            randomSymbols(rng, 10 + rng() % 30),
            1
        );
        checkInvalidBooking(request);
    }
}


// ============================================================
// 4. CANCELLATION INVALID-INPUT TESTING
// ============================================================

TEST(TC_Fuzz_04_CancellationRejectsMalformedInputs) {
    Env e;

    auto booking = bookN(e, "alice", "SL", 1);

    CHECK(booking.ok);

    if (!booking.ok) return;

    const std::string validPnr = booking.ticket.pnr;

    auto checkInvalidCancellation = [&](const std::string& pnr,
                                        const std::string& requester) {
        auto result = e.cancel.cancel(pnr, requester);

        expectRejected(result);

        CHECK(e.db.tickets.front().status == Status::Confirmed);
    };

    checkInvalidCancellation("", "alice");
    checkInvalidCancellation("!@#$%^&*", "alice");
    checkInvalidCancellation(std::string(10000, 'X'), "alice");

    checkInvalidCancellation(validPnr, "");
    checkInvalidCancellation(validPnr, std::string(10000, '!'));
    checkInvalidCancellation(validPnr, "!@#$%^&*");

    std::mt19937 rng(0x220044);

    for (int i = 0; i < 30; ++i) {
        checkInvalidCancellation(
            randomSymbols(rng, 10 + rng() % 30), "alice"
        );

        checkInvalidCancellation(
            validPnr, randomSymbols(rng, 10 + rng() % 30)
        );
    }

    CHECK_EQ(e.db.tickets.size(), size_t(1));
}


// ============================================================
// 5. ADMIN INVALID-INPUT TESTING
// ============================================================

TEST(TC_Fuzz_05_AdminRejectsMalformedInputs) {
    Env e;

    AdminService admin(e.db, e.clock);

    const size_t originalTrains = e.db.trains.size();

    auto checkInvalidTrain = [&](const Train& train) {
        auto result = admin.addTrain(train);

        expectRejected(result);
        CHECK_EQ(e.db.trains.size(), originalTrains);
    };

    Train badTrain = e.db.trains.front();
    badTrain.number = 13000;

    badTrain.number = 0;
    checkInvalidTrain(badTrain);

    badTrain.number = INT_MIN;
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13001;
    badTrain.name = "";
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13002;
    badTrain.source = "";
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13003;
    badTrain.destination = "";
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13004;
    badTrain.destination = badTrain.source;
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13005;
    badTrain.departure = std::string(10000, '9');
    checkInvalidTrain(badTrain);

    // Corrected: use a named random generator.
    std::mt19937 adminRng(0x220055);

    badTrain.departure = randomSymbols(adminRng, 10000);
    badTrain.number = 13006;
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13007;
    badTrain.classes.clear();
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13008;
    badTrain.classes[0].totalSeats = 0;
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13009;
    badTrain.classes[0].racSeats = -1;
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13010;
    badTrain.classes[0].wlLimit = -1;
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13011;
    badTrain.classes[0].fare = 0;
    checkInvalidTrain(badTrain);

    badTrain = e.db.trains.front();
    badTrain.number = 13012;
    badTrain.classes[0].racSeats = INT_MAX;
    checkInvalidTrain(badTrain);

    auto result = admin.modifyTrain(INT_MIN, e.db.trains.front());
    expectRejected(result);

    Train updatedTrain = e.db.trains.front();
    updatedTrain.number = INT_MAX;

    result = admin.modifyTrain(12627, updatedTrain);
    expectRejected(result);

    updatedTrain = e.db.trains.front();
    updatedTrain.departure = std::string(10000, '!');
    result = admin.modifyTrain(12627, updatedTrain);
    expectRejected(result);

    expectRejected(admin.deactivateTrain(INT_MIN));
    expectRejected(admin.deactivateTrain(INT_MAX));

    auto checkInvalidQuota = [&](int fare, int seats, int rac, int wl,
                                 const std::string& classCode = "SL",
                                 int trainNumber = 12627) {
        auto update = admin.updateClassFareAndQuota(
            trainNumber, classCode, fare, seats, rac, wl
        );

        expectRejected(update);
    };

    checkInvalidQuota(INT_MIN, 4, 2, 2);
    checkInvalidQuota(500, INT_MIN, 2, 2);
    checkInvalidQuota(500, 4, INT_MAX, 2);
    checkInvalidQuota(500, 4, 2, INT_MIN);
    checkInvalidQuota(500, 4, 2, 2, std::string(10000, '!'));
    checkInvalidQuota(500, 4, 2, 2, "SL", INT_MIN);

    std::mt19937 rng(0x220066);

    for (int i = 0; i < 30; ++i) {
        int invalidNumber = -1 - static_cast<int>(rng() % 100000);
        expectRejected(admin.deactivateTrain(invalidNumber));

        int invalidFare = -1 - static_cast<int>(rng() % 100000);
        checkInvalidQuota(invalidFare, 4, 2, 2);
    }

    CHECK_EQ(e.db.trains.size(), originalTrains);
}
