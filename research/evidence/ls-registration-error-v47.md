# Registration error, 24A437 v47

Cache slide: `0x3278000`. Probe runs as mobile (501), without added entitlements.

- Before: LSApplicationProxy is INVALID, isInstalled=0, bundleURL=nil.
- registerApplication: returns false.
- Fresh process after: identical absent record.
- Temporary hardware breakpoint at `0x186f85c90 + slide` (immediately after
  `_LSRegisterURL`) captures x0=`0xffffffffffffdae5`, signed OSStatus **-9499**.
- At `0x186f01118 + slide`, x21 points to the NSError. Its code field is
  `0xffffffffffffdae5`; its domain CFString decodes to `NSOSStatusErrorDomain`.
  User-info memory contains keys `_LSFile`, `_LSLine`, `_LSFunction` and
  strings `LSRegistration.mm`, `_LSRegisterBundleNode`. Line value not decoded.
- Both debugging sessions deleted their breakpoint and detached; guest resumed.

Static follow-up: `_LSRegisterBundleNode` calls `_LSFindOrRegisterBundleNode`
at 0x186f36a28 and wraps a nonzero result into NSError at 0x186f36ac0.
The latter calls 0x186dea878 and explicitly handles -9499 at 0x186e24098;
which runtime branch supplies the final failure remains unverified.
Do not label this an entitlement denial or installd failure from the number alone.
No changes to Developer Mode, daemon identities, or guest code were needed
for these observations. Virtual LCD and RunningBoard remain unapplied in v47.
