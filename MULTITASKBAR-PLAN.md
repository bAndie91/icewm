# Multi-taskbar feature — design decisions and work plan

Goal: one IceWM taskbar per monitor, with task buttons that can optionally
be restricted to the monitor a window lives on, some widgets (start menu,
toolbar, clock, ...) replicated on every bar, and other widgets (systray,
keyboard indicator, meters, ...) each independently assignable to a chosen
monitor.

This file is the source of truth for cross-session continuity: read it
first when picking this back up.

## Design decisions

- **D1 — Scope.** New bool pref `TaskBarShowOnAllMonitors` (default off,
  preserves today's single-taskbar-on-primary behavior). When on, one
  `TaskBar` instance is created per active Xinerama/RandR screen. No
  per-screen include/exclude list for now.

- **D2 — Replicated vs. singleton widgets.**
  - Replicated on every bar: start menu button, `ObjectBar` toolbar,
    clock, window-list-menu button, show-desktop button, workspace pager.
  - Singleton, each independently placeable on a chosen monitor via its
    own preference (number, or an xrandr output name like `DP-1`; empty
    = fall back to `XineramaPrimaryScreen`/`XRRPrimaryScreenName` as
    today): system tray, mailbox status, CPU/MEM/Net status, APM/battery,
    keyboard-layout indicator, address bar. Proposed pref names (final
    naming to match whatever each widget's existing `TaskBarShow*` pref
    is called):
    - `TaskBarMailboxScreen`
    - `TaskBarCPUStatusScreen`
    - `TaskBarMEMStatusScreen`
    - `TaskBarNetStatusScreen`
    - `TaskBarAPMScreen`
    - `TaskBarKeyboardScreen`
    - `TaskBarSystemTrayScreen`
    - `TaskBarAddressBarScreen`
  - These all reuse the generic resolver added in Piece 1,
    `YDesktop::screenFromDescriptor()`.

- **D3 — Window-button filtering.** A new, separate bool pref
  `TaskBarWindowsHomeScreenOnly` (default **off** — every bar shows every
  window, matching today's behavior extended across bars). When on, each
  bar only lists frames whose home monitor (already available for free
  via the existing `YFrameWindow::getScreen()` / `YDesktop::
  getScreenForRect()` largest-overlap logic — no new tracking code
  needed) matches its own screen index. Fully decoupled from
  `TaskBarShowAllWindows`, which keeps its current, purely
  workspace-scoped meaning.

- **D4 — Struts/workarea.** Every taskbar instance reserves its own
  `_NET_WM_STRUT_PARTIAL` on its own monitor (not deferred — this is in
  scope for v1). Workarea/maximize computation in `wmmgr.cc`
  (`NetWorkAreaBehaviour` and friends) needs to move from "one strut
  source" to "N strut sources, looked up by monitor."

- **D5 — Hotplug.** On monitor add/remove (the existing
  `updateXineramaInfo()` RandR-change path in `wmmgr.cc`), tear down and
  rebuild the whole `TaskBar` set rather than diffing incrementally.

## Pieces

Each piece is sized to be doable in one sitting. Status below.

1. **[DONE]** Screen enumeration + `TaskBar` array skeleton + generic
   monitor-descriptor resolver + `TaskBarShowOnAllMonitors`. See commit
   on this branch. `taskBar` (singular) remains as a compatibility
   pointer to the primary-screen instance for the ~58 call sites not
   yet updated — those are Piece 2.
2. **[TODO]** Walk the ~58 `taskBar->` call sites in `wmmgr.cc`,
   `wmframe.cc`, `decorate.cc`, `movesize.cc`, `amailbox.cc`,
   `amemstatus.cc`, `apppstatus.cc`, `wmapp.cc`; route each to the
   primary instance or broadcast to all instances as appropriate.
3. **[TODO]** D2: implement the replicate/singleton applet split in
   `TaskBar::initApplets()`/`initToolbar()`, plus the 8 per-widget
   `*Screen` preferences.
4. **[TODO]** D3: `TaskBarWindowsHomeScreenOnly` pref; gate task-button
   routing in `atasks.cc` / `TaskBar::addTasksApp` / `delistFrame` on
   `frame->getScreen()` vs. this bar's screen index.
5. **[TODO]** D5: hotplug — rebuild `TaskBar` set on monitor
   add/remove.
6. **[TODO]** D4: per-monitor strut reservation + workarea calc in
   `wmmgr.cc`.
7. **[TODO]** Docs (`man/icewm-preferences.5`), `NEWS` entry, manual QA
   pass (hotplug, drag-across-monitors, restart, single-monitor
   fallback).

## Build notes

Configure + build verified in a throwaway sandbox via CMake:

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)" icewm
```

Needed packages beyond a bare Debian/Ubuntu base: `cmake
libxrandr-dev libxinerama-dev libxpm-dev libxft-dev libfontconfig1-dev
libxcursor-dev libxres-dev libsndfile1-dev libfribidi-dev
libxcomposite-dev libxdamage-dev libxfixes-dev libimlib2-dev
libasound2-dev gettext`. Piece 1 builds clean with no new warnings.
