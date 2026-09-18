#!/bin/bash
# Disposable restore guest: /private/var must be a real directory, not a symlink.
set -eu
if [ ! -f /private/var/.darwinvm-var-ready ]; then
    if ! /sbin/mount_tmpfs -s 536870912 /private/var; then
        echo DIRECT_VAR_MOUNT_FAILED
        exec /bin/bash -i
    fi
    /bin/cp -a /var-seed/. /private/var/
    /bin/mkdir -p /var/root/Library/Caches /var/mobile/Library/Caches \
        /var/mobile/Library/Preferences /var/run /var/tmp /var/log \
        /var/db /var/containers /var/mobile/Containers /var/empty
    /bin/chown -R 0:0 /private/var
    /bin/chown -R 501:501 /var/mobile
    /bin/chmod 700 /var/root
    /bin/chmod 755 /var/mobile /var/run /var/db /var/containers /var/empty
    /bin/chmod 1777 /var/tmp
    printf 'writable-var\n' > /var/tmp/darwinvm-write-probe
    /bin/cat /var/tmp/darwinvm-write-probe
    /bin/touch /private/var/.darwinvm-var-ready
fi
printf '\nWRITABLE_VAR_READY\n'
/sbin/mount
exec /bin/bash -i
