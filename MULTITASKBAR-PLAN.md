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
   now; Piece 5's hotplug rebuild reconciles them (done, below).
5. **[DONE]** D5: hotplug. `YWMApp::rebuildTaskBarsIfNeeded()`, called
   from `YWindowManager::updateScreenSize()` after the monitor info is
   refreshed. Rather than rebuilding on every RandR event, each bar set
   records a *signature* at creation (`TaskBar::recordSignature()`: bar
   count, primary screen index, and the screen each of the 8 pinned
   singleton prefs resolved to) and `needsRebuild()` recomputes it. Same
   layout (e.g. only a resolution change) -> just `updateLocationAll()`
   as before, so the system tray etc. aren't disturbed; different ->
   full teardown and recreate. The signature includes the primary index
   because a single bar is now bound to a screen *index* (Piece 1) and
   would otherwise stay on a stale index when hotplug shifts it.
   Because the signature re-resolves every `*Screen` pref each time, a
   widget pinned to a monitor that's absent falls back to primary and
   **moves back when that monitor returns** (verified).
   Teardown order matters and is the reason for the care here: (1)
   `removeAppStatus()` on every frame, so no frame keeps a button
   pointer into a pane about to die (this also resolves the "parallel
   arrays only grow" caveat from Piece 4); (2) for each bar,
   `frame->unmanage(); delete frame;` — the bar is a managed client of
   its own `TaskBarFrame`, and `~YFrameWindow` would otherwise `delete`
   the bar itself (shutdown does the same dance); (3) `delete bar`;
   (4) `createTaskBar()`, which re-adds buttons via `updateAppStatus()`.
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
7. **[PARTLY DONE]** Release notes and a real-hardware pass.
   - `NEWS` entry: **deliberately not hand-edited.** `NEWS` is generated
     from `git shortlog` by `gennews.sh` at release time, so a manual
     entry would just be overwritten. The user-visible surface is
     documented in `man/icewm-preferences.pod` instead (all
     preferences, including the maintainer's `TaskBarShowKeyboard` /
     `KeyboardCommand`, are there). If upstream wants a hand-written
     summary, it belongs in the pull-request description.
   - Simulated-hardware pass under Xvfb: done, see "Testing" below.
   - Real multi-monitor hardware pass: **still TODO, and only the
     maintainer can do it** (no RandR hardware in the sandbox).

## Follow-ups after Piece 4 (found by testing / review)

- **Segfault (`745adb5`, fixed by the maintainer):** `updateLayout()`
  dereferenced `fMailBoxControl`/`fCPUStatus`/`fNetStatus` when the
  *pref* was on, but Piece 3 made construction also depend on
  `hostsSingleton()`, so a bar not hosting the widget had a null
  pointer. Root cause was Piece 3 (`0451368`), not Piece 4. Reproduced
  under the test harness (kernel log showed a null-field read) and
  confirmed fixed. Lesson: gate on the pointer, not the pref.
- **Singleton actions routed to the wrong bar:** the same Piece 3
  change silently broke keyboard-layout updates, the address-bar hotkey
  and system-tray docking whenever the widget was pinned off the
  primary bar (they still went through the compat `taskBar`).
  Fixed with `keyboardUpdateAll()`, `showAddressBarOnHost()`,
  `detachDesktopTrayAll()`, `windowTrayRequestDockAny()`.
- **Buttons follow windows across monitors** (with
  `TaskBarWindowsHomeScreenOnly`): the maintainer's `07939a8` does this
  from `moveWindow()` (interactive drags) via the deferred
  `updateTaskBar()` queue. `YFrameWindow::configure()` now does the same
  for *every* geometry change (client-requested moves, keyboard,
  maximize/tile) using the same deferred path, tracking the routed
  screen in `fHomeScreen`. The two are compatible; `07939a8` is now
  redundant for the common case but harmless and can stay or go.

## Maintainer changes after Piece 5 (already on this branch)

- `46be8c4` `TaskBarShowKeyboard` (default on) and `KeyboardCommand`
  (default `setxkbmap`): the keyboard indicator can be hidden
  independently of `KeyboardLayouts`, and the layout-switch command is
  configurable. Still gated by `hostsSingleton(taskBarKeyboardScreen)`,
  so it composes with the per-widget monitor pinning from Piece 3.
- `0061566` `afterManage()` now also calls `updateWorkArea()` when the
  managed client is one of the taskbars, so each bar's strut is applied
  as soon as it is managed. Verified: see the strut check below.

## Testing

Builds clean with CMake. Exercised at runtime under Xvfb with synthetic
monitors (`contrib/multitaskbar-test/`, a test-only patch — never ship
it): multi-bar startup, per-bar `_NET_WM_STRUT_PARTIAL`, singleton
widgets pinned to non-primary and to non-existent monitors (fall back to
primary), default vs home-screen-only task buttons, buttons following a
client-requested move, and 10 rapid hotplug cycles (1-3 monitors) with
pinned widgets and open windows. Re-run on 2026-09-29 against
`0061566` (clean CMake build, no warnings): 2/3/1 monitors with
per-bar `_NET_WM_STRUT_PARTIAL` (each bar's interval covers only its own
monitor, e.g. 0-425 / 426-851 / 852-1277 for three); keyboard indicator
pinned to monitor 1 falls back to the primary at 1 monitor and returns
at 2; task buttons are re-created on every bar after each rebuild
(5 hotplug cycles); `TaskBarWindowsHomeScreenOnly` shows one button per
bar, moves the button to the other bar on a client-requested move, and
merges/splits correctly across hotplug; 5 hotplug cycles with the start
menu opened first did not crash (weak check: it confirms icewm stayed
alive, not that the menu was still open when the screen changed).
The test hook itself is in `contrib/multitaskbar-test/fake-screens.patch`
(apply with `git apply`, never commit the result). Test-script gotcha:
`run_test.sh` starts xeyes/xclock without `setsid`, so they die when the
calling shell exits; launch them with `setsid` when driving the harness
across several commands.
**Not** exercised: real RandR
hardware, interactive titlebar drags (my synthetic drag missed the
titlebar), a docked tray icon surviving a rebuild, whether an open menu
is actually torn down cleanly (vs. merely not crashing), and
per-monitor DPI.

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

## Follow-on feature: place new windows on the monitor the user is looking at

Pref `NewWindowScreenPolicy` (0 = legacy, default; 1; 2). Implemented in
`YWindowManager::getNewWindowScreen()` (wmmgr.cc), used by `placeWindow()`
only for windows that do not request a position themselves (no
USPosition/PPosition, or `foIgnorePosition`); explicit geometry from
window options or the client still wins.

- **Why not "monitor of the last input event":** a window manager only
  sees input that is grabbed (key bindings) or delivered to its own
  windows (frames, taskbar, menus). Key/button/motion events going to
  clients are invisible without XInput2 raw events, and even then the
  input is often unrelated to the window that appears later.
- **Heuristic chain (policy 2):** (1) owner (`WM_TRANSIENT_FOR`) frame's
  monitor; (2) monitor of the most recently focused frame with the same
  `WM_CLIENT_LEADER`; (3) whichever input device was used last: the mouse pointer's monitor
  (`XQueryPointer` at map time) if pointer activity is newer, the focused
  window's monitor if keyboard activity is newer; (4) focused window /
  primary. Policy 1 skips step 2. Policy 0 is
  the old behavior (focused window, else owner, else primary).
- **Fix needed along the way:** `getCascadePlace()` and the `CenterLarge`
  branch ignored the chosen monitor (they used the frame's not-yet-placed
  geometry); both now receive it. `getSmartPlace()` already honored it.
- **Tested** under Xvfb with 2 synthetic monitors
  (`contrib/multitaskbar-test/fake-screens.patch`, applied locally, not
  committed): policy 0 ignores the pointer; policy 2 puts a plain window
  where the pointer is, a `WM_TRANSIENT_FOR` dialog with its owner on the
  other monitor, and a same-`WM_CLIENT_LEADER` window with its sibling.
  Not tested on real RandR hardware.
- **Possible follow-ups:** remember the screen of the last icewm-visible
  input (key binding, taskbar/menu click) and prefer it for a couple of
  seconds so a launcher keybinding beats a resting pointer; use the same
  chooser for `getSwitchScreen()` if the quick-switch popup should follow
  the pointer too; per-monitor cascade counters instead of one shared
  `fCascadeX/Y`.

### Keyboard vs. pointer recency (step 3)

The core protocol has no "time since last key / last motion" query.
`XScreenSaver` and the XSync `IDLETIME` counter are global, and the
per-device `DEVICEIDLETIME <id>` XSync counters exist but were measured
(Xvfb, XTEST input) to reset all together, so they cannot tell the
devices apart. What works: XInput2 raw events (`XI_RawKeyPress`,
`XI_RawButtonPress`; motion is no longer used, see below) selected on the root window for all
master devices are delivered regardless of focus and grabs, with a server
timestamp. `YWMApp::initInputTracking()` selects them,
`YWMApp::filterEvent()` records the newest key and pointer times, and
`keyboardUsedLastNotPointer()` compares them. Optional dependency
(`CONFIG_XINPUT2`, needs libXi >= 1.2; CMake and configure.ac, autotools
path not built here); without it, or before any input was seen, step 3
falls back to the pointer. Tested under Xvfb/XTEST: after a keypress the
new window follows the focused window even with the pointer on the other
monitor; after mouse movement it follows the pointer.
Only `XI_RawButtonPress` (clicks and wheel) counts as pointer activity; `XI_RawMotion` is deliberately not selected, so a bumped mouse does not override typing.

### Click that does not take focus, then keyboard (bug found by the user)

With "keyboard used last -> focused window's monitor", clicking the root
window or a taskbar on monitor 2 and then launching something by key
(binding, run dialog) still placed the window on the focused window's
monitor 1, because those clicks do not move focus. Root/taskbar click
alone already worked. Fix: on each raw button press `YWMApp` records the
monitor under the pointer and the focused client at that moment;
`lastClickScreenWithoutFocusChange()` returns that monitor only while the
focus is still the same client (a click on an application window changes
focus right after the raw event, so then the focused window is trusted).
Used by `getNewWindowScreen()` only in the keyboard-newer branch.
Tested (Xvfb, 2 monitors): focus on monitor 1, root click / wheel /
taskbar click on monitor 2, then a key -> monitor 2; click on an app on
monitor 2, then a key -> monitor 2; focus on B (monitor 2), root click on
monitor 1, key -> monitor 1; earlier owner/group/typing/motion cases
unchanged. Residual: a wheel turn over an unfocused window on another
monitor followed by typing into the (still focused) window on the first
monitor is placed on the wheel's monitor.

### Input tracking robustness and diagnostics

Hardening of the input-recency tracking for real hardware, which the
Xvfb/XTEST test setup cannot exercise (no /dev/uinput in the sandbox):
- Input recency is ordered by arrival in icewm's event loop (`fKeySeq`,
  `fClickSeq`, `fFocusSeq`) instead of device timestamps or a comparison
  of the focused client at click time. Focus changes are counted in
  `switchFocusTo()/switchFocusFrom()`. A click newer than the last focus
  change wins over the focused window when a key follows it.
- Smooth-scroll devices (libinput touchpads, most modern wheels) report
  the wheel as scroll-valuator motion rather than `XI_RawButtonPress`.
  `XI_RawMotion` is therefore selected, only when some device has
  `XIScrollClass` axes (reloaded on `XI_HierarchyChanged`), and counts
  only when a scroll axis is in the event's valuator mask. Plain motion
  still does not count. Not testable under Xvfb (no scroll axes).
- `ICEWM_PLACEMENT_DEBUG=1 icewm` logs raw key/click events with their
  monitor and each placement decision with the reason.
All earlier Xvfb scenarios still pass.
