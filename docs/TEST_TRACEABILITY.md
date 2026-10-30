# Test Traceability - STP IDs <-> automated tests

Run everything: `ctest --test-dir build --output-on-failure`.

The test count must be verified against the current branch before reporting
final execution totals.

| SRS ID | STP test case | Automated test(s) | Level / status |
|---|---|---|---|
| RRS-F-001 | TC-Reg-01 | `TC_Reg_01..04` | Unit |
| RRS-F-002 | TC-Auth-01 | `TC_Auth_01..03` | Unit |
| RRS-F-003 / SR-003 | TC-Auth-04 | `TC_Auth_04_AdminLoginRequiresAdminAccount` | Unit |
| RRS-F-010..012 | - | `TC_Search_01..07` | Unit |
| RRS-F-020 / F-022 | - | `TC_Book_01`, `TC_Book_03` | Unit |
| RRS-F-021 | TC-Book-02 | `TC_Book_02_PnrIsUniqueAnd10Digits`, `IT_05`, `IT_06` | Unit + Integration |
| RRS-F-023 | TC-Book-04 | `TC_Book_04`, `TC_Book_05` | Unit |
| RRS-F-024 | - | `TC_Book_11_PaymentFailureStoresNothing`, `UT_Pay_*`, `IT_08` | Unit + Integration |
| RRS-F-025 | - | `TC_Book_13_ReceiptContainsKeyFields` | Unit |
| RRS-F-030 / F-033 | - | `TC_Cancel_01`, `TC_Cancel_05`, `IT_06` | Unit + Integration |
| RRS-F-031 | TC-Cancel-02 | `TC_Cancel_02_RefundSlabBoundaries`, `TC_Cancel_03`, `TC_Cancel_04` | Unit |
| RRS-F-032 | TC-Cancel-03 | `TC_Cancel_06..10`, `IT_07` | Unit + Integration |
| RRS-F-034 | - | `TC_Book_14`, `TC_Book_15` | Unit |
| RRS-F-050 | - | `TC_Store_01..06`, `IT_01`, `IT_04` | Unit + Integration |
| RRS-NF-002 | - | `TC_Store_03`, `IT_12`, input-validation tests (`TC_Book_06..09`, `TC_Search_04`) | Unit + Integration |
| RRS-NF-003 | - | `TC_Store_04`, `IT_10` | Unit + Integration |
| RRS-NF-001 | TC-Perf-01 | Performance test for search response time <= 1 second | Planned / execution not verified |
| RRS-SR-001 | TC-Sec-01 | `TC_Sec_01`, `TC_Sec_02`, `IT_02` | Unit + Integration |
| RRS-SR-002 | - | `TC_Sec_03`; masked CLI entry checked manually | Unit + Manual |
| RRS-SR-003 | TC-Auth-04 | `TC_Auth_04_AdminLoginRequiresAdminAccount` | Unit |
| RRS-SR-004 | - | Admin action logging to `admin_log.txt` | Implementation/test status to verify |
| RRS-SR-005 | TC-Sec-05 | `TC_Sec_05..07`, `IT_03` | Unit + Integration |
| Admin module | - | `TC_Admin_01..18` (18 test cases identified in `tests/unit/test_admin.cpp`) | Unit tests; execution to verify |
| Backup/restore | - | No automated test mapping verified | Pending verification |
| Fuzzing | - | No automated test mapping verified | Pending verification |

**Boundary/error coverage examples:** age 0/1/120/121; passengers 0/6/7;
username 2/3/20/21 characters; password 7/8/64/65; refund at 48h/47h59,
12h/11h59, and 4h/3h59; leap-day dates; booking at departure minute versus
one minute before; corrupt and half-written data files.


**Sprint 2 follow-up:** verify execution of the admin tests and admin action
logging tests; execute and record TC-Perf-01; and confirm implementation and
test evidence for backup/restore and fuzzing before marking them covered.
