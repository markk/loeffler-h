/*
a = LoefflerH.init;
a.gui
a.play;
a.play(2);
a.play(7);
a.stop;
a.putArduino(2);
a.putArduino(50);
a.putallArduino(Int8Array[]);
a.parseAction("d 8 34-72 2 n", 56)

actions
=======

command [halfturns] direction pitch[-endpitch] [duration] [recentre]

commands:
    t = turn            (halfturns, pitch, direction)
    d = duration turn   (duration, pitch, direction, recentre)
    g = duration gliss  (duration, pitch, endpitch, direction, recentre)
    T = timed turn      (halfturns, duration, direction)
    G = gliss           (halfturns, pitch, endpitch, direction)

halfturns:
    1-19 number of halfturns to complete

directions:
    l = left
    r = right

pitches:
    36-72 (including quartertones, e.g. 40.5)

duration:
    in beats at current tempo

recentre:
    c = recentre
    n = do not recentre

*/
LoefflerH {
    classvar server, clock, click, commands, directions, centrecodes;
    classvar arduino1, arduino2, a1listener, a2listener;
    classvar arduino3, arduino4, a3listener, a4listener;
    classvar path, score, downbeatnote=69, beatnote=65, startbar=0;
    classvar window, barbox, playbutton, a1button, a2button;

    *init {
        server = Server.default;
        path = "/home/johannes/build/arduino/loeffler-h/supercollider/score.csv";
        commands = Dictionary[$t -> 3, $G -> 4, $T -> 5, $d -> 6, $g -> 7];
        directions = Dictionary[$l -> 8, $r -> 9];
        centrecodes = Dictionary[$c -> 10, $n -> 11];
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
                });
            };
        });
    }

    *addSynthDefs {
        SynthDef(\click, { arg t_trig=0, note=69, level=0, delay=0.08;
            var sig;
            sig = RLPF.ar(Decay.ar(
                TDelay.ar(Trig.ar(t_trig, 0), delay),
                0.05), note.midicps, 0.1);
            Out.ar(0, sig * level.dbamp);
        }).add;
    }

    *parseAction { arg action, tempo;
        var out, command, halfturns, direction, pitch, endpitch, duration, recentre;
        action.split($ ).do { arg w, i;
            var c = w[0];
            if (c.isDecDigit, {
                case
                { halfturns.isNil } { halfturns = w.asInteger.clip(1, 19) + 20 }
                { pitch.isNil } { // set pitch, endpitch
                    pitch = this.midiToPitchbyte(w.split($-)[0].asFloat);
                    try { endpitch = this.midiToPitchbyte(w.split($-)[1].asFloat, 1); };
                    //"pitch % endpitch %".format(pitch, endpitch).postln;
                }
                { duration.isNil } { // set duration in ms, padded to 5 digits
                    duration = (w.asFloat * (60 / tempo) * 1000).asInteger;
                    duration = [12] ++ duration.asStringToBase(10, 5).ascii;
                };
            }, {
                case
                { commands[c].notNil } { command = commands[c] }
                { directions[c].notNil } {
                    direction = directions[c];
                    if (halfturns.isNil, { halfturns = 21; });
                }
                { centrecodes[c].notNil } { recentre = centrecodes[c] };
            });
        };
        if (pitch.isNil, { pitch = this.midiToPitchbyte(72); });
        out = [halfturns, direction, pitch];
        if (endpitch.notNil, { out = out.add(endpitch); });
        if (recentre.notNil, { out = out.add(recentre); });
        if (duration.notNil, { out = out ++ duration; });
        if (command.notNil, { out = out.add(command); });
        ^out;
    }

    *doAction { arg action, tempo;
        var cmd = this.parseAction(action, tempo);
        "% (hex: %)".format(cmd, cmd.collect(_.asHexString(2))).postln;
        arduino1.putAll(cmd ++ [0]);
    }

    *testArduino { arg ardNum;
        arduino1.put(2);
    }

    *initArduino { arg ardNum;
        // reset to defaults
        arduino1.putAll([13, 0]);
    }

    *putallArduino { arg ardNum, cmd;
        arduino1.putAll(cmd ++ [0]);
    }

    *putArduino { arg ardNum, byte;
        arduino1.put(byte);
    }

    *midiToPitchbyte { arg pitch, endpitch=0;
        /* midi pitches 36-72 map to:
            startpitch  40-112
              endpitch 120-192 */
        ^(((pitch.clip(36, 72) - 36) * 2) + 40 + (80 * endpitch)).asInteger;
    }

    *play { arg start=0;
        var data, currentbar, currenttempo, currentmeter, currentbeat, startfound;
        this.initArduino;
        click = Synth(\click);
        clock = TempoClock.new;
        data = CSVFileReader.read(path);
        startfound = false;
        startbar = start;
        currentbar = start;
        currenttempo = 60;
        currentmeter = 4;
        currentbeat = 0;
        data.drop(1).do { arg row;
            var bar, tempo, meter, beat, action1, action2;
            # bar, tempo, meter, beat, action1, action2 = row;
            if (bar.asInteger == startbar, {
                startfound = true;
                // force schedule of tempo/meter
                meter = currentmeter;
                tempo = currenttempo;
                currentmeter = 0;
                currenttempo = 0;
            });
            if (bar.asInteger > currentbar, {
                currentbeat = currentbeat + ((bar.asInteger - currentbar) * currentmeter);
                currentbar = bar.asInteger;
            });
            if ((tempo.asFloat > 0).and(tempo.asFloat != currenttempo), {
                currenttempo = tempo.asFloat;
                if (startfound, {
                    "scheduling tempo change to % BPM".format(tempo.asFloat).postln;
                    clock.schedAbs(currentbeat, {
                        clock.tempo_(tempo.asFloat / 60);
                        "set tempo % BPM".format(tempo.asFloat).postln;
                        nil
                    });
                });
            });
            if ((meter.asInteger > 0).and(meter.asInteger != currentmeter), {
                currentmeter = meter.asInteger;
                if (startfound, {
                    "scheduling meter change to % BPB".format(meter.asInteger).postln;
                    clock.schedAbs(currentbeat, {
                        clock.beatsPerBar_(meter.asInteger);
                        "set meter % BPB".format(meter.asInteger).postln;
                        nil
                    });
                });
            });
            if ((beat.asFloat >= 1).and(startfound), {
                clock.schedAbs(currentbeat + beat.asFloat - 1, {
                    this.doAction(action1, currenttempo);
                    nil
                });
            });
        };
        // check max bar
        if (startbar > currentbar, {
            clock.schedAbs(0, {
                "MAXIMUM BAR NUMBER EXCEEDED".postln;
                this.stop;
                if (barbox.notNil, { { barbox.value_(0); }.defer; });
                nil
            })
        });
        // add click function
        clock.schedAbs(clock.beats.ceil, { arg beat, sec;
            var note = beatnote;
            if (clock.beatInBar == 0, {
                note = downbeatnote;
                if (barbox.notNil, { { barbox.value_(startbar + clock.bar); }.defer; });
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
        if (playbutton.notNil, { { playbutton.value_(0); }.defer; });
        if (barbox.notNil, { { barbox.value_(startbar); }.defer; });
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
                        1, { this.play(barbox.value.asInteger); }
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
