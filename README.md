[![CI](https://github.com/TEAM15-RRS/se-miniproject-railway/actions/workflows/ci.yml/badge.svg)](https://github.com/TEAM15-RRS/se-miniproject-railway/actions/workflows/ci.yml)

# Railway Reservation System (Booking & Cancellation)

Software Engineering Mini Project - **Team 15** · C++17 console application

## Features implemented

### Passenger features

| Feature | SRS IDs |
|---|---|
| Registration, passenger login, admin login, lockout after 5 failures, hashed passwords | RRS-F-001..003, RRS-SR-001/003/005 |
| Train search and seat availability (AVAILABLE / RAC / WL / REGRET) | RRS-F-010..012 |
| Booking: validation, PNR generation, seat decrement, RAC/Waitlist, simulated payment, e-ticket | RRS-F-020..025 |
| Cancellation by PNR, refund slabs, waitlist/RAC auto-promotion, booking history | RRS-F-030..033 |
| PNR status lookup | RRS-F-034 |

### Admin features

- List, add, modify and deactivate trains.
- Update class fares and quotas, with validation against existing bookings.
- Create data backups and restore application data from backups.
- Admin actions are recorded in an activity log.
- Train-date reports can be generated through the admin service; they are not currently exposed in the CLI menu.

### Storage

- File-based persistence for users, trains and bookings.
- Atomic file writes to reduce the risk of corrupted data.
- Backup and restore support.

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

- `tests/unit/` - 140 unit test cases.
- `tests/integration/` - 12 integration test cases.
- Test names use test-case identifiers (e.g. TC_Book_04_RacThenWaitlistThenRejected) - see docs/TEST_TRACEABILITY.md.

## Layout

```text
include/rrs/   public headers          src/        module implementations + main.cpp (CLI)
tests/unit     unit tests              tests/integration   integration tests
docs/          sprint plans, sprint reviews, sprint goals, test reports,
               traceability, SAD, demo script
```

## Branching & commits

- `main` = always green & demo-able. Work on `feature/<module>-<name>`, open a Pull Request, CI must pass, one teammate reviews.
- Commit style: `feat(booking): add RAC queuing (#7)`, `test(cancel): refund slab boundaries (#12)`, `ci: add valgrind job`.