# Sprint 1 Plan - Railway Reservation System (Team 15)

## 1. Sprint goal
Deliver a working console application that lets a passenger **register, log in, search trains, book a ticket
(with RAC/WL), cancel it and see the refund**, with data persisted to disk - backed by unit tests, integration
tests and a CI pipeline that is green on every push.

## 2. Scope (traced to SRS v1.0)
| In Sprint 1 | Deferred to Sprint 2 |
|---|---|
| RRS-F-001..003 auth | RRS-F-040..043 admin train/fare/quota management, reports |
| RRS-F-010..012 search/availability | RRS-F-051 backup/restore |
| RRS-F-020..025 booking/payment/receipt | RRS-SR-004 admin action log |
| RRS-F-030..034 cancel/refund/promotion/status | RRS-NF-001 performance run with 5,000 trains |
| RRS-F-050 persistence | Fuzz/system/acceptance test passes |
| RRS-SR-001/002/003/005, RRS-NF-002/003 | |

## 3. Design assumptions - CHECK THESE AGAINST YOUR SRS
The SRS itself was not available when this code was written; the items below were my interpretation of the STP.
**If your SRS says something different, change the constant and the matching test (one place each).**

| # | Assumption | Where in code |
|---|---|---|
| A-1 | Max 6 passengers per PNR; all passengers on one PNR share one status (all-or-nothing) | `booking.h` `kMaxPassengersPerBooking` |
| A-2 | Booking opens 120 days ahead; no booking once the train has departed | `util.h` `kMaxAdvanceDays` |
| A-3 | Refund: WL 100 %; >=48h 100 % minus Rs.60/passenger; 12-48h 75 %; 4-12h 50 %; <4h 0 %; none after departure | `cancel.cpp` `calculateRefund` |
| A-4 | Lockout: 5 consecutive failures -> 15 min cool-down (persisted so restart cannot bypass it) | `auth.h` |
| A-5 | Promotion is strictly first-come-first-served; WL cannot jump ahead of waiting RAC | `cancel.cpp` `promote` |
| A-6 | Password: 8-64 chars, >=1 letter and >=1 digit. Username 3-20 of letters/digits/underscore | `auth.cpp` |
| A-7 | Simulated payment: 16 digits + Luhn; `4000000000000002` always declines | `payment.cpp` |
| A-8 | Passenger age 1-120; gender M/F/O | `booking.cpp` |
| A-9 | PNR = 10 digits, derived from a never-reused booking sequence | `booking.cpp` `pnrFromSeq` |

## 4. Module ownership (every member = identifiable commits)
| Member | STP role | Code ownership | Test ownership | Other |
|---|---|---|---|---|
| **Amruta Karoshi** | Developer | `booking`, `payment`, `inventory` | `test_booking.cpp`, payment tests | Fix defects |
| **Samaresh** | Product Owner | `storage`, `main.cpp` (CLI) | `test_storage.cpp`, IT_01-IT_04 | Backlog, sprint review, README |
| **Manapriya Kudurebyalya** | QA Lead | `auth`, `util`, `sha256` | `test_auth.cpp`, `test_util_sha.cpp`, IT_02/03 | **CI/CD pipeline**, traceability doc |
| **Pranav B N** | Test Engineer | `search`, `cancel` | `test_search_payment.cpp` (search), `test_cancel.cpp`, IT_05-IT_12 | Defect log (GitHub Issues) |

Everyone commits under **their own GitHub account**. Do not let one person push all the code - the evaluator
looks at `git shortlog -sn` and the Insights -> Contributors graph.

## 5. Backlog = GitHub Issues
Run `scripts/create_issues.sh` once (needs GitHub CLI) or create the 14 issues manually from that file.
Move cards on a GitHub Project board: *To do -> In progress -> In review -> Done*.

## 6. Definition of Done
Code reviewed via Pull Request, unit tests written, CI green, issue closed with the PR number, doc updated.

## 7. Known limitations (be honest in the review)
* Single user / single process; no file locking.
* Fares and refund policy are assumptions (A-3).
* Admin console is a stub until Sprint 2.
