#pragma once
// Core domain types shared by every module (SRS: data model).
#include <functional>
#include <string>
#include <vector>

namespace rrs {

// Clock returns "minutes since 1970-01-01 (local civil time)". Injected so that
// tests can control time (refund slabs, lockout cool-down, departure checks).
using Clock = std::function<long long()>;

enum class Status { Confirmed, RAC, Waitlist, Cancelled };
std::string toString(Status s);
bool statusFromString(const std::string& s, Status& out);

struct User {
    std::string username;
    std::string salt;
    std::string passwordHash;  // SHA-256(salt + password) - never plaintext (RRS-SR-001)
    bool isAdmin = false;
    int failedAttempts = 0;        // consecutive failures (RRS-SR-005)
    long long lockedUntil = 0;     // clock minutes; 0 = not locked
};

struct ClassInfo {
    std::string code;   // SL, 3A, 2A, CC ...
    int totalSeats = 0;
    int racSeats = 0;
    int wlLimit = 0;
    int fare = 0;       // per passenger, whole rupees
};

struct Train {
    int number = 0;
    std::string name, source, destination;
    std::string departure;  // "HH:MM"
    std::vector<ClassInfo> classes;
    bool active = true;
    const ClassInfo* findClass(const std::string& code) const;
};

struct Passenger {
    std::string name;
    int age = 0;
    char gender = 'M';  // M / F / O
};

struct Ticket {
    std::string pnr;
    long long seq = 0;  // booking order; drives RAC/WL queue order
    std::string username;
    int trainNumber = 0;
    std::string classCode;
    std::string journeyDate;  // YYYY-MM-DD
    std::vector<Passenger> passengers;
    Status status = Status::Confirmed;
    int totalFare = 0;
    long long bookedAt = 0;
    int refundAmount = 0;
    long long cancelledAt = 0;
};

struct Database {
    std::vector<User> users;
    std::vector<Train> trains;
    std::vector<Ticket> tickets;  // cancelled tickets are retained (RRS-F-033)
    long long nextSeq() const;    // max(seq)+1 -> PNRs never repeat, even after reload
};

User* findUser(Database& db, const std::string& username);
const User* findUser(const Database& db, const std::string& username);
const Train* findTrain(const Database& db, int number);
Ticket* findTicket(Database& db, const std::string& pnr);
const Ticket* findTicket(const Database& db, const std::string& pnr);

}  // namespace rrs
