#!/usr/bin/env bash
# Creates the Sprint 1 labels, milestone and issues on GitHub using the GitHub CLI.
#   1) install gh: https://cli.github.com   2) gh auth login   3) run from the repo folder
set -e
gh label create sprint-1 --color 0E8A16 --force
gh label create feature  --color 1D76DB --force
gh label create testing  --color FBCA04 --force
gh label create ci-cd    --color 5319E7 --force
gh label create docs     --color C5DEF5 --force
gh api repos/:owner/:repo/milestones -f title="Sprint 1" -f description="Auth, search, booking, cancellation, persistence, tests, CI" >/dev/null || true

issue() { gh issue create --title "$1" --label "$2" --milestone "Sprint 1" --body "$3"; }

issue "[F] Passenger registration & login (RRS-F-001/002)" "feature,sprint-1" "Validation rules, SHA-256+salt hashing. Tests: TC-Reg-01..04, TC-Auth-01..03"
issue "[F] Admin login & access check (RRS-F-003, SR-003)" "feature,sprint-1" "Admin flag, 'access denied' for non-admins. Test: TC-Auth-04"
issue "[F] Login lockout after 5 failures (RRS-SR-005)" "feature,sprint-1" "15-minute cool-down, persisted. Tests: TC-Sec-05..07, IT-03"
issue "[F] Train search & availability (RRS-F-010..012)" "feature,sprint-1" "Case-insensitive search, date window, AVAILABLE/RAC/WL/REGRET hint. Tests: TC-Search-01..07"
issue "[F] Ticket booking, PNR, RAC/WL (RRS-F-020..023)" "feature,sprint-1" "All-or-nothing group status, unique PNR. Tests: TC-Book-01..10"
issue "[F] Simulated payment + e-ticket receipt (RRS-F-024/025)" "feature,sprint-1" "Luhn check, declined test card, receipt. Tests: UT_Pay_*, TC-Book-11,13"
issue "[F] Cancellation, refund slabs, WL promotion (RRS-F-030..033)" "feature,sprint-1" "Slabs 48h/12h/4h, FIFO promotion. Tests: TC-Cancel-01..11"
issue "[F] PNR status lookup (RRS-F-034)" "feature,sprint-1" "Status + queue position. Tests: TC-Book-14,15"
issue "[F] File persistence with atomic save (RRS-F-050, NF-003)" "feature,sprint-1" "users.dat / trains.dat / bookings.dat. Tests: TC-Store-*, IT-10, IT-12"
issue "[F] Console UI (menus, masked password/card input)" "feature,sprint-1" "RRS-SR-002, RRS-NF-004"
issue "[T] Unit test suite (65 tests)" "testing,sprint-1" "One file per module under tests/unit"
issue "[T] Integration test suite (12 tests)" "testing,sprint-1" "tests/integration - modules + real files"
issue "[CI] GitHub Actions: build + test on push/PR" "ci-cd,sprint-1" "Ubuntu + Windows, sanitizers, valgrind"
issue "[D] SRS/STP/SAD alignment and Sprint 1 report" "docs,sprint-1" "Keep docs/ in sync with code"
