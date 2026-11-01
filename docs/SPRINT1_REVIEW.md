# Sprint 1 Review

## 1. Sprint Overview

**Project:** Railway Reservation System
**Sprint:** Sprint 1
**Team:** Team 15

### Sprint Goal

Develop a working console-based Railway Reservation System supporting passenger registration, login, train search, ticket booking, RAC/WL handling, cancellation, refunds, and persistent storage.

## 2. Sprint Scope

The planned Sprint 1 scope included:

- Passenger registration and authentication.
- Train search and seat availability.
- Ticket booking and simulated payment.
- PNR generation and booking receipts.
- RAC and waiting-list handling.
- Ticket cancellation and refund processing.
- Persistent storage of users, trains, and bookings.
- Unit and integration tests.
- Continuous integration through GitHub Actions.

Admin train management, fare and quota management, reports, backup/restore, admin action logging, performance testing, and fuzz testing were deferred to Sprint 2.

## 3. Implementation Review

The project includes the core passenger-facing functionality, booking and cancellation services, file-based persistence, and a console interface.

The implementation also includes unit and integration test cases covering authentication, search, booking, cancellation, security, and storage.

The repository identifies 140 unit test cases and 12 integration test cases. These are source-level test counts and should not be interpreted as confirmed passing execution totals.

## 4. Testing and Quality Review

Test cases are mapped to SRS requirements in `docs/TEST_TRACEABILITY.md`.

The following areas have automated test mappings:

- Registration and authentication.
- Train search and availability.
- Booking, payment, and PNR generation.
- Cancellation, refunds, and waiting-list promotion.
- Data persistence.
- Security-related requirements.

Actual test execution totals and CI results must be confirmed from the relevant branch and GitHub Actions run before being reported as final results.

## 5. Deferred Work and Known Limitations

The following items were carried forward to Sprint 2:

- Admin train management.
- Fare and quota management.
- Booking and revenue reports.
- Backup and restore.
- Admin action logging.
- Performance testing with the specified dataset.
- Fuzz testing and further system/acceptance testing.

The Sprint 1 plan also identified single-process operation and the absence of file locking as limitations.

The refund policy initially documented in the Sprint 1 plan was an assumption and required correction against the SRS. The SRS policy must be treated as authoritative.

## 6. Retrospective

### What Went Well
- The core passenger reservation workflow was established.
- The project has separate modules for major system functionalities.
- Unit and integration test cases were created and mapped to SRS requirements.
- A GitHub Actions CI workflow was established.

### Areas for Improvement
- Some implementation assumptions required correction against the SRS, particularly the refund policy.
- Additional verification was needed for administrative functionality, security, and data recovery.
- Performance, fuzz, and comprehensive system testing remained outstanding.

### Lessons Learned
- Implementation decisions should be validated against the SRS before being treated as final.
- Test-case creation and successful test execution must be tracked separately.
- Integration and regression testing are necessary before declaring the system complete.

## 7. Sprint 1 Review Summary

Sprint 1 established the core passenger reservation workflow and the supporting storage and testing structure.

The main follow-up areas were administrative functionality, additional quality and security verification, performance testing, and alignment of implementation assumptions with the SRS.

Test execution results and final CI status remain subject to verification/

## 8. Action Items for Sprint 2

1. Complete and verify administrative functionality.
2. Verify fare and quota validation against the SRS.
3. Verify backup/restore and admin action logging.
4. Execute performance and fuzz tests and record evidence.
5. Complete system/acceptance and regression testing.
6. Update test traceability and project documentation.
7. Verify the refund policy against the SRS.