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
    `YDesktop::screenFromDescriptor()`. That resolver already
    guarantees the fallback-if-missing behavior requested: an
    out-of-range numeric index, an unmatched/unplugged output name, or
    XRandR being unavailable all fall through to the `fallback`
    argument (the primary screen, for these widgets) rather than
    failing — so a hotplug event that removes the monitor a widget was
    pinned to will not crash or misplace it, as long as the pref is
    re-resolved after the monitor set changes rather than cached
    (something Piece 5's hotplug rebuild needs to do for every
    per-widget screen pref, not just re-create the bars themselves).

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
2. **[DONE]** Walked every `taskBar->`/`== taskBar`/`!= taskBar` call
   site in `wmmgr.cc`, `wmframe.cc`, `decorate.cc`, `movesize.cc`,
   `wmapp.cc` (the applet files `amailbox.cc`/`amemstatus.cc`/
   `apppstatus.cc` turned out to need no changes — their
   `MailBoxControl`/`MEMStatus`/etc. classes already store their own
   `IAppletContainer *taskBar` member from construction, which shadows
   the global, so they were already correctly scoped per-instance).
   Two new families of static broadcast helpers on `TaskBar`:
   - `*All()` (workspacesRepaint, workspacesUpdateButtons,
     workspacesRelabelButtons, setWorkspaceActive, updateFullscreen,
     relayout, relayoutNow, refresh, updateLocation, initToolbar,
     handleCollapseButton) — loop over `taskBars`, used for
     everything that's either a replicated widget (pager, toolbar,
     collapse) or a whole-desktop lifecycle event (screen change,
     idle relayout, theme refresh).
   - `whichTaskBar(client)` / `isTaskBar(client)` — replaces the old
     `client() == taskBar` / `!= taskBar` identity checks (14 sites)
     used to exclude the taskbar's own window from focus, raise,
     fullscreen, work-area-affecting, etc. logic. Now checks
     membership in `taskBars` instead of identity with the one
     global.
   - The `restackWindows()` edge-trigger append and the
     `doNotCover()`/hidden check in the work-area strut loop
     (wmmgr.cc) were generalized to consider every instance.
   - Left deliberately on the single compat `taskBar` pointer (not
     broadcast): task/tray button management
     (`addTasksApp`/`addTrayApp`/`relayoutTasks`/`relayoutTray`/
     `updateFrame`/`delistFrame` in wmframe.cc) and singleton-widget
     actions (systray, keyboard indicator, address bar, start-menu/
     window-list-menu hotkey popups, pager prev/next-workspace
     actions) — these still only have real content on one instance
     until Pieces 3/4, so routing them further now would be premature.
   - Builds clean, no new warnings.
3. **[DONE]** D2: implemented the replicate/singleton applet split in
   `TaskBar::initApplets()`, plus the 8 per-widget `*Screen`
   preferences (`TaskBarMailboxScreen`, `TaskBarCPUStatusScreen`,
   `TaskBarMEMStatusScreen`, `TaskBarNetStatusScreen`,
   `TaskBarAPMScreen`, `TaskBarKeyboardScreen`,
   `TaskBarSystemTrayScreen`, `TaskBarAddressBarScreen`). New
   `TaskBar::hostsSingleton(screenPref)` gates each singleton widget's
   construction: always true when there's only one bar
   (`TaskBarShowOnAllMonitors` off), so a stray `*Screen` setting can
   never leave a widget homeless on a single-taskbar setup; otherwise
   true only on the instance matching `screenFromDescriptor()`. The
   replicated widgets (clock, start menu, toolbar, window-list button,
   show-desktop, collapse button, workspace pager, task pane, window
   tray) were untouched — they already get constructed on every
   instance since each `TaskBar` calls its own `initApplets()`.
   Documented all 9 new prefs (this one + the 8 `*Screen` prefs) in
   `man/icewm-preferences.pod`. Builds clean.
4. **[DONE]** D3: added `TaskBarWindowsHomeScreenOnly` (default off).
   `YFrameWindow`'s single `fTaskBarApp`/`fTrayApp` pointers became
   `YArray<TaskBarApp*> fTaskBarApps`/`YArray<TrayApp*> fTrayApps`,
   parallel-indexed to the global `taskBars` array, since a frame can
   now have a button on more than one bar simultaneously (the default:
   every bar shows every window, same as today just multiplied across
   bars). `updateAppStatus()` loops over `taskBars` and, when the pref
   is on, only creates/shows a button on the bar whose `screen()`
   matches the frame's own `getScreen()` (the existing largest-overlap
   helper from `YWindow`/`YDesktop` -- no new tracking code needed, as
   suspected back when this decision was made). `updateTaskBar()`,
   `removeAppStatus()`, and the repaint call sites in `updateTitle()`/
   `updateIconTitle()`/`updateIcon()` all updated to loop accordingly.
   Confirmed `TaskPane::addApp()` (`atasks.cc`) has no shared/static
   state across instances, so multiple independent `TaskPane`s each
   holding their own button for the same frame is safe. Builds clean.
   Caveat noted in code comments: the parallel arrays only grow for
   now; Piece 5's hotplug rebuild will need to reconcile them when
   `taskBars` itself is rebuilt rather than just extended.
5. **[TODO]** D5: hotplug — rebuild `TaskBar` set on monitor
   add/remove.
6. **[DONE]** D4: turned out to need almost no new logic. Confirmed by
   code reading (no multi-monitor test rig available in the sandbox
   this was built in, so this is verified by inspection, not runtime
   testing -- worth a real hotplug/multi-monitor smoke test in Piece 7):
   - `TaskBar::updateWMHints()` already computed its strut from
     `desktop->getScreenGeometry(fScreen)` -- it was one of the three
     call sites Piece 1 already fixed to be screen-aware, before this
     piece even started.
   - `YWindowManager::updateWorkAreaInner()` already attributes every
     window's strut to `w->getScreen()`'s own monitor bounds
     (`xiInfo[s]`), not the whole virtual desktop -- so multiple
     `TaskBar` instances, each correctly positioned per Piece 1, were
     already reserving space only on their own screen with zero
     changes needed here.
   - The one real gap: `updateWMHints()` only published the plain
     `_NET_WM_STRUT` (4 values: left/right/top/bottom), which by EWMH
     spec reserves that margin across the *entire* edge of the desktop,
     not just one monitor's segment of it. Harmless for icewm's own
     placement (which ignores the interval fields and uses
     `getScreen()` instead, confirmed by reading
     `YFrameWindow::updateNetWMStrutPartial()` -- it only extracts
     left/right/top/bottom, discarding the start/end interval), but
     wrong for any *external* EWMH-reading tool. Fixed by also
     publishing `_NET_WM_STRUT_PARTIAL` with `top_start_x`/`top_end_x`
     or `bottom_start_x`/`bottom_end_x` scoped to the bar's own screen
     geometry.
   - Known residual limitation, pre-existing and out of scope here:
     `_NET_WORKAREA` (the property icewm exports summarizing the
     workarea for *other* clients) is a single rect per workspace, not
     per-monitor -- external tools relying on that property alone
     won't see the per-monitor split icewm's own internal logic uses.
     This isn't new; it's the same limitation any Xinerama-era
     multi-monitor icewm setup already had, unrelated to multi-taskbar.
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
