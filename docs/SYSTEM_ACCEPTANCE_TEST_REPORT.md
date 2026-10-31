# System & Acceptance Test Execution Report
## Railway Reservation System — Team 15

| Report field | Value |
|---|---|
| Work item | GitHub Issue #23 — System and acceptance test run with defect log |
| Test level | Manual system and acceptance testing through the CLI |
| Test plan | Software Test Plan (STP), Version 1.0, dated 16 September 2026 |
| Requirements baseline | Railway Reservation System SRS, Version 1.0, dated 13 September 2026 |
| Execution environment | Google Colab, Linux, C++17 build (`build/rrs`) |
| Execution date | 31 October 2026 |
| Evidence directory | `docs/evidence/issue-23/` |
| Report file | `docs/SYSTEM_ACCEPTANCE_TEST_REPORT.md` |

---

## 1. Purpose

This report records the manual system and acceptance testing performed for the Railway Reservation System against the STP and the corresponding functional/security requirements and test-case references in the SRS.

The work item acceptance criteria require the tester to:

1. Execute the STP system test cases manually through the command-line interface.
2. Record each test result as pass or fail and retain screenshot evidence in `docs/`.
3. Raise a GitHub Issue for every failure, with the `bug` label and assignment to the relevant module owner.
4. Retest affected test cases after fixes and update the execution log.

This report records the observed results and evidence. Test cases that could not be performed because the current CLI does not expose the required operation are explicitly identified as not executable through the current CLI. They are not represented as passed.

## 2. Source documents and test basis

The test execution was based on:

- `docs/Software_Test_Plan.pdf`
  - Section 2 — Test Items
  - Section 3 — Features to be Tested
  - Section 5 — Test Approach / Strategy, including System Tests
  - Section 5.1 — Security Validation
  - Section 13 — Test Case Management & Traceability
- `docs/15_SRS_MiniProject.pdf`
  - Section 4 — System Features and acceptance criteria
  - Section 5.1 — Security Requirements
  - Section 8 — Requirements Traceability Matrix (RTM)

The STP describes the system-test level as end-to-end CLI execution. The SRS provides the feature-level acceptance criteria and reference test-case IDs used in the execution matrix below.

### Scope applied

This report is limited to the manual CLI system/acceptance checks and the related manual security checks evidenced by the supplied screenshots and the additional CLI runs performed for this work item.

The report does not claim that an item passed merely because its implementation was visible in source code. Source-code and data-file screenshots are identified as supplementary inspection evidence where applicable.

No screenshots are embedded in this Markdown file. All screenshots are referenced by filename and are stored separately under `docs/evidence/issue-23/`.

## 3. Execution environment and test data

The project was executed in Google Colab using the compiled application at:

```text
/content/se-miniproject-railway/build/rrs
```

Separate data directories were used for isolated test runs:

| Test activity | Data directory | Purpose |
|---|---|---|
| Seat-decrement test (TC-Book-03) | `test_data_tc_book03` | Before/after confirmed-seat availability and booking persistence check |
| Earlier system/acceptance screenshots | `test_acceptance_data` and the corresponding run directories shown in the screenshots | Registration, authentication, search, booking, PNR, cancellation and admin checks |
| End-to-end CLI workflow | `test_e2e_issue23` | Fresh registration-to-cancellation workflow |

### Data-directory correction during TC-Data-01

The first restart check was initially launched with `test_acceptance_data`, which did not contain the PNR being checked. The booking record was then located in `test_data_tc_book03/bookings.dat`. The application was restarted using that original data directory, and PNR `1000000003` was successfully retrieved with its original details. The final TC-Data-01 result is based on the corrected, same-directory restart check shown in screenshot `33_tc_data_01_persistence_after_restart.png`.

## 4. Overall execution summary

The SRS RTM contains 27 relevant manual feature/security test-case rows included in this report. The report is limited to the requested manual CLI system/acceptance checks and associated security checks. One additional end-to-end CLI workflow was also executed as required by the STP's system-testing approach.

