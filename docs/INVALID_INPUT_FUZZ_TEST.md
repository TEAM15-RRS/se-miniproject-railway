
# Invalid-Input and Fuzz Testing Report

## 1. Objective

The objective of this testing activity is to evaluate the robustness
of the Railway Reservation System when its services receive
malformed, unexpected, extreme and randomized inputs.

The testing covers five service areas:
- Registration
- Train search
- Ticket booking
- Ticket cancellation
- Administration

## 2. Testing Methodology

Two approaches were used.

### 2.1 Handcrafted Invalid Inputs

Inputs were deliberately created to test validation boundaries.

The following categories were covered:

- Empty strings and whitespace.
- Very long strings (up to 10,000 characters).
- Special characters and symbols.
- Invalid dates and malformed strings.
- Negative and extreme numeric values.
- Invalid passenger details.
- Invalid train numbers and class codes.
- Invalid PNRs and requester information.
- Invalid fare and quota values.

### 2.2 Randomized Fuzz Testing

Randomized inputs were generated using the C++ standard
random number generator (std::mt19937).

Fixed random seeds were used to ensure reproducibility.

Thirty iterations were executed for each service area,
generating different malformed inputs for the tested operations.

## 3. Test Coverage

| Service | Invalid Inputs Tested |
|---|---|
| Registration | Empty usernames, invalid characters, oversized usernames and invalid passwords |
| Search | Empty routes, identical stations, malformed dates, oversized strings and randomized routes |
| Booking | Invalid usernames, extreme train numbers, invalid classes, dates, passenger details and payment information |
| Cancellation | Empty, nonexistent and oversized PNRs, invalid requesters and randomized inputs |
| Administration | Invalid train details, departure times, train numbers, fares and quota values |

## 4. Expected Behaviour

The tests verify that:

1. Invalid registration requests are rejected.
2. Invalid searches are handled without application failure.
3. Invalid booking requests are rejected without creating tickets.
4. Invalid cancellation requests do not alter existing tickets.
5. Invalid administrative requests are rejected without adding invalid trains.
6. Rejected operations return a non-empty explanatory message.
7. The application handles the tested inputs without crashing.

A syntactically valid search with no matching trains is treated
as a successful search with an explanatory no-results message.

## 5. Test Results

The following test suites were executed:

| Test Suite | Passed | Failed |
|---|---:|---:|
| Unit tests | 127 | 0 |
| Integration tests | 12 | 0 |
| Total | 139 | 0 |

All five newly added fuzz-test cases passed.

CTest reported:

100% tests passed, 0 tests failed out of 2.

The tests completed without observed crashes or hangs.

## 6. Defect Reporting

No application defects were identified during these test executions.

Therefore, no additional bug issues were required as a result
of this testing activity.

Any defects discovered during subsequent testing will be
reported through separate GitHub Issues with the bug label.

## 7. Conclusion

The invalid-input and fuzz testing activity successfully
covered registration, search, booking, cancellation and
administrative operations.

Both handcrafted and reproducible randomized inputs
were executed.

All 127 unit tests and 12 integration tests passed.

No crashes, hangs or unexpected database modifications
were observed during the executed tests.

The tested invalid-input scenarios were handled
successfully by the application.
