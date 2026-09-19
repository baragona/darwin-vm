# Settings launch reaches SpringBoard but fails to start

The completed settings-open-reason-v74.txt request returned LS_OPEN_RESULT=0
and OPEN_EXIT=1. Read-only observers saw the request enter both SpringBoard
open-application handlers. LaunchServices exposes error 115 in
LSApplicationWorkspaceErrorDomain, with only a generic launch failure message.

Observing the earlier completion at runtime 0x18fa5eb18 (unslid 0x186f4ab18,
cache slide 0x8b14000) preserves the original NSError before that wrapping:

- FBSOpenApplicationServiceErrorDomain, code 3: ProcessExited;
  "Process failed to launch."
- Underlying FBProcessExit, code 64: launch-failed.
- Further underlying RBSRequestErrorDomain, code 5.

See settings-open-original-error-v74.json. The bounded object reader stopped
at depth 5, so the innermost RunningBoard diagnostic remains undecoded.
The request-ID read error is a limitation of decoding a tagged object, not
evidence of a guest memory fault. This narrows the failure to process launch;
it does not identify its ultimate cause or establish that Settings ran.
Missing SplashBoard/CommCenter service messages alone do not establish causality.
No launch result was overridden during these observations.
