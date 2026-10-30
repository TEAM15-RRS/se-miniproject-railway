
# Performance Testing – Train Search

## 1. Objective

The objective of this performance test is to evaluate the
execution time of SearchService::search() when searching
a database containing 5,000 trains.

Acceptance criterion:
A search operation must complete within 1 second (1000 ms).

## 2. Test Environment

The benchmark was executed using Google Colab.

| Parameter | Specification |
|---|---|
| Environment | Google Colab |
| Operating System | Ubuntu 24.04.5 LTS |
| CPU | Intel(R) Xeon(R) CPU @ 2.20GHz |
| Logical Processors | 2 |
| Total Memory | 12 GiB |
| Available Memory at Inspection | 11 GiB |
| Compiler | GCC 13.3.0 |
| Language Standard | C++17 |
| Build Configuration | Release |
| Build Systems | CMake and GNU Make |

## 3. Test Dataset

A synthetic database containing 5,000 train records
was generated for this benchmark.

| Dataset Parameter | Value |
|---|---:|
| Total trains | 5,000 |
| Matching trains | 500 |
| Non-matching trains | 4,500 |
| Total tickets | 500 |
| Classes per train | 1 |

Each train has one SL class.

Every tenth train matches the search route
(Bengaluru to New Delhi).

The remaining trains have a different route.

One confirmed ticket was created for every matching train
to include seat availability calculations in the benchmark.

## 4. Test Methodology

1. Generate a database containing 5,000 trains.
2. Populate the database with 500 ticket records.
3. Initialize SearchService with the generated database.
4. Use a fixed clock to ensure deterministic journey-date validation.
5. Execute SearchService::search() for Bengaluru to New Delhi,
   with class SL.
6. Perform five warm-up searches.
7. Execute 30 measured searches.
8. Measure execution time using std::chrono::steady_clock.
9. Verify that every search returns the expected 500 results.
10. Calculate minimum, median and maximum execution times.
11. Compare the maximum observed execution time against
    the 1,000 ms acceptance threshold.

Database generation and initialization are performed outside
the measured search operation.

The measured interval covers the actual search operation,
including route filtering and seat availability calculations.

## 5. Performance Results

### 5.1 CMake Release Benchmark

| Metric | Observed Result |
|---|---:|
| Total trains | 5,000 |
| Matching results | 500 |
| Warm-up runs | 5 |
| Measured runs | 30 |
| Minimum execution time | 0.783 ms |
| Median execution time | 0.819 ms |
| Maximum execution time | 0.948 ms |
| Maximum permitted time | 1,000 ms |
| Test status | PASS |

### 5.2 Makefile Release Benchmark

| Metric | Observed Result |
|---|---:|
| Warm-up runs | 5 |
| Measured runs | 30 |
| Minimum execution time | 1.002 ms |
| Median execution time | 1.779 ms |
| Maximum execution time | 3.734 ms |
| Maximum permitted time | 1,000 ms |
| Test status | PASS |

Both build configurations successfully satisfied
the 1-second search performance requirement.

## 6. Acceptance Criteria Verification

| Acceptance Criterion | Result |
|---|---|
| Database contains 5,000 trains | PASS |
| SearchService::search() is measured | PASS |
| Search completes within 1 second | PASS |
| Expected search results are verified | PASS |
| Multiple measurements are performed | PASS |
| Machine specifications documented | PASS |
| Performance results documented | PASS |

## 7. Regression Test Results

The existing project tests were executed alongside
the new performance benchmark.

| Test Suite | Result |
|---|---|
| Unit tests | 122 passed, 0 failed |
| Integration tests | 12 passed, 0 failed |
| Performance benchmark | Passed |
| CMake/CTest | 3 of 3 test targets passed |

No existing test failures were observed.

## 8. Conclusion

The performance test successfully evaluated the
SearchService::search() operation using a synthetic
database containing 5,000 trains.

The CMake Release benchmark recorded a median execution
time of 0.819 ms and a maximum execution time of 0.948 ms
across 30 measured searches.

The Makefile Release benchmark also satisfied the
performance requirement.

Both results are comfortably below the permitted
1,000 ms threshold.

Therefore, the search performance acceptance criterion
is satisfied in the tested Google Colab environment.

Performance measurements may vary depending on hardware,
system load and execution environment.
