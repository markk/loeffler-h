/* Run Løffler H
place the cursor on the following line and press Cmd-Enter */

s.waitForBoot({ LoefflerH.init; });

/* GUI functions

The four buttons along the top of the window show the status of the four motor
controllers. Each button has four possible states indicating:
    - blank: offline or unplugged
    - green: ready
    - blue: not ready (performing an action)
    - red: alarm (driver error)

The bar number box indicates the current bar which increments during play. Click
in the box to type a bar number to start at, then press enter or tab.

The test button plays a sequence of full turns on each motor in order.

The play button starts playback.

Closing the window will shutdown all playback and close connections to the motor
controllers. */

/* Advanced Functions

More detailed control is available for rehearsal and testing as follows. */

s.boot;                           // make sure the server is booted first

LoefflerH.init(verbose: true);    // log extra information
// NB: do not use this option for performance as it halts playback on all
// errors rather than recovering automatically!

LoefflerH.play;                   // play all
LoefflerH.play(10);               // play from bar 10
LoefflerH.play(10, 15);           // play from bar 10 to the end of bar 15
LoefflerH.play(rate: 85);         // play at 85% tempo
LoefflerH.play(10, 15, rate: 85); // play bar 10 to bar 15 at 85% tempo
LoefflerH.stop;                   // stop playback
LoefflerH.free;                   // shutdown
