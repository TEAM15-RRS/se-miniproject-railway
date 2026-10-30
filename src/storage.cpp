#include "rrs/storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "rrs/auth.h"
#include "rrs/util.h"

#include <chrono>
#include <ctime>
#include <iomanip>


namespace fs = std::filesystem;

namespace rrs {

std::string Storage::usersPath() const { return (fs::path(dir_) / "users.dat").string(); }
std::string Storage::trainsPath() const { return (fs::path(dir_) / "trains.dat").string(); }
std::string Storage::bookingsPath() const { return (fs::path(dir_) / "bookings.dat").string(); }

namespace {

bool writeAtomic(const std::string& path, const std::string& content, std::string* err) {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) { if (err) *err = "Cannot write " + tmp; return false; }
        out << content;
        out.flush();
        if (!out) { if (err) *err = "Write failed for " + tmp; return false; }
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec) { if (err) *err = "Cannot replace " + path + ": " + ec.message(); return false; }
    return true;
}

bool readLines(const std::string& path, std::vector<std::string>& lines) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return true;  // missing file = empty data set
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!trim(line).empty()) lines.push_back(line);
    }
    return true;
}

bool toInt(const std::string& s, int& out) {
    try { size_t p; out = std::stoi(s, &p); return p == s.size(); } catch (...) { return false; }
}
bool toLL(const std::string& s, long long& out) {
    try { size_t p; out = std::stoll(s, &p); return p == s.size(); } catch (...) { return false; }
}

bool parseUser(const std::string& line, User& u) {
    auto f = split(line, '|');
    int failed, admin;
    if (f.size() != 6 || f[0].empty() || !toInt(f[3], admin) || !toInt(f[4], failed) || !toLL(f[5], u.lockedUntil))
        return false;
    u.username = f[0]; u.salt = f[1]; u.passwordHash = f[2];
    u.isAdmin = admin != 0; u.failedAttempts = failed;
    return true;
}

bool parseTrain(const std::string& line, Train& t) {
    auto f = split(line, '|');
    int active;
    if (f.size() != 7 || !toInt(f[0], t.number) || !toInt(f[5], active)) return false;
    t.name = f[1]; t.source = f[2]; t.destination = f[3]; t.departure = f[4]; t.active = active != 0;
    int h, m;
    if (!parseTime(t.departure, h, m)) return false;
    for (const auto& cs : split(f[6], ';')) {
        if (cs.empty()) continue;
        auto c = split(cs, ':');
        ClassInfo ci;
        if (c.size() != 5 || !toInt(c[1], ci.totalSeats) || !toInt(c[2], ci.racSeats) ||
            !toInt(c[3], ci.wlLimit) || !toInt(c[4], ci.fare))
            return false;
        ci.code = c[0];
        t.classes.push_back(ci);
    }
    return true;
}

bool parseTicket(const std::string& line, Ticket& t) {
    auto f = split(line, '|');
    if (f.size() != 12) return false;
    if (!toLL(f[1], t.seq) || !toInt(f[3], t.trainNumber) || !statusFromString(f[6], t.status) ||
        !toInt(f[7], t.totalFare) || !toLL(f[8], t.bookedAt) || !toInt(f[9], t.refundAmount) ||
        !toLL(f[10], t.cancelledAt))
        return false;
    t.pnr = f[0]; t.username = f[2]; t.classCode = f[4]; t.journeyDate = f[5];
    for (const auto& ps : split(f[11], ';')) {
        if (ps.empty()) continue;
        auto p = split(ps, ',');
        Passenger pa;
        if (p.size() != 3 || !toInt(p[1], pa.age) || p[2].size() != 1) return false;
        pa.name = p[0]; pa.gender = p[2][0];
        t.passengers.push_back(pa);
    }
    return !t.pnr.empty() && !t.passengers.empty();
}

template <typename T, typename Parser>
void loadFile(const std::string& path, std::vector<T>& out, Parser parser, LoadReport* rep) {
    std::vector<std::string> lines;
    readLines(path, lines);
    for (const auto& l : lines) {
        T item;
        if (parser(l, item)) out.push_back(item);
        else if (rep) { ++rep->skippedLines; rep->warnings.push_back(path + ": skipped corrupt line"); }
    }
}


template <typename T, typename Parser>
bool validateBackupFile(const std::string& path, Parser parser, std::string* err) {
    if (!fs::is_regular_file(path)) {
        if (err) *err = "Missing backup file: " + path;
        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (err) *err = "Cannot read backup file: " + path;
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (trim(line).empty()) continue;

        T item;
        if (!parser(line, item)) {
            if (err) *err = "Corrupt record in backup file: " + path;
            return false;
        }
    }

    if (in.bad()) {
        if (err) *err = "Error reading backup file: " + path;
        return false;
    }

    return true;
}

}  // namespace

