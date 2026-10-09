// Railway Reservation System - console front-end (Sprint 1).
#include <filesystem>
#include <iomanip>
#include <iostream>

#include "rrs/auth.h"
#include "rrs/booking.h"
#include "rrs/cancel.h"
#include "rrs/search.h"
#include "rrs/storage.h"
#include "rrs/util.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace rrs;

struct EofExit {};

static std::string ask(const std::string& p) {
    std::cout << p << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) throw EofExit{};
    return trim(s);
}

// Masked input for passwords / card numbers (RRS-SR-002)
static std::string askMasked(const std::string& p) {
    std::cout << p << std::flush;
    std::string s;
#ifdef _WIN32
    int c;
    while ((c = _getch()) != '\r' && c != '\n') {
        if (c == 8) { if (!s.empty()) { s.pop_back(); std::cout << "\b \b"; } }
        else { s.push_back(static_cast<char>(c)); std::cout << '*'; }
    }
    std::cout << "\n";
#else
    if (isatty(STDIN_FILENO)) {
        termios oldt{}, newt{};
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        bool ok = static_cast<bool>(std::getline(std::cin, s));
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << "\n";
        if (!ok) throw EofExit{};
    } else if (!std::getline(std::cin, s)) throw EofExit{};
#endif
    return trim(s);
}

static int askInt(const std::string& p, int def = -1) {
    std::string s = ask(p);
    try { size_t pos; int v = std::stoi(s, &pos); if (pos == s.size()) return v; } catch (...) {}
    return def;
}

struct App {
    Database db;
    Storage storage;
    Clock clock = [] { return currentMinutes(); };
    AuthService auth;
    PaymentService pay;
    BookingService booking;
    CancelService cancelSvc;
    SearchService search;
    std::string user;  // logged-in username
    bool admin = false;

    explicit App(const std::string& dir)
        : storage(dir), auth(db, clock), booking(db, pay, clock), cancelSvc(db, clock), search(db, clock) {}

    void persist() {
        std::string err;
        if (!storage.save(db, &err)) std::cout << "WARNING: could not save data: " << err << "\n";
    }

    void doRegister() {
        std::string u = ask("Choose username: ");
        std::string p = askMasked("Choose password: ");
        auto r = auth.registerUser(u, p);
        std::cout << r.message << "\n";
        if (r.ok) persist();
    }

    void doLogin(bool adminLogin) {
        std::string u = ask("Username: ");
        std::string p = askMasked("Password: ");
        auto r = auth.login(u, p, adminLogin);
        std::cout << r.message << "\n";
        persist();  // failed-attempt counters are persisted too
        if (r.ok) { user = findUser(db, u)->username; admin = adminLogin; }
    }

    void doSearch() {
        SearchQuery q{ask("From: "), ask("To: "), ask("Date (YYYY-MM-DD): "), ask("Class (blank = all): ")};
        auto r = search.search(q);
        if (!r.ok || r.rows.empty()) { std::cout << r.message << "\n"; return; }
        std::cout << std::left << std::setw(7) << "Train" << std::setw(22) << "Name" << std::setw(7) << "Dep"
                  << std::setw(6) << "Cls" << std::setw(8) << "Fare" << "Availability\n";
        for (const auto& w : r.rows)
            std::cout << std::setw(7) << w.trainNumber << std::setw(22) << w.trainName << std::setw(7)
                      << w.departure << std::setw(6) << w.classCode << std::setw(8) << w.fare
                      << w.availability << "\n";
    }

    void doBook() {
        BookingRequest req;
        req.username = user;
        req.trainNumber = askInt("Train number: ");
        req.classCode = ask("Class: ");
        req.journeyDate = ask("Date (YYYY-MM-DD): ");
        int n = askInt("Number of passengers (1-" + std::to_string(kMaxPassengersPerBooking) + "): ", 0);
        for (int i = 1; i <= n && i <= kMaxPassengersPerBooking; ++i) {
            Passenger p;
            p.name = ask("  Passenger " + std::to_string(i) + " name: ");
            p.age = askInt("  Age: ");
            std::string g = ask("  Gender (M/F/O): ");
            p.gender = g.empty() ? '?' : static_cast<char>(std::toupper(static_cast<unsigned char>(g[0])));
            req.passengers.push_back(p);
        }
        req.cardNumber = askMasked("Dummy card number (16 digits): ");
        auto r = booking.book(req);
        if (!r.ok) { std::cout << "Booking failed: " << r.message << "\n"; return; }
        persist();
        std::cout << r.message << "\n" << formatReceipt(db, r.ticket, r.txnId);
    }

    void doStatus() {
        const Ticket* t = lookupPnr(db, ask("PNR: "));
        if (!t) std::cout << "PNR not found.\n";
        else std::cout << formatReceipt(db, *t);
    }

    void doMyTickets() {
        auto v = booking.ticketsFor(user);
        if (v.empty()) std::cout << "No bookings yet.\n";
        for (const Ticket* t : v)
            std::cout << t->pnr << "  train " << t->trainNumber << "  " << t->journeyDate << "  "
                      << t->classCode << "  " << toString(t->status) << "  Rs." << t->totalFare << "\n";
    }

    void doCancel() {
        auto r = cancelSvc.cancel(ask("PNR to cancel: "), user);
        std::cout << r.message << "\n";
        for (const auto& p : r.promotedPnrs) std::cout << "  Waiting-list update: PNR " << p << " moved up.\n";
        if (r.ok) persist();
    }

    void guestMenu() {
        std::cout << "\n--- Railway Reservation System ---\n1. Register\n2. Passenger login\n3. Admin login\n"
                     "4. Search trains\n5. PNR status\n0. Exit\n";
        switch (askInt("Choice: ")) {
            case 1: doRegister(); break;
            case 2: doLogin(false); break;
            case 3: doLogin(true); break;
            case 4: doSearch(); break;
            case 5: doStatus(); break;
            case 0: throw EofExit{};
            default: std::cout << "Please enter a number from the menu.\n";
        }
    }

    void passengerMenu() {
        std::cout << "\n--- Welcome, " << user << " ---\n1. Search trains\n2. Book ticket\n3. My bookings\n"
                     "4. Cancel ticket\n5. PNR status\n0. Logout\n";
        switch (askInt("Choice: ")) {
            case 1: doSearch(); break;
            case 2: doBook(); break;
            case 3: doMyTickets(); break;
            case 4: doCancel(); break;
            case 5: doStatus(); break;
            case 0: user.clear(); break;
            default: std::cout << "Please enter a number from the menu.\n";
        }
    }

    void adminMenu() {
        std::cout << "\n--- Admin console (" << user << ") ---\n"
                     "Train/fare/quota management and reports arrive in Sprint 2.\n0. Logout\n";
        if (askInt("Choice: ") == 0) { user.clear(); admin = false; }
    }

    void run() {
        LoadReport rep;
        storage.load(db, &rep);
        if (rep.skippedLines) std::cout << "Note: " << rep.skippedLines << " corrupt record(s) were skipped.\n";
        if (db.trains.empty() || !findUser(db, "admin")) { seedDatabase(db); persist(); }
        try {
            while (true) {
                if (user.empty()) guestMenu();
                else if (admin) adminMenu();
                else passengerMenu();
            }
        } catch (const EofExit&) { std::cout << "\nGoodbye.\n"; }
        persist();
    }
};

int main(int argc, char** argv) {
    App app(argc > 1 ? argv[1] : "data");
    app.run();
    return 0;
}
