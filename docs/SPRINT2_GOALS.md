# Sprint 2 Goals

## 1. Sprint Objective

Complete the administrative functionality of the Railway Reservation System and improve its reliability, security, and test coverage while ensuring that the implementation follows the SRS requirements.

## 2. Sprint Goals and Scope

### 2.1 Administrative Functionality

- Implement train creation, modification, and deactivation.
- Support fare and seat quota management for different classes.
- Validate administrative inputs and prevent invalid updates.
- Ensure existing bookings retain their original fare when fares are changed.
- Provide booking and revenue reports grouped by train and date.

### 2.2 Backup and Restore

- Implement backup and restore functionality for persistent application data.
- Ensure data integrity during backup and restoration.
- Verify recovery using appropriate test cases.

### 2.3 Security and Admin Logging

- Restrict administrative operations to authenticated admin accounts.
- Verify that passenger accounts cannot access administrative functionality.
- Record administrative actions in `admin_log.txt` with timestamps and relevant details.

### 2.4 Refund Policy Compliance

Implement and verify the cancellation refund policy according to the SRS:

- More than 24 hours before departure: 75% refund.
- Between 12 and 24 hours, inclusive: 50% refund.
- Less than 12 hours before departure: 25% refund.
- After departure: cancellation is rejected.

### 2.5 Testing and Quality Assurance

- Execute and verify the admin unit tests.
- Verify admin access control and action logging.
- Execute the performance test (TC-Perf-01) and verify the search response-time requirement of 1 second or less under the specified conditions.
- Verify backup and restore functionality.
- Implement and execute fuzz testing.
- Complete system acceptance and regression testing.
- Record actual test results, failures, and fixes in the relevant documentation.

## 3. Expected Deliverables

1. Completed administrative train, fare, quota, and reporting functionality.
2. Verified backup and restore functionality.
3. Admin access control and action logging.
4. Refund policy aligned with the SRS.
5. Updated automated test coverage and test traceability.
6. Performance and fuzz testing evidence.
7. System acceptance and regression test results.
8. Updated README and project documentation.

## 4. Definition of Done

A Sprint 2 task is considered complete when:

- Its acceptance criteria are satisfied.
- The implementation follows the relevant SRS requirements.
- Appropriate unit, integration, or system tests are completed.
- Tests pass on the integrated branch.
- Relevant CI checks pass.
- Required documentation and test evidence are updated.
- Changes are reviewed and merged through the project's pull-request workflow.

## 5. Verification Requirements

The following items require execution evidence before being reported as verified:

- Admin unit-test results.
- Admin access-control and action-logging tests.
- Performance test results for TC-Perf-01.
- Backup and restore test results.
- Fuzz-test implementation and execution.
- System acceptance and regression test results.

Test counts and pass/fail totals must be recorded from actual execution on the current integrated branch, not inferred from the number of test cases present in the source code.