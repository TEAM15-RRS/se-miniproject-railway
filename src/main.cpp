
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

static std::string ask(const std::string& prompt) {
    std::cout << prompt << std::flush;

    std::string value;
    if (!std::getline(std::cin, value)) {
        throw EofExit{};
    }

    return trim(value);
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
                adminService.setAdminUser(user);
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

        const int trainNumber = askInt(
            "\nTrain number to update (0 to cancel): ", -1
        );

        if (trainNumber == 0) {
            std::cout << "Update cancelled.\n";
            return;
        }

        if (trainNumber < 0) {
            std::cout << "Invalid train number.\n";
            return;
        }

        const std::string classCode = ask("Class code: ");

        const int fare = askInt("New fare (Rs.): ", -1);
        const int totalSeats = askInt(
            "New confirmed-seat quota: ", -1
        );
        const int racSeats = askInt("New RAC quota: ", -1);
        const int wlLimit = askInt("New waitlist limit: ", -1);

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
                     "1. View and update class fare/quota\n"
                     "0. Logout\n";

        switch (askInt("Choice: ")) {
            case 1:
                doUpdateFareAndQuota();
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
