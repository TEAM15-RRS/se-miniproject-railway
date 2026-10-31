
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>

#include "rrs/admin.h"
#include "rrs/auth.h"
#include "rrs/booking.h"
#include "rrs/cancel.h"
#include "rrs/search.h"
#include "rrs/storage.h"
#include "rrs/types.h"
#include "rrs/util.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace rrs;

struct EofExit {};

struct AddTrainCancelled {};

static std::string ask(const std::string& prompt) {
    std::cout << prompt << std::flush;

    std::string value;
    if (!std::getline(std::cin, value)) {
        throw EofExit{};
    }

    return trim(value);
}

static std::string askAddTrain(const std::string& prompt) {
    std::string value = ask(prompt);

    std::string command = value;
    std::transform(command.begin(), command.end(), command.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::toupper(c));
                   });

    if (command == "CNCL") {
        throw AddTrainCancelled{};
    }

    return value;
}

static std::string askMasked(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string value;

#ifdef _WIN32
    int c;

    while ((c = _getch()) != '\r' && c != '\n') {
        if (c == 8) {
            if (!value.empty()) {
                value.pop_back();
                std::cout << "\b \b";
            }
        } else if (c == 3) {
            throw EofExit{};
        } else {
            value.push_back(static_cast<char>(c));
            std::cout << '*';
        }
    }

    std::cout << '\n';
#else
    if (isatty(STDIN_FILENO)) {
        termios oldSettings{}, newSettings{};
        tcgetattr(STDIN_FILENO, &oldSettings);

        newSettings = oldSettings;
        newSettings.c_lflag &= ~static_cast<tcflag_t>(ECHO);

        tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

        const bool ok = static_cast<bool>(
            std::getline(std::cin, value)
        );

        tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
        std::cout << '\n';

        if (!ok) {
            throw EofExit{};
        }
    } else if (!std::getline(std::cin, value)) {
        throw EofExit{};
    }
#endif

    return trim(value);
}

static int askInt(const std::string& prompt, int defaultValue = -1) {
    const std::string value = ask(prompt);

    try {
        std::size_t position = 0;
        const int result = std::stoi(value, &position);

        if (position == value.size()) {
            return result;
        }
    } catch (...) {
    }

    return defaultValue;
}

static int askIntRange(
    const std::string& prompt,
    int minValue,
    int maxValue
) {
    while (true) {
        const std::string value = ask(prompt);

        try {
            std::size_t position = 0;
            const int result = std::stoi(value, &position);

            if (position == value.size() &&
                result >= minValue &&
                result <= maxValue) {
                return result;
            }
        } catch (...) {
        }

        std::cout << "Invalid input. Enter a number between "
                  << minValue << " and " << maxValue << ".\n";
    }
}

static int askAddTrainIntRange(
    const std::string& prompt,
    int minValue,
    int maxValue
) {
    while (true) {
        const std::string value = askAddTrain(prompt);

        try {
            std::size_t position = 0;
            const int result = std::stoi(value, &position);

            if (position == value.size() &&
                result >= minValue &&
                result <= maxValue) {
                return result;
            }
        } catch (...) {
        }

        std::cout << "Invalid input. Enter a number between "
                  << minValue << " and " << maxValue << ".\n";
    }
}

static std::string askNonEmpty(const std::string& prompt) {
    while (true) {
        const std::string value = ask(prompt);

        if (!value.empty()) {
            return value;
        }

        std::cout << "Input cannot be empty. Please try again.\n";
    }
}

static std::string askAddTrainNonEmpty(const std::string& prompt) {
    while (true) {
        const std::string value = askAddTrain(prompt);

        if (!value.empty()) {
            return value;
        }

        std::cout << "Input cannot be empty. Please try again.\n";
    }
}

static std::string askAddTrainTime(const std::string& prompt) {
    while (true) {
        const std::string value = askAddTrainNonEmpty(prompt);

        int hour = 0;
        int minute = 0;

        if (parseTime(value, hour, minute)) {
            return value;
        }

        std::cout << "Invalid time. Use HH:MM format (00:00-23:59).\n";
    }
}

