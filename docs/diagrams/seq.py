import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Rectangle

def draw(title, parts, steps, out):
    n = len(parts); W = 2.6
    H = 0.62 * len(steps) + 2.6
    fig, ax = plt.subplots(figsize=(W * n + 0.6, H))
    ax.set_xlim(-1.1, W * (n - 1) + 1.1); ax.set_ylim(-H + 1.4, 1.3); ax.axis("off")
    x = {p: i * W for i, (p, _) in enumerate(parts)}
    for p, label in parts:
        ax.add_patch(FancyBboxPatch((x[p] - 0.95, 0.35), 1.9, 0.6, boxstyle="round,pad=0.02",
                                    fc="#EAF2FB", ec="#2F5D8C", lw=1.3))
        ax.text(x[p], 0.65, label, ha="center", va="center", fontsize=10, fontweight="bold", color="#1b3a5c")
    bottom = -0.62 * len(steps) - 0.5
    for p, _ in parts:
        ax.plot([x[p], x[p]], [0.35, bottom], color="#999999", lw=1, ls=(0, (4, 3)), zorder=0)
    y = -0.1; num = 0
    for s in steps:
        kind = s[0]
        if kind == "frame":      # ('frame', text)
            ax.add_patch(Rectangle((-1.0, y - 0.2), W * (n - 1) + 2.0, 0.4, fc="#FFF4D6", ec="#B58900", lw=1))
            ax.text(-0.9, y, s[1], va="center", fontsize=9, style="italic", color="#6b5200")
        else:
            _, a, b, text, dashed = s
            num += 1
            if a == b:
                ax.plot([x[a], x[a] + 0.55, x[a] + 0.55, x[a]], [y + 0.12, y + 0.12, y - 0.12, y - 0.12], color="#222", lw=1.2)
                ax.annotate("", xy=(x[a], y - 0.12), xytext=(x[a] + 0.3, y - 0.12), arrowprops=dict(arrowstyle="->", color="#222"))
                ax.text(x[a] + 0.65, y, f"{num}. {text}", va="center", fontsize=8.5)
            else:
                ax.annotate("", xy=(x[b], y), xytext=(x[a], y),
                            arrowprops=dict(arrowstyle="->", color="#222", lw=1.2, ls="--" if dashed else "-"))
                ax.text((x[a] + x[b]) / 2, y + 0.1, f"{num}. {text}", ha="center", va="bottom", fontsize=8.5,
                        bbox=dict(fc="white", ec="none", pad=0.6))
        y -= 0.62
    ax.set_title(title, fontsize=12, fontweight="bold", pad=6)
    fig.savefig(out, dpi=170, bbox_inches="tight", facecolor="white"); plt.close(fig)

P = lambda *a: ("msg",) + a
draw("Sequence 1 - Book a ticket (RRS-F-020..025)",
     [("P","Passenger"),("UI","Console UI"),("B","BookingService"),("I","Inventory"),("Pay","PaymentService"),("S","Storage")],
     [P("P","UI","Select train, class, date, passengers, card",0),
      P("UI","B","book(request)",0),
      P("B","B","validate user, train, class, date, passengers",0),
      P("B","I","computeSeats(train, class, date)",0),
      P("I","B","confirmed / RAC / WL free",1),
      ("frame","alt  no capacity in any quota  ->  error 'No seats, RAC or WL left'  (nothing stored)"),
      P("B","B","pick status: CONFIRMED, else RAC, else WL",0),
      P("B","Pay","charge(card, fare)",0),
      ("frame","alt  payment declined / invalid card  ->  error returned, nothing stored"),
      P("Pay","B","success + txnId",1),
      P("B","B","create ticket with unique PNR",0),
      P("B","UI","BookingResult(ticket)",1),
      P("UI","S","save(database)  [temp file + rename]",0),
      P("UI","P","e-ticket: PNR, status, fare",1)],
     "seq_booking.png")
draw("Sequence 2 - Cancel a ticket with refund and waitlist promotion (RRS-F-030..033)",
     [("P","Passenger"),("UI","Console UI"),("C","CancelService"),("I","Inventory"),("S","Storage")],
     [P("P","UI","Enter PNR to cancel",0),
      P("UI","C","cancel(pnr, username)",0),
      P("C","C","PNR exists? owner matches? not already cancelled?",0),
      P("C","C","train not yet departed?",0),
      P("C","C","calculateRefund(ticket, departure, now)",0),
      P("C","C","set status CANCELLED (record is kept)",0),
      ("frame","loop  until no further change (FIFO)"),
      P("C","I","computeSeats(train, class, date)",0),
      P("I","C","free berths / RAC slots",1),
      P("C","C","promote RAC -> CONFIRMED, then WL -> RAC / CONFIRMED",0),
      P("C","UI","CancelResult(refund, promoted PNRs)",1),
      P("UI","S","save(database)",0),
      P("UI","P","refund amount + waiting-list updates",1)],
     "seq_cancel.png")
draw("Sequence 3 - Login with lockout (RRS-F-002, RRS-SR-005)",
     [("P","Passenger"),("UI","Console UI"),("A","AuthService"),("S","Storage")],
     [P("P","UI","username + masked password",0),
      P("UI","A","login(username, password)",0),
      ("frame","alt  unknown user  ->  generic 'Invalid username or password'"),
      ("frame","alt  now < lockedUntil  ->  'Account locked, try again in N minutes'"),
      P("A","A","compare SHA-256(salt + password) with stored hash",0),
      ("frame","match -> reset failedAttempts      mismatch -> failedAttempts++ (5th: lock for 15 min)"),
      P("A","UI","success / failure message",1),
      P("UI","S","save(database)  [persists counters, restart cannot bypass lock]",0)],
     "seq_login.png")
