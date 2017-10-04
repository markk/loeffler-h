/*
a = LoefflerH.init;
a.play;
a.stop;
a.putArduino(2);
a.putArduino(50);
a.putArduino(Int8Array[]);
*/
LoefflerH {
    classvar server, clock, click;
    classvar arduino1, arduino2, a1listener, a2listener;
    classvar path, score, downbeatnote=69, beatnote=65, startbar=0;
    classvar window, barbox, playbutton, a1button, a2button;

    *init {
        server = Server.default;
        path = "/home/johannes/build/arduino/loeffler-h/supercollider/score.csv";
        this.addSynthDefs;
        this.openSerial;
        this.listenSerialOne;
        //this.gui;
    }

    *openSerial {
        var devicePath;
        Platform.case(
            \osx, { devicePath = "/dev/tty.usbmodem001"; },
            \linux, { devicePath = "/dev/ttyACM0"; }
        );
        if (File.type(devicePath) == \character,
            { arduino1 = SerialPort(devicePath, baudrate: 9600, crtscts: true); },
            { ("*** serial device not found at" + devicePath).postln; }
        );
    }

    *listenSerialOne {
        a1listener = Routine.run({
            var byte;
            loop {
                while ({ byte = arduino1.read; byte.notNil }, {
                    switch (byte,
                        6,  { "ready received".postln; },
                    );
                    byte.asInteger.asString.postln;
                });
            };
        });
    }

    *addSynthDefs {
        SynthDef(\click, { arg t_trig=0, note=69, level=0, delay=0.2;
            var sig;
            sig = RLPF.ar(Decay.ar(
                TDelay.ar(Trig.ar(t_trig, 0), delay),
                0.05), note.midicps, 0.1);
            Out.ar(0, sig * level.dbamp);
        }).add;
    }

    *doAction { arg action1, action2;
        var turns;
        "MOTOR ACTION 1: %; 2: %".format(action1, action2).postln;
        turns = switch (action1,
            "cw", { 1 },
            "ccw", { 2 },
            { action1.asInteger }
        );
        arduino1.put(10 + turns);
    }

    *setSpeed { arg pulse;
        arduino1.put(20 + pulse);
    }

    *testArduino { arg ardNum;
        arduino1.put(2);
    }

    *putallArduino { arg intarray;
        arduino1.putAll(intarray);
    }

    *putArduino { arg byte;
        arduino1.put(byte);
    }

    *play {
        var data, currentbar, currenttempo, currentmeter, currentbeat;
        click = Synth(\click);
        clock = TempoClock.new;
        data = CSVFileReader.read(path);
        startbar = try { barbox.value } { 0 };
        currentbar = startbar;
        currenttempo = 60;
        currentmeter = 4;
        currentbeat = 0;
        data.drop(1).do { arg row;
            var bar, tempo, meter, beat, action1, action2;
            # bar, tempo, meter, beat, action1, action2 = row;
            if (bar.asInteger > currentbar, {
                currentbeat = currentbeat + ((bar.asInteger - currentbar) * currentmeter);
                currentbar = bar.asInteger;
            });
            if ((tempo.asFloat > 0).and(tempo.asFloat != currenttempo), {
                "scheduling tempo change to % BPM".format(tempo.asFloat).postln;
                clock.schedAbs(currentbeat, {
                    clock.tempo_(tempo.asFloat / 60);
                    "set tempo % BPM".format(tempo.asFloat).postln;
                    nil
                });
                currenttempo = tempo.asFloat;
            });
            if ((meter.asInteger > 0).and(meter.asInteger != currentmeter), {
                "scheduling meter change to % BPB".format(meter.asInteger).postln;
                clock.schedAbs(currentbeat, {
                    clock.beatsPerBar_(meter.asInteger);
                    "set meter % BPB".format(meter.asInteger).postln;
                    nil
                });
                currentmeter = meter.asInteger;
            });
            if (beat.asFloat >= 1, {
                clock.schedAbs(currentbeat + beat.asFloat - 1, {
                    this.doAction(action1, action2);
                    nil
                });
            });
            // check max bar
            if (startbar > bar.asInteger, {
                clock.schedAbs(0, {
                    "MAXIMUM BAR NUMBER EXCEEDED".postln;
                    this.stop;
                    try { { barbox.value_(0); }.defer; };
                    nil
                })
            });
        };
        // add click function
        clock.schedAbs(clock.beats.ceil, { arg beat, sec;
            var note = beatnote;
            if (clock.beatInBar == 0, {
                note = downbeatnote;
                try { { barbox.value_(startbar + clock.bar); }.defer; };
            });
            click.set(\t_trig, 1, \note, note);
            "[%] bar %, beat %".format(beat, startbar + clock.bar, clock.beatInBar + 1).postln;
            1
        });
        // schedule stop
        clock.schedAbs(currentbeat + currentmeter, {
            "END OF SCORE".postln;
            this.stop;
            nil
        });
    }

    *stop {
        clock.clear;
        clock.stop;
        clock.free;
        click.release;
        try { {
            playbutton.value_(0);
            barbox.value_(startbar);
        }.defer; };
    }

    *gui {
        var bartext;
        window = Window.new("Loeffler: H", bounds: Rect(0, 0, 300, 100));
        a1button = Button(window)
            .states_([["test arduino 1", Color.black, Color.white]])
            .action_({ arg button; this.testArduino(1); });
        a2button = Button(window)
            .states_([["test arduino 2", Color.black, Color.white]])
            .action_({ arg button; this.testArduino(2); });
        bartext = StaticText(window).align_(\right).font_(Font(\Sans, 20)).string_("bar");
        barbox = NumberBox(window).step_(1).clipLo_(0)
            .align_(\center).font_(Font(\Sans, 20)).value_(startbar);
        playbutton = Button(window).states_([
                    ["play", Color.black, Color.white],
                    ["stop", Color.white, Color.red]])
                .action_({ arg button;
                    switch (button.value,
                        0, { this.stop; },
                        1, { this.play; }
                    );
                }).focus;
        window.layout = VLayout(
            HLayout(a1button, a2button),
            HLayout(bartext, barbox),
            playbutton
        );
        window.front;
    }

    *free {
        this.stop;
        if (window.notNil, {
            { window.close; window.free; }.defer;
        });
        if (a1listener.notNil, {
            a1listener.stop;
            a1listener.free;
        });
        if (arduino1.notNil, {
            arduino1.close;
            arduino1.free;
        });
    }
}
