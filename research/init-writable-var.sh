#!/bin/bash
# Disposable restore guest only. /private/var must point to /mnt1.
set -eu
if [ ! -f /mnt1/.darwinvm-var-ready ]; then
    /sbin/mount_tmpfs -s 536870912 /mnt1
    /bin/cp -a /var-seed/. /mnt1/
    /bin/mkdir -p /var/root/Library/Caches /var/mobile/Library/Caches \
        /var/mobile/Library/Preferences /var/run /var/tmp /var/log \
        /var/db /var/containers /var/mobile/Containers /var/empty
    /bin/chown -R 0:0 /mnt1
    /bin/chown -R 501:501 /var/mobile
    /bin/chmod 700 /var/root
    /bin/chmod 755 /var/mobile /var/run /var/db /var/containers /var/empty
    /bin/chmod 1777 /var/tmp
    printf 'writable-var\n' > /var/tmp/darwinvm-write-probe
    /bin/cat /var/tmp/darwinvm-write-probe
    /bin/touch /mnt1/.darwinvm-var-ready
fi
printf '\nWRITABLE_VAR_READY\n'
/sbin/mount
exec /bin/bash -i