### RTM test-case result summary

| Result | Count |
|---|---:|
| PASS | 17 |
| PARTIAL — evidence exists, but does not demonstrate the full acceptance condition | 3 |
| FAIL | 3 |
| NOT EXECUTABLE THROUGH CURRENT CLI | 4 |
| **Total RTM cases recorded** | **27** |

### Additional end-to-end workflow

| Test case | Result | Evidence |
|---|---|---|
| E2E-CLI-01 — Register, log in, search, book, look up PNR, cancel and look up the same PNR again | **PASS** | `34_e2e_cli_workflow_part1.png`, `35_e2e_cli_workflow_part2.png` |

The three failures identified during evidence review are:
- TC-Reg-01 — registration does not collect all passenger profile fields required by SRS requirement RRS-F-001.
- TC-Book-01 — booking does not prompt for berth preference, which is required by SRS requirement RRS-F-020.
- TC-Sec-04 — admin action log records an incorrect event timestamp.

The GitHub bug issue references are to be added to the defect register after the corresponding issues have actually been created. This report does not invent issue numbers or claim issue creation before it has occurred.

## 5. Detailed system and acceptance test execution matrix

**Status definitions**

- **PASS:** The available manual test evidence demonstrates the stated acceptance behaviour.
- **PARTIAL:** Some of the expected behaviour is demonstrated, but the available evidence does not cover the complete acceptance condition.
- **FAIL:** The observed CLI behaviour does not meet the referenced requirement.
- **NOT EXECUTABLE THROUGH CURRENT CLI:** The current application menu does not expose the operation needed to execute the test manually.

### 5.1 Registration and authentication

| Test case | Requirement / objective | Manual execution performed / evidence reviewed | Expected result | Actual result | Status | Screenshot evidence |
|---|---|---|---|---|---|---|
| TC-Reg-01 | RRS-F-001 — Register a new passenger with the required profile and credential information. | Open Register from the main menu and inspect the registration prompts and completion message. The same limited prompt sequence is visible again in the end-to-end run. | Registration collects the required passenger name, age, gender, contact number and password, and creates a retrievable user record. | The CLI asks only for a username and password, then reports registration success. It does not prompt for age, gender or contact number, and does not separately collect the passenger's name. This does not meet the SRS registration requirement. | **FAIL** | `04_registration.png`; `34_e2e_cli_workflow_part1.png` |
| TC-Auth-01 | RRS-F-002 — Authenticate a passenger. | Log in using a passenger account; incorrect-credential attempts are also shown in the lockout evidence. | Correct credentials allow passenger login; incorrect credentials are rejected. | Successful passenger login is shown. Repeated incorrect login attempts are rejected. | **PASS** | `05_passenger_login.png`; `11_account_lockout.png`; `12_login_blocked_during_lockout.png` |
| TC-Auth-02 | RRS-F-003 — Authenticate an administrator separately from passenger accounts. | Select Admin login and enter the administrator credentials. | Valid admin credentials open the admin console. | Admin login succeeds and the admin console is displayed. | **PASS** | `06_admin_login.png` |

### 5.2 Train search and seat availability