static std::string askTime(const std::string& prompt) {
    while (true) {
        const std::string value = askNonEmpty(prompt);

        int hour = 0;
        int minute = 0;

        if (parseTime(value, hour, minute)) {
            return value;
        }

        std::cout << "Invalid time. Use HH:MM format (00:00-23:59).\n";
    }
}

struct App {
    Database db;
    Storage storage;
    Clock clock = [] { return currentMinutes(); };

    AuthService auth;
    AdminService adminService;
    PaymentService pay;
    BookingService booking;
    CancelService cancelSvc;
    SearchService search;

    std::string user;
    bool admin = false;

    explicit App(const std::string& directory)
        : storage(directory),
          auth(db, clock),
          adminService(db, clock),
          booking(db, pay, clock),
          cancelSvc(db, clock),
          search(db, clock) {}

    void persist() {
        std::string error;

        if (!storage.save(db, &error)) {
            std::cout << "WARNING: could not save data: "
                      << error << '\n';
        }
    }

    void doRegister() {
        const std::string username = ask("Choose username: ");
        const std::string password = askMasked("Choose password: ");

        const auto result = auth.registerUser(username, password);
        std::cout << result.message << '\n';

        if (result.ok) {
            persist();
        }
    }

    void doLogin(bool adminLogin) {
        const std::string username = ask("Username: ");
        const std::string password = askMasked("Password: ");

        const auto result = auth.login(
            username, password, adminLogin
        );

        std::cout << result.message << '\n';
        persist();

        if (result.ok) {
            const User* loggedInUser = findUser(db, username);

            if (loggedInUser) {
                user = loggedInUser->username;
                admin = adminLogin;
            }
        }
    }

    void doSearch() {
        SearchQuery query{
            ask("From: "),
            ask("To: "),
            ask("Date (YYYY-MM-DD): "),
            ask("Class (blank = all): ")
        };

        const auto result = search.search(query);

        if (!result.ok || result.rows.empty()) {
            std::cout << result.message << '\n';
            return;
        }

        std::cout << std::left
                  << std::setw(7) << "Train"
                  << std::setw(22) << "Name"
                  << std::setw(7) << "Dep"
                  << std::setw(6) << "Cls"
                  << std::setw(8) << "Fare"
                  << "Availability\n";

        for (const auto& row : result.rows) {
            std::cout << std::setw(7) << row.trainNumber
                      << std::setw(22) << row.trainName
                      << std::setw(7) << row.departure
                      << std::setw(6) << row.classCode
                      << std::setw(8) << row.fare
                      << row.availability << '\n';
        }
    }

    void doBook() {
        BookingRequest request;
        request.username = user;
        request.trainNumber = askInt("Train number: ");
        request.classCode = ask("Class: ");
        request.journeyDate = ask("Date (YYYY-MM-DD): ");

        const int passengerCount = askInt(
            "Number of passengers (1-" +
            std::to_string(kMaxPassengersPerBooking) + "): ",
            0
        );

        for (int i = 1;
             i <= passengerCount && i <= kMaxPassengersPerBooking;
             ++i) {
            Passenger passenger;

            passenger.name = ask(
                "  Passenger " + std::to_string(i) + " name: "
            );

            passenger.age = askInt("  Age: ");

            const std::string gender = ask("  Gender (M/F/O): ");

            passenger.gender = gender.empty()
                ? '?'
                : static_cast<char>(
                    std::toupper(
                        static_cast<unsigned char>(gender[0])
                    )
                );

            request.passengers.push_back(passenger);
        }

        request.cardNumber = askMasked(
            "Dummy card number (16 digits): "
        );

        const auto result = booking.book(request);

        if (!result.ok) {
            std::cout << "Booking failed: "
                      << result.message << '\n';
            return;
        }

        persist();

        std::cout << result.message << '\n'
                  << formatReceipt(
                      db,
                      result.ticket,
                      result.txnId,
                      PaymentService::mask(request.cardNumber)
                  );
    }

    void doStatus() {
        const Ticket* ticket = lookupPnr(db, ask("PNR: "));

        if (!ticket) {
            std::cout << "PNR not found.\n";
        } else {
            std::cout << formatReceipt(db, *ticket);
        }
    }

