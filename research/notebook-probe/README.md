# Field Notes UIKit prototype

An offline iOS 27 app using the same runtime Objective-C approach as Touch
Probe. It contains an editable UITextView, a UIScrollView containing a supplied
PNG image and drawing canvas, and Save and Draw/Scroll buttons. Draw mode
turns off the parent scroll gesture so touches can be used for drawing; switch
back to Scroll to navigate the page. Save dismisses the keyboard.

Notes and up to 8192 normalized drawing points are written atomically as a
versioned property list under the app's Documents directory. Explicit Save,
finishing a stroke, and scene deactivation trigger saving. The UI reports the
write result. A fresh launch loads the file. The ink-v1 record format is three
32-bit fields: float x, float y, uint32 start-of-stroke. Reads validate record
length, finite normalized coordinates, and stroke flags. Saving and loading
still require testing with the guest's actual container and sandbox.

Build with a local iOS libSystem stub and a PNG you can use:

```sh
python3 research/notebook-probe/build.py \
  --libsystem /path/to/libSystem.B.dylib \
  --image /path/to/Photo.png \
  --output /tmp/notebook-build \
  --trustcache /path/to/current.tc
```

The output directory must not exist. The build compiles with strict warnings,
signs and verifies the bundle, records hashes, and optionally extends a
version-1 trust cache. Install Notebook.app under /Applications in a separate
guest-image clone. Register it with LaunchServices, then test both Home-screen
visibility and foreground launch. The bundle identifier is org.baragona.Notebook.

V79 staging uses our own V78 Home-screen capture as the initial image. The
source builds and signs successfully, but no runtime UI, drawing, keyboard,
or persistence claim is made yet. The exact build and image-copy reports are
in ../evidence/notebook-build-v79.json and notebook-staging-v79.json. The clone
also stages Apple's Calculator and missing direct dependency resource bundles;
Calculator launch is likewise pending. No Apple binaries are committed.

Acceptance: enter a new note through real input, scroll to the image and
canvas, draw a recognizable stroke, save, go Home and reopen. Then terminate
and relaunch the app and verify both saved text and drawing to distinguish
storage from mere retention of a running process. Also verify error reporting
when the Documents directory is not writable. A live host frontend and keyboard
transport remain separate emulator work.