| Test case | Requirement / objective | Manual execution performed / evidence reviewed | Expected result | Actual result | Status | Screenshot evidence |
|---|---|---|---|---|---|---|
| TC-Search-01 | RRS-F-010 — Search trains by source, destination and journey date. | Search Bengaluru to New Delhi and also search a route with no matching train. | Matching trains display their train number, name and departure details; a route with no match returns a clear no-results response. | Karnataka Express and Rajdhani Express are displayed for the matching route. The unmatched route returns “No trains found for the selected route/class.” | **PASS** | `13_train_search_no_results.png`; `14_train_search_and_availability.png` |
| TC-Search-02 | RRS-F-011 — Display availability for each class. | Inspect search results for multiple classes and inspect the result after an admin quota change. | Displayed availability reflects the train/class inventory at the time of the search. | The search output displays class-level availability. The later search also shows the reduced available count after the quota was changed. | **PASS** | `14_train_search_and_availability.png`; `15_admin_quota_update.png`; `16_class_availability.png` |
| TC-Search-03 | RRS-F-012 — Indicate RAC/Waitlist availability when confirmed seats are exhausted. | Search the route/class after its confirmed quota is exhausted and inspect the availability indication; proceed to RAC/Waitlist booking evidence. | The full class is not silently rejected; RAC/Waitlist availability is indicated. | Search output displays `WL 1`, and subsequent booking evidence shows RAC and Waitlist tickets being created. | **PASS** | `19_rac_booking.png`; `20_waitlist_booking.png`; `21_waitlist_availability.png` |

### 5.3 Ticket booking and simulated payment

| Test case | Requirement / objective | Manual execution performed / evidence reviewed | Expected result | Actual result | Status | Screenshot evidence |
|---|---|---|---|---|---|---|
| TC-Book-01 | RRS-F-020 — Capture and validate passenger name, age, gender and berth preference for each booking. | Book confirmed, RAC and Waitlist tickets and inspect the passenger-detail prompts. | All required passenger fields, including berth preference, are collected and validated. | The CLI collects passenger name, age and gender. The shown booking flows proceed from gender to card input without prompting for berth preference. The required berth-preference field is therefore not demonstrated and appears to be omitted. | **FAIL** | `18_successful_booking.png`; `19_rac_booking.png`; `20_waitlist_booking.png`; `34_e2e_cli_workflow_part1.png` |
| TC-Book-02 | RRS-F-021 — Generate unique 10-digit PNRs. | Compare PNRs visible across confirmed, RAC and Waitlist bookings and the subsequent PNR lookups. | Each successful booking receives a unique 10-digit PNR. | The displayed sample bookings have distinct 10-digit PNRs: `1000000001`, `1000000002` and `1000000003`. The screenshots do not demonstrate generation of a new PNR after a data reload, so the complete uniqueness-across-reload condition is not fully evidenced. | **PARTIAL** | `18_successful_booking.png`; `19_rac_booking.png`; `20_waitlist_booking.png`; `24_confirmed_pnr_status.png`; `25_rac_pnr_status.png`; `26_waitlist_pnr_status.png` |
| TC-Book-03 | RRS-F-022 — Decrement confirmed-seat availability after booking. | Record availability, book one confirmed passenger on train 12627 class SL, then repeat the same search. | Availability decreases by the number of confirmed passengers booked. | Availability changed from 72 before booking to 71 after one confirmed passenger was booked. The booking receipt displayed status `CONFIRMED`. | **PASS** | `29_tc_book_03_seat_decrement.png` |
| TC-Book-04 | RRS-F-023 — Assign RAC/Waitlist status and queue position when confirmed seats are unavailable. | Create RAC and Waitlist bookings and inspect their receipts and search availability. | RAC/Waitlist bookings receive the appropriate status and queue position. | A RAC ticket is shown with position 1; a Waitlist ticket is shown with position 1. Search also displays Waitlist availability. | **PASS** | `19_rac_booking.png`; `20_waitlist_booking.png`; `21_waitlist_availability.png` |
| TC-Book-05 | RRS-F-024 — Simulate successful and failed payment. | Complete successful bookings; try an invalid card number and a simulated bank-declined payment; inspect the booking list after the failed attempt. | Successful payment creates a booking; failed payment does not create a new booking record. | Successful bookings are displayed. Invalid card input and simulated bank decline are rejected. The subsequent booking list displays the existing confirmed/RAC/Waitlist records and no additional record for the declined attempt. | **PASS** | `17_invalid_card_rejection.png`; `18_successful_booking.png`; `19_rac_booking.png`; `20_waitlist_booking.png`; `22_payment_declined.png`; `23_bookings_after_failed_payment.png` |
| TC-Book-06 | RRS-F-025 — Display a formatted ticket receipt. | Inspect receipts produced for confirmed and RAC bookings. | Receipt displays the PNR, passenger details, train, class, journey date and fare. | The displayed receipts contain the PNR, train details, route, date/class, status, passenger information and total fare. | **PASS** | `18_successful_booking.png`; `19_rac_booking.png`; `20_waitlist_booking.png` |