    void doMyTickets() {
        const auto tickets = booking.ticketsFor(user);

        if (tickets.empty()) {
            std::cout << "No bookings yet.\n";
        }

        for (const Ticket* ticket : tickets) {
            std::cout << ticket->pnr
                      << "  train " << ticket->trainNumber
                      << "  " << ticket->journeyDate
                      << "  " << ticket->classCode
                      << "  " << toString(ticket->status)
                      << "  Rs." << ticket->totalFare << '\n';
        }
    }

    void doCancel() {
        const auto result = cancelSvc.cancel(
            ask("PNR to cancel: "),
            user
        );

        std::cout << result.message << '\n';

        for (const auto& pnr : result.promotedPnrs) {
            std::cout << "  Waiting-list update: PNR "
                      << pnr << " moved up.\n";
        }

        if (result.ok) {
            persist();
        }
    }

    void doListTrains() const {
        std::cout << "\n--- Train List ---\n";

        if (db.trains.empty()) {
            std::cout << "No trains found.\n";
            return;
        }

        for (const auto& train : db.trains) {
            std::cout << "\nTrain number: " << train.number
                      << "\nName: " << train.name
                      << "\nRoute: " << train.source
                      << " -> " << train.destination
                      << "\nDeparture: " << train.departure
                      << "\nStatus: "
                      << (train.active ? "Active" : "Inactive")
                      << "\nClasses:\n";

            for (const auto& cls : train.classes) {
                std::cout << "  " << cls.code
                          << " | Fare: Rs." << cls.fare
                          << " | Seats: " << cls.totalSeats
                          << " | RAC: " << cls.racSeats
                          << " | WL: " << cls.wlLimit
                          << '\n';
            }
        }
    }

    void doAddTrain() {
        try {
            Train train;

            std::cout << "\n--- Add Train ---\n";
            std::cout << "Enter CNCL at any prompt to cancel.\n";

            train.number = askAddTrainIntRange("Train number: ", 1, 999999);
            train.name = askAddTrainNonEmpty("Train name: ");
            train.source = askAddTrainNonEmpty("Source station: ");
            train.destination = askAddTrainNonEmpty("Destination station: ");
            train.departure = askAddTrainTime("Departure time (HH:MM): ");

            const int classCount = askAddTrainIntRange(
                "Number of classes (1-10): ", 1, 10
            );

            for (int i = 0; i < classCount; ++i) {
                ClassInfo cls;

                std::cout << "\nClass " << i + 1 << '\n';

                while (true) {
                    cls.code = askAddTrainNonEmpty(
                        "Class code (SL/3A/2A/CC/EC): "
                    );

                    std::transform(
                        cls.code.begin(),
                        cls.code.end(),
                        cls.code.begin(),
                        [](unsigned char c) {
                            return static_cast<char>(std::toupper(c));
                        }
                    );

                    const bool validCode =
                        cls.code == "SL" ||
                        cls.code == "3A" ||
                        cls.code == "2A" ||
                        cls.code == "CC" ||
                        cls.code == "EC";

                    const bool duplicate = std::any_of(
                        train.classes.begin(),
                        train.classes.end(),
                        [&](const ClassInfo& existing) {
                            return existing.code == cls.code;
                        }
                    );

                    if (validCode && !duplicate) {
                        break;
                    }

                    if (!validCode) {
                        std::cout << "Invalid class code. Choose SL, 3A, 2A, CC, or EC.\n";
                    } else {
                        std::cout << "This class has already been added. Choose another.\n";
                    }
                }

                cls.fare = askAddTrainIntRange("Fare (Rs.): ", 1, 1000000);
                cls.totalSeats = askAddTrainIntRange(
                    "Confirmed-seat quota: ", 1, 100000
                );
                cls.racSeats = askAddTrainIntRange(
                    "RAC quota: ", 0, 100000
                );
                cls.wlLimit = askAddTrainIntRange(
                    "Waitlist limit: ", 0, 100000
                );

                train.classes.push_back(cls);
            }

            const auto result = adminService.addTrain(train);

            std::cout << result.message << '\n';

            if (result.ok) {
                persist();
                std::cout << "Train added and saved successfully.\n";
            }
        } catch (const AddTrainCancelled&) {
            std::cout << "\nAdd Train cancelled. No changes saved.\n";
        }
    }

