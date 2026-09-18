#!/bin/bash
printf '\nUI_PROBE_BEGIN\n'
uname -v
id
printf '\nCACHE_DIAGNOSTICS_BEGIN\n'
/bin/cache-diagnostics
/bin/stat -c '%u:%g %n' /System/Cryptexes/OS/System/Library/Caches/com.apple.dyld/dyld_shared_cache_arm64e
printf '\nIOREG_GPU_BEGIN\n'
/usr/sbin/ioreg -r -c IOAccelerator -l
printf '\nIOREG_GPU_EXIT=%s\n' "$?"
printf '\nBACKBOARDD_BEGIN\n'
/bin/timeout 30 /usr/libexec/backboardd
printf '\nBACKBOARDD_EXIT=%s\n' "$?"
printf '\nKERNEL_LOG_BEGIN\n'
/bin/cache-diagnostics dump
printf '\nUI_PROBE_END\n'