### 5.4 Cancellation, queue promotion and PNR lookup

| Test case | Requirement / objective | Manual execution performed / evidence reviewed | Expected result | Actual result | Status | Screenshot evidence |
|---|---|---|---|---|---|---|
| TC-Cancel-01 | RRS-F-030 — Cancel a ticket by PNR for the logged-in passenger. | Cancel a confirmed ticket using its PNR from the passenger menu. | A valid PNR belonging to the logged-in passenger is cancelled; an unowned PNR is rejected. | Successful cancellation of the logged-in passenger's ticket is shown. The supplied screenshots do not show an attempt to cancel another passenger's PNR, so the ownership-rejection part is not evidenced. | **PARTIAL** | `27_confirmed_ticket_cancellation.png`; `35_e2e_cli_workflow_part2.png` |
| TC-Cancel-03 | RRS-F-032 — Promote eligible RAC/Waitlist bookings after cancellation. | Cancel a confirmed ticket while RAC/Waitlist bookings are present; inspect promotion messages and the resulting PNR statuses. | Eligible queued bookings are promoted in the correct order and their updated status is visible. | The CLI reports that the queued PNRs moved up. The follow-up PNR display shows the resulting Confirmed and RAC statuses. | **PASS** | `27_confirmed_ticket_cancellation.png`; `28_cancellation_and_waitlist_promotion.png` |
| TC-Cancel-04 | RRS-F-033 — Retain cancelled booking status rather than deleting the record. | Look up the PNR after cancellation. | The booking remains retrievable with status `CANCELLED`. | The cancelled PNR remains retrievable and is displayed with status `CANCELLED`. | **PASS** | `28_cancellation_and_waitlist_promotion.png`; `35_e2e_cli_workflow_part2.png` |
| TC-PNR-01 | RRS-F-034 — Look up the current status of a booking by PNR. | Look up Confirmed, RAC and Waitlist PNRs; inspect the cancelled PNR after the end-to-end cancellation. | PNR lookup displays the current stored status and booking details. | Confirmed, RAC, Waitlist and Cancelled statuses are displayed for the corresponding PNRs. | **PASS** | `24_confirmed_pnr_status.png`; `25_rac_pnr_status.png`; `26_waitlist_pnr_status.png`; `28_cancellation_and_waitlist_promotion.png`; `35_e2e_cli_workflow_part2.png` |

### 5.5 Admin and data persistence

