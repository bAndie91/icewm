export DISPLAY=:79
state() {
  echo "    bars:"; for id in $(timeout 10 xdotool search --name '^TaskBar$'); do
    x=$(timeout 5 xwininfo -id $id | awk '/Absolute upper-left X/{print $4}')
    k=$(timeout 10 xwininfo -id $id -children | grep -oE '"(Keyboard|CPU-1|Clock)"' | tr -d '"' | sort | tr '\n' ' ')
    echo "      x=$x: $k"; done | sort
  python3 /tmp/panes.py | sed 's/^/    /'
  pgrep -x icewm >/dev/null && echo "    icewm alive" || echo "    *** icewm DIED ***"
}
change() { echo "$1" > /tmp/fake_n; timeout 10 xrandr --fb 1200x600 >/dev/null 2>&1; sleep 1; timeout 10 xrandr --fb 1280x600 >/dev/null 2>&1; sleep 2; }
