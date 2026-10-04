# Decision 07: desktop application search and recent-app wheel

2026-10-04. Authorized by the owner's explicit request for Advanced Radial Menu,
eight recent applications and application search. This bounded task (P21-T48)
may proceed before the separate T45 close-request work. It does not complete
general application compatibility or the broader workspace gates.

Use the requested MIT [Advanced Radial Menu](https://github.com/diklor/advanced_radial_menu)
at `decd8679efd19947a54bbe99fc5644846322f745`, vendored locally. The store's 1.4.1
listing is marked unstable; verify our actual configuration in graphical tests.
Retain its license. One local change permits a fractional rotation offset so
eight sectors have their first icon exactly at twelve o'clock.

Use installed Python 3/PyGObject, GIO desktop-entry discovery and GTK 3 icon-theme
lookup in a separate helper process. These are explicit runtime dependencies,
already available here; no installation or host-service change is authorized.
They reuse freedesktop entries, localization, precedence and installed icon
themes without requiring Plasma, KRunner or a running KDE session. Read the KDE
icon-theme preference when available; fall back to Breeze/hicolor. GTK is an
icon resolver here, not the frontend UI. Missing helper dependencies must surface
as an error while navigation and the existing terminal remain usable.

The helper resolves a selected desktop ID again at launch, expands supported
Desktop Entry Exec field codes and execs an argument vector without a shell.
Search text never becomes a command. Launch with the private Wayland socket,
no host DISPLAY/WAYLAND_SOCKET, and no host session-bus activation. Flatpak
launching is deferred until its sandbox/display handoff can be verified.
Terminal=true entries use the existing Weston terminal. Unsupported applications
may fail against the limited compositor; show launch failures honestly.

Persist only eight unique desktop-entry IDs, newest first. PIDs and window
handles remain transient. With one pending launch at a time, a newly mapped
window is the initial presentation candidate; this is not durable identity or
general multiwindow association. Confirm visible content before recording a
launch in recents. Existing app windows can be selected again; full window
management and client close requests remain separate work.

References: [desktop entries](https://specifications.freedesktop.org/desktop-entry/latest/),
[Exec](https://specifications.freedesktop.org/desktop-entry/latest/exec-variables.html),
[icon themes](https://specifications.freedesktop.org/icon-theme/latest/).