    void doModifyTrain() {
        std::cout << "\n--- Modify Train ---\n";

        const int trainNumber = askIntRange(
            "Train number to modify (0 to cancel): ", 0, 999999
        );

        if (trainNumber == 0) {
            std::cout << "Modification cancelled.\n";
            return;
        }

        const Train* existing = findTrain(db, trainNumber);

        if (!existing) {
            std::cout << "Train not found.\n";
            return;
        }

        Train updated = *existing;

        std::cout << "Enter updated train details:\n";

        updated.name = askNonEmpty("Train name: ");
        updated.source = askNonEmpty("Source station: ");
        updated.destination = askNonEmpty("Destination station: ");
        updated.departure = askTime("Departure time (HH:MM): ");

        const auto result = adminService.modifyTrain(
            trainNumber,
            updated
        );

        std::cout << result.message << '\n';

        if (result.ok) {
            persist();
            std::cout << "Train updated and saved successfully.\n";
        }
    }

    void doDeactivateTrain() {
        std::cout << "\n--- Deactivate Train ---\n";

        const int trainNumber = askIntRange(
            "Train number to deactivate (0 to cancel): ", 0, 999999
        );

        if (trainNumber == 0) {
            std::cout << "Deactivation cancelled.\n";
            return;
        }

        const std::string confirmation = ask(
            "Deactivate train " + std::to_string(trainNumber) +
            "? Type YES to confirm: "
        );

        if (confirmation != "YES") {
            std::cout << "Deactivation cancelled.\n";
            return;
        }

        const auto result = adminService.deactivateTrain(trainNumber);

        std::cout << result.message << '\n';

        if (result.ok) {
            persist();
            std::cout << "Train deactivated and saved successfully.\n";
        }
    }

    void doUpdateFareAndQuota() {
        std::cout << "\n--- Current trains and classes ---\n";

        for (const auto& train : db.trains) {
            std::cout << "\nTrain " << train.number
                      << " - " << train.name
                      << (train.active ? " [Active]" : " [Inactive]")
                      << '\n';

            for (const auto& cls : train.classes) {
                std::cout << "  Class: " << cls.code
                          << " | Fare: Rs." << cls.fare
                          << " | Confirmed seats: " << cls.totalSeats
                          << " | RAC: " << cls.racSeats
                          << " | WL limit: " << cls.wlLimit
                          << '\n';
            }
        }

        const int trainNumber = askIntRange(
            "\nTrain number to update (0 to cancel): ", 0, 999999
        );

        if (trainNumber == 0) {
            std::cout << "Update cancelled.\n";
            return;
        }

        const std::string classCode = askNonEmpty("Class code: ");

        const int fare = askIntRange("New fare (Rs.): ", 1, 1000000);
        const int totalSeats = askIntRange(
            "New confirmed-seat quota: ", 1, 100000
        );
        const int racSeats = askIntRange("New RAC quota: ", 0, 100000);
        const int wlLimit = askIntRange("New waitlist limit: ", 0, 100000);

        const auto result = adminService.updateClassFareAndQuota(
            trainNumber,
            classCode,
            fare,
            totalSeats,
            racSeats,
            wlLimit
        );

        std::cout << result.message << '\n';

        if (result.ok) {
            persist();
            std::cout << "Updated configuration saved successfully.\n";
        } else {
            std::cout << "No changes were applied.\n";
        }
    }

    void doCreateBackup() {
        std::string error;

        // Save the latest in-memory data before creating the backup.
        if (!storage.save(db, &error)) {
            std::cout << "Backup failed: could not save current data: "
                      << error << '\n';
            return;
        }

        std::string backupPath;

        if (!storage.createBackup(backupPath, &error)) {
            std::cout << "Backup failed: " << error << '\n';
            return;
        }

        std::cout << "Backup created successfully.\n"
                  << "Location: " << backupPath << '\n';
    }

