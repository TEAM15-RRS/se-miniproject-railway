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
