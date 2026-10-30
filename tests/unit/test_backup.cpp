
#include <fstream>

#include "test_helpers.h"

TEST(TC_Backup_01_CreatesBackupWithAllFiles) {
    Env e;
    Storage st(freshTempDir("backup_create"));
    CHECK(st.save(e.db));

    std::string backupPath;
    std::string error;

    CHECK(st.createBackup(backupPath, &error));
    CHECK(!backupPath.empty());

    CHECK(std::filesystem::exists(std::filesystem::path(backupPath) / "users.dat"));
    CHECK(std::filesystem::exists(std::filesystem::path(backupPath) / "trains.dat"));
    CHECK(std::filesystem::exists(std::filesystem::path(backupPath) / "bookings.dat"));
}

TEST(TC_Backup_02_RestoresOriginalData) {
    Env e;
    bookN(e, "alice", "SL", 1);

    Storage st(freshTempDir("backup_restore"));
    CHECK(st.save(e.db));

    std::string backupPath;
    CHECK(st.createBackup(backupPath));

    Database changed;
    changed.trains.push_back(Train{});
    CHECK(st.save(changed));

    CHECK(st.restoreBackup(backupPath));

    Database restored;
    CHECK(st.load(restored));
    CHECK_EQ(restored.users.size(), e.db.users.size());
    CHECK_EQ(restored.trains.size(), e.db.trains.size());
    CHECK_EQ(restored.tickets.size(), e.db.tickets.size());
    CHECK_EQ(restored.tickets[0].pnr, e.db.tickets[0].pnr);
}

TEST(TC_Backup_03_RejectsCorruptBackupWithoutChangingData) {
    Env e;
    Storage st(freshTempDir("backup_corrupt"));
    CHECK(st.save(e.db));

    std::string backupPath;
    CHECK(st.createBackup(backupPath));

    // Corrupt one record in the backup.
    {
        std::ofstream out(
            std::filesystem::path(backupPath) / "trains.dat",
            std::ios::trunc
        );
        out << "this is not a valid train record\n";
    }

    // Save the current data so we can verify it remains unchanged.
    Database changed;
    changed.users.push_back(makeUser("charlie", "Passw0rd3", false));
    CHECK(st.save(changed));

    std::string error;
    CHECK(!st.restoreBackup(backupPath, &error));
    CHECK(!error.empty());

    Database after;
    CHECK(st.load(after));
    CHECK_EQ(after.users.size(), size_t(1));
    CHECK_EQ(after.users[0].username, std::string("charlie"));
}

TEST(TC_Backup_04_RejectsMissingBackupDirectory) {
    Storage st(freshTempDir("backup_missing"));

    Database original;
    original.users.push_back(makeUser("charlie", "Passw0rd3", false));
    CHECK(st.save(original));

    std::string error;
    CHECK(!st.restoreBackup("missing_backup_folder", &error));
    CHECK(!error.empty());

    Database after;
    CHECK(st.load(after));
    CHECK_EQ(after.users.size(), size_t(1));
    CHECK_EQ(after.users[0].username, std::string("charlie"));
}
