
# Regression Testing Report – Administrative Changes

## 1. Objective

The objective of regression testing is to verify that existing
ticket cancellation, refund calculation and waitlist promotion
functionality continues to work correctly after administrative
changes to train fares, train status and seat quotas.

## 2. Scope

The testing covers three scenarios:

1. Refund calculation after a fare change.
2. Ticket cancellation after train deactivation.
3. Waitlist promotion after RAC quota modification.

## 3. Test Scenarios

### 3.1 Refund After Fare Change

Procedure:
1. Book a ticket for two passengers at the original fare of Rs.500 per passenger.
2. Change the class fare to Rs.900 using AdminService.
3. Cancel the previously booked ticket.
4. Verify that the original ticket fare remains unchanged.
5. Verify that the refund is calculated using the original fare.

Expected result:

The original ticket fare should remain Rs.1,000.
When cancelled 48 hours before departure, the refund should be Rs.880
after deducting Rs.60 per passenger.

### 3.2 Refund After Train Deactivation

Procedure:
1. Book a ticket for two passengers.
2. Deactivate the train using AdminService.
3. Attempt to cancel the previously booked ticket.
4. Verify that cancellation succeeds.
5. Verify that the original refund policy is applied.

Expected result:

The existing ticket should remain cancellable even after train
deactivation. The refund should be calculated using the original
ticket fare and applicable cancellation rules.

### 3.3 Waitlist Promotion After Quota Change

Procedure:
1. Fill the confirmed, RAC and waitlist capacities of a train.
2. Increase the RAC quota using AdminService.
3. Cancel an existing confirmed ticket.
4. Verify that the earliest RAC ticket is promoted to confirmed.
5. Verify that eligible waitlisted tickets are promoted to RAC.

Expected result:

The cancellation operation should trigger waitlist promotion
using the updated RAC quota while maintaining FIFO ordering.

## 4. Test Environment

| Parameter | Specification |
|---|---|
| Language | C++17 |
| Testing framework | Project's custom unit testing framework |
| Build system | CMake and GNU Make |
| Test file | tests/unit/test_cancel.cpp |
| Test execution | Google Colab |

## 5. Test Results

To be updated after executing the regression tests.

| Test Case | Description | Result |
|---|---|---|
| TC_Cancel_15 | Refund after fare change | PASS |
| TC_Cancel_16 | Refund after train deactivation | PASS |
| TC_Cancel_17 | Waitlist promotion after quota change | PASS |

## 6. Conclusion

The regression tests are designed to verify that administrative
configuration changes do not adversely affect existing tickets,
refund calculations or waitlist promotion.

All three newly added regression tests passed successfully.

The complete unit test suite reported 125 passed tests and
zero failures. CTest also reported 100% tests passed,
with both unit and integration test targets passing.

No regression failures were observed.