    void doRestoreBackup() {
        const std::string backupPath = askNonEmpty(
            "Enter backup directory path: "
        );

        if (backupPath.empty()) {
            std::cout << "Restore cancelled: path cannot be empty.\n";
            return;
        }

        const std::string confirmation = ask(
            "This will replace current application data. "
            "Type RESTORE to confirm: "
        );

        if (confirmation != "RESTORE") {
            std::cout << "Restore cancelled.\n";
            return;
        }

        std::string error;

        if (!storage.restoreBackup(backupPath, &error)) {
            std::cout << "Restore failed: " << error << '\n';
            return;
        }

        LoadReport report;

        if (!storage.load(db, &report)) {
            user.clear();
            admin = false;
            std::cout << "Backup was restored, but reloading application data failed. "
                        "Please restart the application.\n";
            return;
        }

        user.clear();
        admin = false;

        std::cout << "Backup restored successfully.\n"
                  << "You have been logged out. Please log in again.\n";

        if (report.skippedLines > 0) {
            std::cout << "Warning: " << report.skippedLines
                      << " record(s) were skipped while loading.\n";
        }
    }

    void guestMenu() {
        std::cout << "\n--- Railway Reservation System ---\n"
                     "1. Register\n"
                     "2. Passenger login\n"
                     "3. Admin login\n"
                     "4. Search trains\n"
                     "5. PNR status\n"
                     "0. Exit\n";

        switch (askInt("Choice: ")) {
            case 1:
                doRegister();
                break;
            case 2:
                doLogin(false);
                break;
            case 3:
                doLogin(true);
                break;
            case 4:
                doSearch();
                break;
            case 5:
                doStatus();
                break;
            case 0:
                throw EofExit{};
            default:
                std::cout << "Please enter a number from the menu.\n";
        }
    }

    void passengerMenu() {
        std::cout << "\n--- Welcome, " << user << " ---\n"
                     "1. Search trains\n"
                     "2. Book ticket\n"
                     "3. My bookings\n"
                     "4. Cancel ticket\n"
                     "5. PNR status\n"
                     "0. Logout\n";

        switch (askInt("Choice: ")) {
            case 1:
                doSearch();
                break;
            case 2:
                doBook();
                break;
            case 3:
                doMyTickets();
                break;
            case 4:
                doCancel();
                break;
            case 5:
                doStatus();
                break;
            case 0:
                user.clear();
                break;
            default:
                std::cout << "Please enter a number from the menu.\n";
        }
    }

    void adminMenu() {
        std::cout << "\n--- Admin console (" << user << ") ---\n"
                     "1. List trains\n"
                     "2. Add train\n"
                     "3. Modify train\n"
                     "4. Deactivate train\n"
                     "5. Update class fare/quota\n"
                     "6. Create backup\n"
                     "7. Restore backup\n"
                     "0. Logout\n";

        switch (askIntRange("Choice: ", 0, 7)) {
            case 1:
                doListTrains();
                break;
            case 2:
                doAddTrain();
                break;
            case 3:
                doModifyTrain();
                break;
            case 4:
                doDeactivateTrain();
                break;
            case 5:
                doUpdateFareAndQuota();
                break;
            case 6:
                doCreateBackup();
                break;
            case 7:
                doRestoreBackup();
                break;
            case 0:
                user.clear();
                admin = false;
                break;
            default:
                std::cout << "Please enter a number from the menu.\n";
        }
    }


    void run() {
        LoadReport report;
        storage.load(db, &report);

        if (report.skippedLines) {
            std::cout << "Note: " << report.skippedLines
                      << " corrupt record(s) were skipped.\n";
        }

        if (db.trains.empty() || !findUser(db, "admin")) {
            seedDatabase(db);
            persist();
        }

        try {
            while (true) {
                if (user.empty()) {
                    guestMenu();
                } else if (admin) {
                    adminMenu();
                } else {
                    passengerMenu();
                }
            }
        } catch (const EofExit&) {
            std::cout << "\nGoodbye.\n";
        }

        persist();
    }
};

int main(int argc, char** argv) {
    App app(argc > 1 ? argv[1] : "data");
    app.run();
    return 0;
}
