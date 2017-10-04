a = LoefflerH.init;
a.free
a.play;
a.stop;
a.putArduino(2); // query
a.putArduino(5); // enable
a.putArduino(7); // disable
a.putArduino(3); // accelturn
a.putArduino(4); // glissturn
// 11-20 halfTurns
a.putArduino(15);
// 21-40 minPulse
a.putArduino(21);
// 41-60 maxPulse
a.putArduino(50);
// 61-80 startPulse
a.putArduino(40);
// 81-100 endPulse
a.putArduino(40);
// 101-120 accelSteps
a.putArduino(110);
a.putallArduino(Int8Array[11, 40, 60, 110, 3]);
a.putallArduino(Int8Array[18, 31, 60, 110, 3]);
a.putallArduino(Int8Array[18, 21, 60, 110, 3]);
// accel steps
(
    Routine.run({
        a.putallArduino(Int8Array[15, 21, 50, 61, 90, 101]);
    });
)
// min pulse
(
    Routine.run({
        a.putallArduino(Int8Array[15, 21, 50, 101]);
        (21..25).do({ arg i;
            "% min pulse".format(i).postln;
            a.putallArduino(Int8Array[i, 3]);
            wait(2);
        });
    });
)

a.putallArduino(Int8Array[11, 21, 41, 120, 3]);
a.putallArduino(Int8Array[12, 21, 44, 120, 3]);
a.putallArduino(Int8Array[12, 28, 44, 120, 3]);
a.putallArduino(Int8Array[12, 21, 60, 110, 3]);
a.putallArduino(Int8Array[12, 40, 44, 120, 3]);
a.putallArduino(Int8Array[15, 21, 41, 120, 3]);

a.putallArduino(Int8Array[15, 21, 41, 120, 3]);
a.putallArduino(Int8Array[15, 21, 41, 80, 81, 120, 4]);

a.free
