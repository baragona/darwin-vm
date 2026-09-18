#!/bin/bash
printf '\nUI_PROBE_BEGIN\n'
id
/bin/launch-probe list
for service in com.apple.iohideventsystem com.apple.CARenderServer com.apple.system.notification_center com.apple.cfprefsd.daemon.system com.apple.cfprefsd.daemon; do
  /bin/launch-probe lookup "$service"
done
/bin/launch-probe list
/bin/display-probe &
display_pid=$!
printf '\nDISPLAY_PID=%s\n' "$display_pid"
/bin/sleep 10
/bin/cache-diagnostics devmode
/bin/thread-probe "$display_pid"
/bin/launch-probe list | while read -r tag label pid_field rest; do
  if [ "$tag" = JOB ] && [ "$label" = com.apple.backboardd ]; then
    /bin/thread-probe "${pid_field#PID=}"
  fi
done
printf '\nUI_PROBE_END\n'
