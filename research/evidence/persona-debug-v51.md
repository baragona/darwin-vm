# Populated persona manifest, v51 / iOS 27 24A437

Generated with `python3 research/make-persona-seed.py /tmp/a19-persona-v51.plist`.
Installed at `/var-seed/keybags/persona.kb` in service-root.dmg, copied by the
existing writable-var initialization to `/private/var/keybags/persona.kb`.
SHA-256: `f3f5554801aa5468ebe0e0fa71ff4cfdcf6db66465ab85ba4b18d39a77f5987b`.
Size 2635 bytes. No executable or trust-cache changes in this experiment.

UserManager PID 38 stays running. Installer PID 46 initially fails with the
same MIInstallerErrorDomain 4 / underlying ENOENT / SourceFileLine 207.
A normal SpringBoard query completes but returns INVALID, isInstalled=0,
bundleURL=nil. See the accompanying serial transcripts.

## Read-only inspection and two one-shot argument substitutions

Guest cache slide: 0x12294000. UserManager executable base: 0x102b74000,
confirmed by thread-probe's main return 0x102b7d980 (unslid 0x100009980).
Task inspection required a temporary guest developer-mode byte override:
0xfffffe0027ea1600 points to 0xfffffe0017088db4; read 0, set 1, sampled
PID 38, restored 0 and read back. This did not enable a host policy.

The installer uses the embedded `fetchPersonaListforPid:withCompletionHandler:`
implementation at 0x100004ae4, not the other implementation at 0x100020f98
or the all-users method. Its block at 0x100004f5c resolves a user identifier
through 0x1000032c0, then requests that user's personas through 0x1000052e8.
At runtime 0x102b79050, block fields +0x28/+0x2c/+0x30/+0x34 read:
65, 0xffffffff, 0xffffffff, 0xffffffff. At 0x102b79064, x0 (resolved user
identifier) is nil. Static inspection shows the resolver consults kernel
persona information; missing caller persona/session context is a separate
problem from having a populated on-disk manifest.

Manager global 0x102c5b398 -> 0x75bcc10040. Its _state at +0x20 ->
0x1034cd700; _userPersonas at state+8 -> 0x1034cf010. Dictionary storage
at 0x75bcc08000 contains the actual parsed string keys:

- 0x75bd010370: CE7B39AA-4774-5BA4-BD1C-A29AB22DB754 (synthetic mobile user)
- 0x75bd010410: FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF (system session)

For PID 65, at 0x102b79064 changed x0 from nil to the original executable's
system-session CFString at 0x102c4f7e8, verified its characters through
0x102c2d62e. The subsequent lookup returned array 0x75bcc04120 at
0x102b79074. Installer still failed at line 207. No claim that this fixes
kernel personas or the client response.

For PID 70, at the same lookup changed x0 from nil to the existing parsed
mobile-user NSString at 0x75bd010370. At 0x102b79140, immediately after
the array count call, x0=4 and x19=0x75bcc04090. Thus the four synthetic
mobile personas really reached live daemon state; this is not inferred from
file contents alone. Installer still failed at line 207 afterward. The
client response/conversion and any later user-specific lookups need tracing;
do not assume the initial missing caller UUID is the only blocker.

All hardware breakpoints deleted, debugger detached, developer byte restored.
Installer removed to prevent crash loops. No VM reboot during these comparisons.
No registration success or graphical UI claimed.