| Test case | Requirement / objective | Manual execution performed / evidence reviewed | Expected result | Actual result | Status | Screenshot evidence |
|---|---|---|---|---|---|---|
| TC-Admin-01 | RRS-F-040 — Add a train through the admin console and verify it in passenger search. | Inspect the available admin CLI menu. | Admin can add a train and the added train appears in passenger search. | The current CLI admin menu exposes only “View and update class fare/quota” and Logout. No add-train option is available through the menu. | **NOT EXECUTABLE THROUGH CURRENT CLI** | `06_admin_login.png`; `15_admin_quota_update.png` |
| TC-Admin-02 | RRS-F-041 — Update fare/quota and verify the change in a subsequent booking. | Update train 12627, class SL. First, quota values were changed and reflected in search. In the additional test, fare was changed to ₹800 and a subsequent confirmed booking was made. | The updated configuration is saved and applied to subsequent search/booking operations. | The quota update was saved and the reduced availability was displayed. In the additional test, fare changed from ₹745 to ₹800 and the new booking receipt displayed total fare ₹800. | **PASS** | `15_admin_quota_update.png`; `16_class_availability.png`; `31_tc_admin_02_fare_update_and_booking.png` |
| TC-Admin-03 | RRS-F-042 — Deactivate a train and verify that it is unavailable for new searches. | Inspect the available admin CLI menu. | Admin can deactivate a train; deactivated train is excluded from passenger search. | No deactivate-train option is exposed in the current admin CLI menu. | **NOT EXECUTABLE THROUGH CURRENT CLI** | `06_admin_login.png`; `15_admin_quota_update.png` |
| TC-Admin-04 | RRS-F-043 — Generate booking/revenue reports and compare with stored records. | Inspect the available admin CLI menu. | Admin can generate the requested report through the CLI. | No reporting option is exposed in the current admin CLI menu. | **NOT EXECUTABLE THROUGH CURRENT CLI** | `06_admin_login.png` |
| TC-Data-01 | RRS-F-050 — Verify persistence after normal application exit and restart. | Book a ticket, exit normally, restart using the same data directory, and look up the original PNR. | The same booking remains available with its original details after restart. | After restarting with the original `test_data_tc_book03` directory, PNR `1000000003` was found with the same train, passenger, date, class, status and total fare. | **PASS** | `33_tc_data_01_persistence_after_restart.png` |
| TC-Data-02 | RRS-F-051 — Back up and restore reservation data. | Inspect the available admin CLI menu. | Admin can create and restore a backup through the CLI. | No backup/restore option is exposed in the current admin CLI menu. | **NOT EXECUTABLE THROUGH CURRENT CLI** | `06_admin_login.png` |

### 5.6 Security validation

| Test case | Requirement / objective | Manual execution performed / evidence reviewed | Expected result | Actual result | Status | Screenshot evidence |
|---|---|---|---|---|---|---|
| TC-Sec-01 | RRS-SR-001 — Verify that stored passwords are not readable plaintext. | Inspect the stored user records and the password-hashing implementation evidence. | User records contain salted password hashes rather than readable passwords. | The sample `users.dat` records show salt and hash-like values rather than readable passwords. The implementation screenshot shows the SHA-256 hash being calculated using salt plus password. | **PASS** | `08_password_hash_storage.png`; `09_password_hash_implementation.png` |
| TC-Sec-02 | RRS-SR-002 — Verify masking of password and dummy card-number input. | Observe password entry and dummy card-number entry while typing. | Typed credential/card characters are not displayed in plaintext. | Password input is masked in the passenger login prompt. The dummy card number is also masked during booking input. | **PASS** | `10_password_masking.png`; `30_tc_sec_02_card_number_masking.png` |
| TC-Sec-03 | RRS-SR-003 — Verify that admin-only functions are not exposed to a passenger. | Log in as a passenger and inspect the passenger menu. | Admin-only operations are not accessible from the passenger session. | The passenger menu displays passenger operations and does not display admin operations. The screenshot does not show a direct attempt to invoke an admin operation outside that menu, so the full negative-access condition is not demonstrated. | **PARTIAL** | `07_admin_access_control.png` |
| TC-Sec-04 | RRS-SR-004 — Verify that admin data-changing actions are written to the admin log with a correct timestamp. | Update train 12627, class SL, and inspect the root `admin_log.txt`; compare its event timestamp with the system date and file modification time. | The admin action is recorded with the admin identity, action, target and correct event timestamp. | The action, identity and target were recorded, but the entry timestamp was `1970-12-12 22:54:40`. The system date was 31 October 2026 and the file modification time was also on 31 October 2026. The event timestamp is incorrect. | **FAIL** | `32_tc_sec_04_admin_log_timestamp_failure.png` |
| TC-Sec-05 | RRS-SR-005 — Lock an account after five consecutive failed login attempts and reject further login during the cooldown. | Enter five consecutive incorrect login attempts and attempt another login during the cooldown period. | The account is locked after five failures and further login is refused during the lockout period. | The CLI displays the lockout message after five failed attempts and rejects another login during the stated cooldown. | **PASS** | `11_account_lockout.png`; `12_login_blocked_during_lockout.png` |