bool Storage::load(Database& db, LoadReport* report) const {
    db = Database{};
    loadFile(usersPath(), db.users, parseUser, report);
    loadFile(trainsPath(), db.trains, parseTrain, report);
    loadFile(bookingsPath(), db.tickets, parseTicket, report);
    return true;
}

bool Storage::save(const Database& db, std::string* error) const {
    std::error_code ec;
    fs::create_directories(dir_, ec);

    std::ostringstream u, t, b;
    for (const auto& x : db.users)
        u << sanitizeField(x.username) << '|' << x.salt << '|' << x.passwordHash << '|' << (x.isAdmin ? 1 : 0)
          << '|' << x.failedAttempts << '|' << x.lockedUntil << '\n';
    for (const auto& x : db.trains) {
        t << x.number << '|' << sanitizeField(x.name) << '|' << sanitizeField(x.source) << '|'
          << sanitizeField(x.destination) << '|' << x.departure << '|' << (x.active ? 1 : 0) << '|';
        for (size_t i = 0; i < x.classes.size(); ++i) {
            const auto& c = x.classes[i];
            t << (i ? ";" : "") << sanitizeField(c.code) << ':' << c.totalSeats << ':' << c.racSeats << ':'
              << c.wlLimit << ':' << c.fare;
        }
        t << '\n';
    }
    for (const auto& x : db.tickets) {
        b << x.pnr << '|' << x.seq << '|' << sanitizeField(x.username) << '|' << x.trainNumber << '|'
          << x.classCode << '|' << x.journeyDate << '|' << toString(x.status) << '|' << x.totalFare << '|'
          << x.bookedAt << '|' << x.refundAmount << '|' << x.cancelledAt << '|';
        for (size_t i = 0; i < x.passengers.size(); ++i) {
            std::string nm = sanitizeField(x.passengers[i].name);
            for (auto& ch : nm) if (ch == ',') ch = ' ';
            b << (i ? ";" : "") << nm << ',' << x.passengers[i].age << ',' << x.passengers[i].gender;
        }
        b << '\n';
    }
    return writeAtomic(usersPath(), u.str(), error) && writeAtomic(trainsPath(), t.str(), error) &&
           writeAtomic(bookingsPath(), b.str(), error);
}


bool Storage::createBackup(std::string& backupPath, std::string* error) const {
    std::error_code ec;

    const fs::path backupRoot = fs::path(dir_) / "backups";
    fs::create_directories(backupRoot, ec);

    if (ec) {
        if (error) *error = "Cannot create backup directory: " + ec.message();
        return false;
    }

    // Generate a timestamp for the backup folder name.
    const auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());

    std::tm localTime{};
    #ifdef _WIN32
        localtime_s(&localTime, &now);
    #else
        localtime_r(&now, &localTime);
    #endif

    std::ostringstream name;
    name << "backup_" << std::put_time(&localTime, "%Y%m%d_%H%M%S");

    fs::path destination = backupRoot / name.str();

    // Avoid overwriting a backup created in the same second.
    int suffix = 1;
    while (fs::exists(destination)) {
        destination = backupRoot / (name.str() + "_" + std::to_string(suffix++));
    }

    const std::vector<std::string> files = {
        usersPath(), trainsPath(), bookingsPath()
    };

    for (const auto& file : files) {
        if (!fs::is_regular_file(file)) {
            if (error) *error = "Missing data file: " + file;
            return false;
        }
    }

    fs::create_directory(destination, ec);
    if (ec) {
        if (error) *error = "Cannot create backup folder: " + ec.message();
        return false;
    }

    for (const auto& file : files) {
        fs::copy_file(
            file,
            destination / fs::path(file).filename(),
            fs::copy_options::none,
            ec
        );

        if (ec) {
            fs::remove_all(destination);
            if (error) *error = "Backup copy failed: " + ec.message();
            return false;
        }
    }

    backupPath = destination.string();
    return true;
}


