# Sprint 1 Review - demo script (about 8 minutes)

1. **GitHub (1 min)** - Issues board with closed Sprint 1 issues; Insights -> Contributors (all 4 members); PR list with reviews.
2. **CI (1 min)** - Actions tab: green run on `main`. Show the three jobs (build/test on Ubuntu+Windows, sanitizers, valgrind).
   *Live trick:* push a commit that breaks one assertion -> red run -> fix -> green. Shows the pipeline really guards the code.
3. **Run the tests (1 min)** - `make test` or `ctest --output-on-failure` -> 77 tests passing. Open `docs/TEST_TRACEABILITY.md`.
4. **Live demo (4 min)** - `./build/rrs`
   * Register `demo_user` (type the password - point out it is hidden) -> log in
   * Search `Bengaluru -> New Delhi`, a date next month -> see AVAILABLE / RAC / WL hints
   * Book 2 passengers in SL with card `4111111111111111` -> e-ticket with PNR
   * Try card `4000000000000002` -> payment declined, nothing booked
   * Cancel the ticket -> refund shown; check "PNR status" -> CANCELLED retained
   * Wrong password 5 times -> account locked
   * `cat data/users.dat` -> hashes only, no plain passwords
5. **Wrap-up (1 min)** - what is deferred to Sprint 2 (admin console, backup/restore, admin log, performance test).

# Sprint 2 Review - Demo Script (About 8 Minutes)

1. **GitHub and CI (1 min)**
   - Show the GitHub Issues board and Sprint 2 progress.
   - Show pull requests and review status.
   - Open GitHub Actions and demonstrate successful build and test runs.

2. **Run Automated Tests (1 min)**
   - Run `cmake --build build`.
   - Run `ctest --test-dir build --output-on-failure`.
   - Show the unit, integration and search performance test results.
   - Open `docs/TEST_TRACEABILITY.md`.

3. **Passenger Demo (2 min)**
   - Run `./build/rrs`.
   - Register a passenger account and log in.
   - Search for trains between Bengaluru and New Delhi.
   - Book a ticket and demonstrate payment processing.
   - Check PNR status and demonstrate ticket cancellation.

4. **Admin Console Demo (3 min)**
   - Log in using the seeded development admin account.
   - List trains and display their classes, fares and quotas.
   - Add a train and demonstrate input validation.
   - Modify train details.
   - Deactivate a train and verify its inactive status.
   - Update class fare and quotas.
   - Demonstrate `CNCL` cancellation during Add Train.
   - Demonstrate invalid and duplicate class-code rejection.

5. **Backup and Restore (1 min)**
   - Create a backup and show its directory.
   - Restore the backup.
   - Demonstrate that the admin session is logged out after restoration.
   - Log in again and verify the restored train details and status.

6. **Wrap-up**
   - Summarize completed Sprint 2 features.
   - Mention remaining integration work and pending pull requests.