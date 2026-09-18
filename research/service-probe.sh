#!/bin/bash
printf '\nUI_PROBE_BEGIN\n'
id
/bin/launch-probe list
for service in com.apple.iohideventsystem com.apple.CARenderServer com.apple.system.notification_center com.apple.cfprefsd.daemon.system com.apple.cfprefsd.daemon; do
  /bin/launch-probe lookup "$service"
done
/bin/launch-probe list
/bin/display-probe
printf '\nDISPLAY_PROBE_EXIT=%s\n' "$?"
printf '\nUI_PROBE_END\n'
