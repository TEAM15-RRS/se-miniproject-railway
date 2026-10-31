
#include "rrs/search.h"
#include "rrs/util.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

int main() {
    constexpr int kTotalTrains = 5000;
    constexpr int kMatchingTrains = 500;
    constexpr int kWarmupRuns = 5;
    constexpr int kMeasuredRuns = 30;
    constexpr double kMaximumTimeMs = 1000.0;
    constexpr char kJourneyDate[] = "2026-10-10";

    rrs::Database db;

    db.trains.reserve(kTotalTrains);
    db.tickets.reserve(kMatchingTrains);

    // Generate 5000 trains.
    // Every tenth train matches the Bengaluru-New Delhi route.
    for (int i = 0; i < kTotalTrains; ++i) {
        const bool matchesRoute = (i % 10 == 0);

        rrs::Train train;

        train.number = 10000 + i;
        train.name = "Benchmark Express " + std::to_string(i);
        train.source = matchesRoute ? "Bengaluru" : "Mysuru";
        train.destination = matchesRoute ? "New Delhi" : "Chennai";
        train.departure = "18:00";

        train.classes.push_back({"SL", 100, 10, 20, 500});

        // Add one confirmed booking for every matching train.
        if (matchesRoute) {
            rrs::Ticket ticket;

            ticket.pnr = "BENCH-" + std::to_string(i);
            ticket.seq = i + 1;
            ticket.username = "benchmark";
            ticket.trainNumber = train.number;
            ticket.classCode = "SL";
            ticket.journeyDate = kJourneyDate;

            ticket.passengers.push_back(
                {"Benchmark Passenger", 30, 'M'}
            );

            db.tickets.push_back(std::move(ticket));
        }

        db.trains.push_back(std::move(train));
    }

    // Use a fixed clock to make date validation deterministic.
    long long fixedNow = 0;

    if (!rrs::toEpochMinutes("2026-10-01", "10:00", fixedNow)) {
        std::cerr << "Failed to initialize benchmark clock.\n";
        return 1;
    }

    rrs::SearchService search(
        db,
        [fixedNow]() { return fixedNow; }
    );

    const rrs::SearchQuery query{
        "Bengaluru",
        "New Delhi",
        kJourneyDate,
        "SL"
    };

    // Warm-up runs are excluded from performance measurements.
    for (int i = 0; i < kWarmupRuns; ++i) {
        const auto result = search.search(query);

        if (!result.ok ||
            result.rows.size() != static_cast<size_t>(kMatchingTrains)) {
            std::cerr << "Warm-up search failed or returned unexpected results.\n";
            return 1;
        }
    }

    std::vector<double> executionTimes;
    executionTimes.reserve(kMeasuredRuns);

    // Perform 30 measured searches.
    for (int i = 0; i < kMeasuredRuns; ++i) {
        const auto start = std::chrono::steady_clock::now();

        const auto result = search.search(query);

        const auto end = std::chrono::steady_clock::now();

        if (!result.ok ||
            result.rows.size() != static_cast<size_t>(kMatchingTrains)) {
            std::cerr << "Measured search failed or returned unexpected results.\n";
            return 1;
        }

        const double elapsedMs =
            std::chrono::duration<double, std::milli>(
                end - start
            ).count();

        executionTimes.push_back(elapsedMs);
    }

    // Sort measurements to calculate minimum, median and maximum.
    std::sort(executionTimes.begin(), executionTimes.end());

    const double minimumMs = executionTimes.front();
    const double maximumMs = executionTimes.back();

    const double medianMs =
        (executionTimes[kMeasuredRuns / 2 - 1] +
         executionTimes[kMeasuredRuns / 2]) / 2.0;

    std::cout << std::fixed << std::setprecision(3);

    std::cout << "\n========== PERFORMANCE TEST ==========\n";

    std::cout << "Total trains: " << db.trains.size() << '\n';
    std::cout << "Matching trains: " << kMatchingTrains << '\n';
    std::cout << "Total tickets: " << db.tickets.size() << '\n';

    std::cout << "Warm-up runs: " << kWarmupRuns << '\n';
    std::cout << "Measured runs: " << kMeasuredRuns << '\n';

    std::cout << "\nExecution Time Statistics:\n";
    std::cout << "Minimum: " << minimumMs << " ms\n";
    std::cout << "Median:  " << medianMs << " ms\n";
    std::cout << "Maximum: " << maximumMs << " ms\n";

    std::cout << "\nRequired maximum: "
              << kMaximumTimeMs << " ms\n";

    std::cout << "Logical processors: "
              << std::thread::hardware_concurrency() << '\n';

#ifdef NDEBUG
    std::cout << "Build configuration: Release\n";
#else
    std::cout << "Build configuration: Non-Release\n";
#endif

    std::cout << "======================================\n";

    if (maximumMs > kMaximumTimeMs) {
        std::cerr << "PERFORMANCE TEST FAILED: "
                  << "At least one search exceeded 1 second.\n";
        return 1;
    }

    std::cout << "PERFORMANCE TEST PASSED\n";

    return 0;
}