## 6. End-to-end CLI system test

The STP describes a system-level passenger workflow from registration through search, booking and cancellation. The workflow was executed in a separate test data directory and captured in two screenshots.

| Step | Operation | Observed result | Evidence |
|---:|---|---|---|
| 1 | Register a passenger | Registration success message displayed. The same registration-field omission identified under TC-Reg-01 is visible in this flow. | `34_e2e_cli_workflow_part1.png` |
| 2 | Log in as the passenger | Login successful; passenger menu displayed. | `34_e2e_cli_workflow_part1.png` |
| 3 | Search for a train | Karnataka Express, train 12627, was returned for Bengaluru to New Delhi. | `34_e2e_cli_workflow_part1.png` |
| 4 | Book one passenger | Booking confirmed and PNR `1000000001` generated. | `34_e2e_cli_workflow_part1.png` |
| 5 | Look up the PNR | PNR `1000000001` displayed with status `CONFIRMED`. | `35_e2e_cli_workflow_part2.png` |
| 6 | Cancel the same PNR | CLI confirmed cancellation of PNR `1000000001`. | `35_e2e_cli_workflow_part2.png` |
| 7 | Look up the PNR again | The same PNR was displayed with status `CANCELLED`. | `35_e2e_cli_workflow_part2.png` |

**E2E-CLI-01 result: PASS.** The same PNR was followed from successful booking through cancellation and the final status lookup.

## 7. Defect register

Three requirement failures were identified from the manual evidence. Each must be entered as a separate GitHub Issue because the affected requirements/modules differ.

| Defect ID | Related test | Defect title | Observed failure | Affected requirement / module | GitHub tracking |
|---|---|---|---|---|---|
| DEF-01 | TC-Sec-04 | Admin action log records an incorrect event timestamp | The admin action is present, but the event timestamp is `1970-12-12 22:54:40` even though the log file was modified on 31 October 2026. | RRS-SR-004 — Admin logging / Admin module | GitHub Issue reference to be inserted after creation |
| DEF-02 | TC-Reg-01 | Passenger registration omits required profile fields | Registration prompts only for username and password. It does not collect the passenger name, age, gender or contact number required by RRS-F-001. | RRS-F-001 — Registration / Auth module | GitHub Issue reference to be inserted after creation |
| DEF-03 | TC-Book-01 | Booking flow does not collect berth preference | The booking prompts collect passenger name, age and gender, then proceed to card entry. No berth-preference prompt is shown, although RRS-F-020 requires it. | RRS-F-020 — Passenger detail capture / Booking module | GitHub Issue reference to be inserted after creation |

### Defect evidence and reproduction notes

#### DEF-01 — Incorrect admin event timestamp

**Evidence:** `32_tc_sec_04_admin_log_timestamp_failure.png`

Observed log entry:

```text
1970-12-12 22:54:40 | admin=admin | action=UPDATE_FARE_QUOTA | target=12627:SL
```

The same evidence shows the system date as 31 October 2026 and the log file modification time as 31 October 2026. The event timestamp recorded in the log is therefore inconsistent with the actual execution time.

**Expected:** The event entry should contain the correct current date/time while retaining the admin identity, action and target.

#### DEF-02 — Registration fields missing

**Evidence:** `04_registration.png`; `34_e2e_cli_workflow_part1.png`

The registration flow asks for:
- Username
- Password

It then displays “Registration successful.”

**Required by SRS RRS-F-001:** Passenger name, age, gender, contact number and password.

