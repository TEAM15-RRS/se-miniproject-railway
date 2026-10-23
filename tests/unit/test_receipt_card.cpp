#include "test_helpers.h"

// Owner: Amruta - e-ticket shows only the masked card number (RRS-SR-002)

TEST(TC_Book_26_ReceiptShowsMaskedCardOnly) {
    Env e;
    auto r = bookN(e, "alice", "SL", 1);
    std::string receipt = formatReceipt(e.db, r.ticket, r.txnId, PaymentService::mask(kGoodCard));
    CHECK_CONTAINS(receipt, "(card ************1111)");
    CHECK(!tf::contains(receipt, kGoodCard));  // the full number must never appear
}
TEST(TC_Book_27_ReceiptWithoutCardStillWorks) {
    Env e;
    auto r = bookN(e, "alice", "SL", 1);
    std::string receipt = formatReceipt(e.db, r.ticket, r.txnId);
    CHECK_CONTAINS(receipt, r.txnId);
    CHECK(!tf::contains(receipt, "(card"));
}
