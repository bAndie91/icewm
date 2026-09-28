# Multi-taskbar test harness (no multi-monitor hardware needed)

Xvfb cannot present several real monitors to IceWM, so `fake-screens.patch`
adds a **test-only** hook to `YDesktop::updateXineramaInfo()` (do NOT commit
or ship it): if `ICEWM_FAKE_SCREENS_FILE` names a file, its integer N is used
to split the root window into N equal synthetic monitors, re-read on every
screen-change event. Changing the file and then resizing the framebuffer
(`xrandr --fb 1200x600`, then back) fires a real RRScreenChangeNotify, which
simulates monitor hotplug end to end.

    git apply contrib/multitaskbar-test/fake-screens.patch   # then rebuild
    Xvfb :79 -screen 0 1280x600x24 &                          # needs x11-apps,
    echo 2 > /tmp/fake_n                                      # xdotool, x11-utils
    printf 'TaskBarShowOnAllMonitors=1\n' > /tmp/icecfg/preferences
    contrib/multitaskbar-test/run_test.sh   # starts icewm + xeyes + xclock
    python3 contrib/multitaskbar-test/panes.py   # task buttons per bar
    . contrib/multitaskbar-test/hotplug-helpers.sh; change 3; state

Scripts assume display `:79`, `/tmp/icecfg` (ICEWM_PRIVCFG) and a build in
`build/`; adjust as needed. Revert with `git apply -R`.