The CLI flow does not prompt for age, gender or contact number and does not separately collect the passenger's name. The registration result therefore does not demonstrate the profile information required by the SRS.

#### DEF-03 — Berth preference missing from booking

**Evidence:** `18_successful_booking.png`; `19_rac_booking.png`; `20_waitlist_booking.png`; `34_e2e_cli_workflow_part1.png`

The booking flow prompts for passenger name, age and gender. It then proceeds to the dummy card-number prompt.

**Required by SRS RRS-F-020:** Passenger name, age, gender and berth preference.

No berth-preference prompt is visible in the supplied booking executions.

### GitHub issue and assignment tracking

The defect records above are ready for GitHub tracking. Insert the actual issue numbers/links after creating the corresponding issues. Each issue must:
- Have the `bug` label.
- Be assigned to the owner of the affected module.
- Reference its test case and screenshot evidence.
- Be linked back into this report.

The report intentionally does not fabricate GitHub issue numbers or claim an issue has been created before its actual creation.

## 8. Screenshot evidence register

All 32 screenshots supplied for this execution are referenced below. The files are stored separately and are not embedded in this report.

| Screenshot | Evidence captured | Related test / use |
|---|---|---|
| `04_registration.png` | Registration menu, username/password prompts and registration success message. | TC-Reg-01; DEF-02 |
| `05_passenger_login.png` | Successful passenger login and passenger menu. | TC-Auth-01 |
| `06_admin_login.png` | Successful admin login and the available admin menu. | TC-Auth-02; TC-Admin-01/03/04/TC-Data-02 CLI availability review |
| `07_admin_access_control.png` | Passenger session and passenger-only menu. | TC-Sec-03 |
| `08_password_hash_storage.png` | Sample user records showing stored salt/hash values rather than readable passwords. | TC-Sec-01 |
| `09_password_hash_implementation.png` | Source inspection showing salted SHA-256 password-hash implementation. Supplementary implementation evidence. | TC-Sec-01 |
| `10_password_masking.png` | Password entry displayed using masking characters. | TC-Sec-02 |
| `11_account_lockout.png` | Repeated failed login attempts and account-lockout message after five failures. | TC-Sec-05 |
| `12_login_blocked_during_lockout.png` | Subsequent login rejected during the cooldown period. | TC-Sec-05 |
| `13_train_search_no_results.png` | Search for a route with no matching trains and the no-results message. | TC-Search-01 |
| `14_train_search_and_availability.png` | Matching trains displayed with departure, class, fare and availability values. | TC-Search-01; TC-Search-02 |
| `15_admin_quota_update.png` | Admin updates confirmed/RAC/Waitlist quota values for train 12627, class SL; success message displayed. | TC-Admin-02 |
| `16_class_availability.png` | Passenger search shows the updated available count after quota change. | TC-Search-02; TC-Admin-02 |
| `17_invalid_card_rejection.png` | Invalid card number rejected during booking. | TC-Book-05 |
| `18_successful_booking.png` | Confirmed booking and formatted ticket receipt with PNR and passenger details. | TC-Book-01; TC-Book-02; TC-Book-05; TC-Book-06; DEF-03 |
| `19_rac_booking.png` | RAC booking and receipt with RAC position. | TC-Search-03; TC-Book-01; TC-Book-02; TC-Book-04; TC-Book-06; DEF-03 |
| `20_waitlist_booking.png` | Waitlist booking and receipt with queue position. | TC-Search-03; TC-Book-01; TC-Book-02; TC-Book-04; TC-Book-06; DEF-03 |
| `21_waitlist_availability.png` | Search displays Waitlist availability for the selected train/class. | TC-Search-03; TC-Book-04 |
| `22_payment_declined.png` | Simulated bank-declined payment response. | TC-Book-05 |
| `23_bookings_after_failed_payment.png` | Booking list displayed after the failed payment attempt, showing the existing booking records. | TC-Book-05 |
| `24_confirmed_pnr_status.png` | PNR lookup displays Confirmed status. | TC-Book-02; TC-PNR-01 |
| `25_rac_pnr_status.png` | PNR lookup displays RAC status and position. | TC-Book-02; TC-PNR-01 |
| `26_waitlist_pnr_status.png` | PNR lookup displays Waitlist status and position. | TC-Book-02; TC-PNR-01 |
| `27_confirmed_ticket_cancellation.png` | Successful cancellation and messages indicating queued bookings moved up. | TC-Cancel-01; TC-Cancel-03 |
| `28_cancellation_and_waitlist_promotion.png` | Follow-up PNR lookups display the resulting Confirmed, RAC and Cancelled states. | TC-Cancel-03; TC-Cancel-04; TC-PNR-01 |
| `29_tc_book_03_seat_decrement.png` | Before/after search availability shows 72 seats before booking and 71 after one confirmed booking. | TC-Book-03 |
| `30_tc_sec_02_card_number_masking.png` | Dummy card-number entry displayed using masking characters. | TC-Sec-02 |
| `31_tc_admin_02_fare_update_and_booking.png` | Fare updated to ₹800 and subsequent confirmed booking receipt displays total fare ₹800. | TC-Admin-02 |
| `32_tc_sec_04_admin_log_timestamp_failure.png` | Root/build admin logs, system date and file modification metadata; event timestamp discrepancy visible. | TC-Sec-04; DEF-01 |
| `33_tc_data_01_persistence_after_restart.png` | PNR `1000000003` retrieved after restarting with the original data directory; booking details remain present. | TC-Data-01 |
| `34_e2e_cli_workflow_part1.png` | End-to-end run: registration, passenger login, train search and confirmed booking. | E2E-CLI-01; TC-Reg-01; TC-Auth-01; TC-Search-01; TC-Book-01/02/05/06 |
| `35_e2e_cli_workflow_part2.png` | Same PNR looked up as Confirmed, cancelled and then looked up as Cancelled. | E2E-CLI-01; TC-Cancel-01/04; TC-PNR-01 |

