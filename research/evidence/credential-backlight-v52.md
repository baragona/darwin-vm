# V52: credential failure path and virtual-display filtering

The previous turn left SpringBoard waiting in ACMContextCreateWithFlags.
Guest-only debugger experiments now pass that wait and the next backlight
assertion. They do not establish a usable GUI, valid credentials, or SEP emulation.
Shared-cache slide: 0xfb70000, matching build 24A437.

## Credential creation failure injection

Static inspection of SBFCredentialSet initWithSerializedCredentialSet: shows
its no-serialized-data branch calls ACMContextCreateWithFlags at unslid
0x1bb666120. A nonzero return follows a cleanup path that zeroes the context
and returns nil. At runtime 0x1cb1d6120 the output context slot was already zero.
The diagnostic changed w0 to 0xffffffff and PC to 0x1cb1d6124, skipping this
single call and exercising its existing failure path. This is a synthetic
failure, not an observed return value from Secure Enclave hardware.

The old SpringBoard did not exit immediately after SIGTERM/SIGKILL, but a
replacement eventually launched. Do not equate the initial delayed exit with
proof that a process can never terminate. Later removed instances continued
producing endpoint-wait messages; job removal does not prove every old thread
has disappeared.

SpringBoard PID 387 passed the credential initialization and later SIGTRAPed.
A repeat in PID 394, with kernel exception breakpoint 0xfffffe002ab8e2a4
conditioned on x0==6, captured codes [1, 0x235417ddc]. This maps to
BacklightServicesHost's BLSHBacklightService initializer cold assertion at
unslid 0x2258a7ddc. Its CFString at runtime 0x28372f9e0 pointed to
0x23543f29c (length 59), read directly as:

> No state machines created - no display interfaces available

## Virtual LCD rejected by displayType filter

BLSHBacklightOSInterfaceProvider initWithPlatformProvider: calls `displays`,
then filters them with block 0x2258525dc. The block calls `displayType` and
returns true only for zero. This selector was decoded from its actual objc
stub, not inferred from a symbol name.

In PID 399, after the same credential failure injection, a breakpoint at
0x2353c25f0 (unslid 0x2258525f0, after displayType) captured x0=3. The first
array element was x24=0x0a000001008c8760, index x27=0. Its backing pointer at
object+8 was 0x0200007c1ec98000. CADisplay accessors independently establish:

- displayId reads backing+0x68: runtime value 1, the verified virtual LCD.
- displayType reads backing+0x1e0: runtime value 3.

Changing x0 to zero at just this filter evaluation admits display ID 1.
The breakpoint was then removed; other displays follow their original filter.
The display object/type field was not modified. This is a compatibility
experiment, not proof that virtual displays implement every physical-display
operation BacklightServices expects.

PID 399 advanced past the previous backlight trap, then aborted with stderr:

```text
Could not cast value of type '_NSXPCDistantObject' (0x7c1f17ea20) to 'ExtensionFoundation._EXDiscoveryServiceProtocol' (0x7c1f17ea38).
```

The next investigation is this ExtensionFoundation XPC/protocol failure.
No cause or fix is established from the message alone.

## Evidence and live state

- [Initial restart](credential-restart-v52.txt), [result](credential-error-result-v52.txt).
- [Trap reproduction](credential-trace-restart-v52.txt), [removal request](credential-trace-cleanup-v52.txt).
- [Filter experiment launch](backlight-filter-start-v52.txt), [new abort stderr](backlight-filter-result-v52.txt).

Credential, filter, and exception breakpoints were removed. LLDB remains
attached with only the two previously documented auto-continuing persona
argument substitutions for UserManager PID 352. Guest developer override
remains zero. SpringBoard RemoveJob returned errno 36 while exiting; the
serial log later confirms removal, so it should be verified absent before
resubmission. No host security settings or on-disk firmware code changed.
