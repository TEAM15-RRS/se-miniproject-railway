#pragma once
#include <string>
#include <vector>

#include "rrs/types.h"

namespace rrs {

struct LoadReport {
    int skippedLines = 0;  // corrupt/unparsable records that were ignored
    std::vector<std::string> warnings;
};

// File persistence (RRS-F-050): users.dat, trains.dat, bookings.dat in `dir`.
// Saves are atomic: write <file>.tmp, then rename over the real file (RRS-NF-003).
class Storage {
public:
    explicit Storage(std::string dir) : dir_(std::move(dir)) {}
    bool load(Database& db, LoadReport* report = nullptr) const;
    bool save(const Database& db, std::string* error = nullptr) const;

    std::string usersPath() const;
    std::string trainsPath() const;
    std::string bookingsPath() const;

private:
    std::string dir_;
};

// Sample trains (+ default admin account) so the CLI is usable on first run.
void seedDatabase(Database& db);

}  // namespace rrs
