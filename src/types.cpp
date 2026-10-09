#include "rrs/types.h"

#include <algorithm>

#include "rrs/util.h"

namespace rrs {

std::string toString(Status s) {
    switch (s) {
        case Status::Confirmed: return "CONFIRMED";
        case Status::RAC: return "RAC";
        case Status::Waitlist: return "WL";
        case Status::Cancelled: return "CANCELLED";
    }
    return "?";
}

bool statusFromString(const std::string& s, Status& out) {
    for (Status c : {Status::Confirmed, Status::RAC, Status::Waitlist, Status::Cancelled}) {
        if (toString(c) == s) { out = c; return true; }
    }
    return false;
}

const ClassInfo* Train::findClass(const std::string& code) const {
    for (const auto& c : classes)
        if (iequals(c.code, code)) return &c;
    return nullptr;
}

long long Database::nextSeq() const {
    long long m = 0;
    for (const auto& t : tickets) m = std::max(m, t.seq);
    return m + 1;
}

User* findUser(Database& db, const std::string& u) {
    for (auto& x : db.users) if (iequals(x.username, u)) return &x;
    return nullptr;
}
const User* findUser(const Database& db, const std::string& u) {
    for (const auto& x : db.users) if (iequals(x.username, u)) return &x;
    return nullptr;
}
const Train* findTrain(const Database& db, int n) {
    for (const auto& t : db.trains) if (t.number == n) return &t;
    return nullptr;
}
Ticket* findTicket(Database& db, const std::string& pnr) {
    for (auto& t : db.tickets) if (t.pnr == pnr) return &t;
    return nullptr;
}
const Ticket* findTicket(const Database& db, const std::string& pnr) {
    for (const auto& t : db.tickets) if (t.pnr == pnr) return &t;
    return nullptr;
}

}  // namespace rrs
