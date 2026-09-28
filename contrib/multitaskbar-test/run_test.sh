#!/bin/bash
# usage: run_test.sh  (expects /tmp/icecfg/preferences and /tmp/fake_n)
export DISPLAY=:79
pgrep Xvfb >/dev/null || { (setsid timeout 3000 Xvfb :79 -screen 0 1280x600x24 >/tmp/xvfb3.log 2>&1 &); sleep 3; }
pkill -x icewm; pkill xeyes; pkill xclock; sleep 0.5
cd /home/claude/icewm/build
(ICEWM_PRIVCFG=/tmp/icecfg ICEWM_FAKE_SCREENS_FILE=/tmp/fake_n setsid ./icewm > /tmp/icewm.log 2>&1 &)
sleep 3
alive() { pgrep -x icewm >/dev/null && echo "  [$1] icewm alive" || { echo "  [$1] *** icewm DIED ***"; tail -5 /tmp/icewm.log; return 1; }; }
alive startup || exit 1
(xeyes -geometry 100x100+50+100 >/dev/null 2>&1 &); (xclock -geometry 100x100+800+100 >/dev/null 2>&1 &); sleep 2
alive "2 windows mapped" || exit 1