bool Storage::restoreBackup(const std::string& backupPath, std::string* error) const {
    const fs::path backupDir(backupPath);

    if (!fs::is_directory(backupDir)) {
        if (error) *error = "Backup directory does not exist";
        return false;
    }

    const fs::path backupUsers = backupDir / "users.dat";
    const fs::path backupTrains = backupDir / "trains.dat";
    const fs::path backupBookings = backupDir / "bookings.dat";

    // Validate every file before touching the current data.
    if (!validateBackupFile<User>(backupUsers.string(), parseUser, error) ||
        !validateBackupFile<Train>(backupTrains.string(), parseTrain, error) ||
        !validateBackupFile<Ticket>(backupBookings.string(), parseTicket, error)) {
        return false;
    }

    std::error_code ec;
    fs::create_directories(dir_, ec);

    if (ec) {
        if (error) *error = "Cannot access data directory: " + ec.message();
        return false;
    }

    // Stage the new files inside the data directory so renames stay
    // on the same filesystem.
    const fs::path stagingDir = fs::path(dir_) / ".restore_staging";

    if (fs::exists(stagingDir)) {
        if (error) *error = "Restore staging directory already exists";
        return false;
    }

    fs::create_directory(stagingDir, ec);
    if (ec) {
        if (error) *error = "Cannot create restore staging directory: " + ec.message();
        return false;
    }

    const std::vector<std::pair<fs::path, fs::path>> files = {
        {backupUsers, fs::path(usersPath())},
        {backupTrains, fs::path(trainsPath())},
        {backupBookings, fs::path(bookingsPath())}
    };

    struct Replacement {
        fs::path target;
        fs::path staged;
        fs::path old;
        bool originalMoved = false;
        bool installed = false;
    };

    std::vector<Replacement> replacements;

    for (const auto& file : files) {
        const fs::path staged = stagingDir / file.second.filename();

        fs::copy_file(file.first, staged, fs::copy_options::none, ec);

        if (ec) {
            fs::remove_all(stagingDir);
            if (error) *error = "Cannot stage backup file: " + ec.message();
            return false;
        }

        replacements.push_back({
            file.second,
            staged,
            stagingDir / (file.second.filename().string() + ".old")
        });
    }

    // Replace each file. Keep the originals until every replacement succeeds.
    for (auto& item : replacements) {
        if (fs::exists(item.target, ec)) {
            if (ec) break;

            fs::rename(item.target, item.old, ec);
            if (ec) break;

            item.originalMoved = true;
        }

        fs::rename(item.staged, item.target, ec);
        if (ec) break;

        item.installed = true;
    }

    if (ec) {
        const std::string failure = ec.message();

        // Roll back in reverse order if any replacement failed.
        for (auto it = replacements.rbegin(); it != replacements.rend(); ++it) {
            std::error_code rollbackError;

            if (it->installed)
                fs::remove(it->target, rollbackError);

            if (it->originalMoved) {
                rollbackError.clear();
                fs::rename(it->old, it->target, rollbackError);
            }
        }

        fs::remove_all(stagingDir);

        if (error) *error = "Restore failed; rollback attempted: " + failure;
        return false;
    }

    fs::remove_all(stagingDir);
    return true;
}


void seedDatabase(Database& db) {
    auto train = [](int no, const char* name, const char* src, const char* dst, const char* dep,
                    std::vector<ClassInfo> cl) {
        Train t; t.number = no; t.name = name; t.source = src; t.destination = dst; t.departure = dep;
        t.classes = std::move(cl);
        return t;
    };
    db.trains.push_back(train(12627, "Karnataka Express", "Bengaluru", "New Delhi", "19:20",
                              {{"SL", 72, 10, 20, 745}, {"3A", 48, 5, 10, 2015}, {"2A", 24, 3, 6, 2890}}));
    db.trains.push_back(train(22691, "Rajdhani Express", "Bengaluru", "New Delhi", "20:00",
                              {{"3A", 60, 6, 12, 2760}, {"2A", 30, 4, 8, 3880}}));
    db.trains.push_back(train(12007, "Shatabdi Express", "Chennai", "Bengaluru", "06:00",
                              {{"CC", 78, 0, 10, 1145}, {"EC", 24, 0, 4, 2260}}));
    db.trains.push_back(train(12951, "Mumbai Rajdhani", "Mumbai", "New Delhi", "16:35",
                              {{"3A", 64, 6, 12, 2490}, {"2A", 40, 4, 8, 3560}}));
    db.trains.push_back(train(16526, "Kanyakumari Express", "Bengaluru", "Kanyakumari", "22:30",
                              {{"SL", 72, 10, 20, 590}, {"3A", 48, 5, 10, 1565}}));
    if (!findUser(db, "admin")) db.users.push_back(makeUser("admin", "Admin@123", true));  // DEV ONLY
}

}  // namespace rrs