## 9. Retest and closure log

No fixes or retests for the three defects are recorded in this report yet.

| Defect | Initial result | Fix status | Retest required | Retest result |
|---|---|---|---|---|
| DEF-01 — Admin log timestamp | FAIL | Pending | Repeat TC-Sec-04 after the logging fix and verify the event timestamp against the system clock. | Not yet executed |
| DEF-02 — Registration fields missing | FAIL | Pending | Repeat TC-Reg-01 and verify that all fields required by RRS-F-001 are collected and retained as specified. | Not yet executed |
| DEF-03 — Berth preference missing | FAIL | Pending | Repeat TC-Book-01 and verify that berth preference is collected and validated for each passenger. | Not yet executed |

After each fix:
1. Run the affected manual CLI test again.
2. Save the new screenshot in `docs/evidence/issue-23/`.
3. Update the relevant test-case actual result and status.
4. Update the defect's GitHub Issue with the retest evidence.
5. Record the retest outcome in this section.

## 10. Final assessment

The supplied screenshots and additional CLI runs demonstrate substantial manual system coverage across passenger authentication, train search, availability, booking, payment handling, PNR lookup, cancellation, queue promotion, admin fare/quota updates and persistence after restart.

The end-to-end passenger workflow also completed successfully using one PNR from booking through cancellation and final status lookup.

The test execution has also identified three requirement failures requiring defect tracking:
- Incomplete passenger registration fields.
- Missing berth preference during booking.
- Incorrect timestamp in the admin action log.

Four admin/data operations could not be executed through the current CLI because the relevant menu options are not exposed: add train, deactivate train, reporting, and backup/restore.


---

**Evidence directory:** `docs/evidence/issue-23/`  
**Execution report:** `docs/SYSTEM_ACCEPTANCE_TEST_REPORT.md`
