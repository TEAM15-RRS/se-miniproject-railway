#pragma once
#include <string>
#include <vector>

#include "rrs/types.h"

namespace rrs {

struct SearchQuery {
    std::string source, destination, journeyDate;
    std::string classCode;  // optional filter
};

struct AvailabilityRow {
    int trainNumber = 0;
    std::string trainName, source, destination, departure, classCode;
    int fare = 0;
    int confirmedFree = 0, racFree = 0, wlFree = 0;
    std::string availability;  // "AVAILABLE 12" | "RAC 3" | "WL 5" | "REGRET" (RRS-F-012 hint)
};

struct SearchResult {
    bool ok = false;
    std::string message;
    std::vector<AvailabilityRow> rows;
};

class SearchService {
public:
    SearchService(const Database& db, Clock clock) : db_(db), clock_(std::move(clock)) {}
    SearchResult search(const SearchQuery& q) const;  // RRS-F-010, F-011, F-012

private:
    const Database& db_;
    Clock clock_;
};

}  // namespace rrs
