[![CI](https://github.com/TEAM15-RRS/se-miniproject-railway/actions/workflows/ci.yml/badge.svg)](https://github.com/TEAM15-RRS/se-miniproject-railway/actions/workflows/ci.yml)

# Railway Reservation System (Booking & Cancellation)

Software Engineering Mini Project - **Team 15** · C++17 console application

[![CI](https://github.com/<your-org>/<your-repo>/actions/workflows/ci.yml/badge.svg)](https://github.com/<your-org>/<your-repo>/actions)

## Features implemented in Sprint 1
| Area | SRS IDs |
|---|---|
| Registration, passenger login, admin login, lockout after 5 failures, hashed passwords | RRS-F-001..003, RRS-SR-001/003/005 |
| Train search + seat availability (AVAILABLE / RAC / WL / REGRET) | RRS-F-010..012 |
| Booking: validation, PNR, seat decrement, RAC/Waitlist, simulated payment, e-ticket | RRS-F-020..025 |
| Cancellation by PNR, refund slabs, waitlist/RAC auto-promotion, history kept | RRS-F-030..033 |
| PNR status lookup | RRS-F-034 |
| File persistence with atomic writes | RRS-F-050, RRS-NF-003 |

Admin train/fare/quota management, reporting, backup/restore and the admin log are planned for **Sprint 2**.

## Build & run
```bash
# Option A - CMake (what CI uses)
cmake -S . -B build && cmake --build build
./build/rrs            # data files go to ./data

# Option B - plain make (no CMake needed)
make test              # builds + runs all tests
./build-make/rrs
```
Default admin (dev only): `admin` / `Admin@123`.
Test card numbers: `4111111111111111` (success), `4000000000000002` (declined).

## Tests
```bash
ctest --test-dir build --output-on-failure     # or: make test
```
* `tests/unit/` - 65 unit tests (auth, search, payment, booking, cancel, storage, utils)
* `tests/integration/` - 12 integration tests (module interaction through real files)
* Test names carry the STP IDs (e.g. `TC_Book_04_RacThenWaitlistThenRejected`) - see `docs/TEST_TRACEABILITY.md`.

## Layout
```
include/rrs/   public headers          src/        module implementations + main.cpp (CLI)
tests/unit     unit tests              tests/integration   integration tests
docs/          sprint plan, traceability, SAD, demo script     .github/workflows/ci.yml   CI/CD
```

## Branching & commits
* `main` = always green & demo-able. Work on `feature/<module>-<name>`, open a Pull Request, CI must pass, one teammate reviews.
* Commit style: `feat(booking): add RAC queuing (#7)`, `test(cancel): refund slab boundaries (#12)`, `ci: add valgrind job`.
