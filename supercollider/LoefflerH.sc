LoefflerH {
    classvar server, clock, click, commands, directions, centrecodes;
    classvar arduini, ardlisteners, ardmap;
    classvar path, score, downbeatnote=69, beatnote=65, startbar=0;
    classvar window, barbox, playbutton;

    *init { arg showGui=true;
        server = Server.default;
        path = Platform.case(
            \osx, { "~/Documents/loeffler-h/supercollider/score.csv".standardizePath },
            \linux, { "~/build/arduino/loeffler-h/supercollider/score.csv".standardizePath }
        );
        commands = Dictionary[$t -> 3, $G -> 4, $T -> 5, $d -> 6, $g -> 7, $q -> 14];
        directions = Dictionary[$r -> 8, $l -> 9];
        centrecodes = Dictionary[$c -> 10, $n -> 11];
        arduini = Array.newClear(4);
        ardlisteners = Array.newClear(4);
        ardmap = Array.newClear(4);
        this.addSynthDefs;
        this.openSerial;
        this.listenSerial;
        this.mapArduini;
        if (showGui, { this.gui; });
    }

    *addSynthDefs {
         SynthDef(\click, { arg t_trig=0, note=69, level=0, delay=0.08;
             var sig;
             sig = RLPF.ar(Decay.ar(TDelay.ar(
                     Trig.ar(t_trig, 0), delay), 0.05), note.midicps, 0.1);
             Out.ar(0, sig * level.dbamp);
         }).add;
    }

    *openSerial {
        Platform.case(
            \osx, { SerialPort.devicePattern = "/dev/tty.usbmodem*"; },
            \linux, { SerialPort.devicePattern = "/dev/ttyACM*"; }
        );
        SerialPort.devices.do { arg dev, i;
            arduini[i] = SerialPort(dev, baudrate: 38400, crtscts: true);
        };

    }

    *listenSerial {
        4.do { arg ardNum;
            if (arduini[ardNum].notNil, {
                ardlisteners[ardNum] = Routine.run({
                    var byte;
                    loop {
                        while ({ byte = arduini[ardNum].read; byte.notNil }, {
                            switch (byte,
                                6, { "arduino 0 ready (%)".format(ardNum).postln;
                                     ardmap[0] = ardNum; },
                                7, { "arduino 1 ready (%)".format(ardNum).postln;
                                     ardmap[1] = ardNum; },
                                8, { "arduino 2 ready (%)".format(ardNum).postln;
                                     ardmap[2] = ardNum; },
                                9, { "arduino 3 ready (%)".format(ardNum).postln;
                                     ardmap[3] = ardNum; }
                             );
                         });
                     };
                 });
             }, { "no plugged arduino %".format(ardNum).postln; });
         };
    }

    *initArduini {
        // reset to defaults
        4.do { arg ardNum;
            if (ardmap[ardNum].notNil, { arduini[ardmap[ardNum]].putAll([13, 0]); });
        };
    }

    *putArduino { arg ardNum, cmd;
        if (ardmap[ardNum].notNil, { arduini[ardmap[ardNum]].putAll(cmd ++ [0]); });
    }

    *mapArduini {
         ardmap = Array.newClear(4);
         Routine.run {
             4.do { arg ardNum;
                try
                { arduini[ardNum].putAll([2, 0]); }
                { "no plugged arduino %".format(ardNum).postln; }
             };
             3.wait;
             "arduino map: %".format(ardmap).postln;
         };
    }

    *testArduini {
        Routine.run {
            this.mapArduini;
            1.wait;
            4.do { arg ardNum;
                this.doAction(ardNum, "T 2 l 72 1", 60);
                0.5.wait;
            };
        };
    }

    *parsePitches { arg pitches;
        /* midi pitches 36-72 map to:
            startpitch  40-112
              endpitch 120-192 */
        var pitchesout = Array.new();
        pitches.split($-).do { arg p, i;
            pitchesout = pitchesout.add((((p.asFloat.clip(36, 72) - 36) * 2) + 40 +
                                         (80 * (i > 0).asInteger)).asInteger);
        };
        ^pitchesout;
    }

    *parseDuration { arg duration, tempo, durtype=12;
        // set duration in ms, padded to 5 digits
        // durtype: 12=normal duration; 15=gliss end note duration
        var durationout = Array.new();
        duration.split($-).do { arg d, i;
            var dur = (d.asFloat * (60 / tempo) * 1000).asInteger;
            durationout = durationout.add([durtype] ++ dur.asStringToBase(10, 5).ascii);
        };
        ^durationout.flatten;
    }

    *parseAction { arg action, tempo;
        var out, command, halfturns, direction, pitches, duration, recentre;
        if (action == "h", { action = "t 1 r 72"; });
        if (action[0] == $S, {
            ^this.parseDuration(action.split($ )[1], tempo, 15);
        });
        action.split($ ).do { arg w, i;
            var c = w[0];
            if (c.isDecDigit, {
                case
                { halfturns.isNil } { halfturns = w.asInteger.clip(1, 19) + 20 }
                { pitches.isNil } { pitches = this.parsePitches(w); }
                { duration.isNil } { duration = this.parseDuration(w, tempo); };
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
        if (halfturns.isNil, { out = [21]; }, { out = [halfturns]; });
        if (direction.notNil, { out = out.add(direction); });
        if (pitches.notNil, { out = out ++ pitches; });
        if (recentre.notNil, { out = out.add(recentre); });
        if (duration.notNil, { out = out ++ duration; });
        if (command.notNil, { out = out.add(command); });
        ^out;
    }

    *doAction { arg ardNum, action, tempo;
        var cmd;
        //"arduino %: %".format(ardNum, action).postln;
        if (ardmap[ardNum].notNil, {
            cmd = this.parseAction(action, tempo);
            if (cmd.size > 14, {
                Routine.run {
                    arduini[ardmap[ardNum]].putAll(cmd[0..15]);
                    arduini[ardmap[ardNum]].putAll(cmd[16..] ++ [0]);
                };
            }, {
                arduini[ardmap[ardNum]].putAll(cmd ++ [0]);
            });
            //"% (hex: %)".format(action, cmd.collect(_.asHexString(2)).join("")).postln;
        });
    }

    *play { arg start=0, end=inf, rate=100;
        var data, currentbar, currenttempo, currentmeter, currentbeat, currentclickdb, startfound;
        this.initArduini;
        click = Synth(\click);
        clock = TempoClock.new(queueSize: 16384);
        data = CSVFileReader.read(path);
        startfound = false;
        startbar = start;
        currentbar = start;
        currenttempo = 60;
        currentmeter = 4;
        currentbeat = 0;
        currentclickdb = 0;
        block { arg break;
            data.drop(1).do { arg row;
                var bar, tempo, meter, beat, clickdb, actions;
                # bar, tempo, meter, beat, clickdb = row;
                actions = row[5..8];
                if (bar.asInteger > end, { break.value; });
                if (bar.asInteger == startbar, {
                    startfound = true;
                    // force schedule of tempo/meter/clickdb
                    if (meter == "", { meter = currentmeter; });
                    if (tempo == "", { tempo = currenttempo; });
                    if (clickdb == "", { clickdb = currentclickdb; });
                    currentmeter = 0;
                    currenttempo = 0;
                    currentclickdb = -100;
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
                            clock.tempo_((tempo.asFloat * (rate / 100)) / 60);
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
                    4.do { arg ardNum;
                        if ((actions[ardNum] != "").and(actions[ardNum].notNil), {
                            clock.schedAbs(currentbeat + beat.asFloat - 1, {
                                this.doAction(ardNum,
                                    actions[ardNum].stripWhiteSpace, clock.tempo * 60);
                                nil
                            });
                        });
                    };
                });
                if ((clickdb != "").and(clickdb.asFloat != currentclickdb), {
                    currentclickdb = clickdb.asFloat;
                    if (startfound, {
                        "scheduling clickdb change to % dB".format(clickdb.asFloat).postln;
                        clock.schedAbs(currentbeat, {
                            click.set(\level, clickdb.asFloat);
                            "set clickdb % dB".format(clickdb.asFloat).postln;
                            nil
                        });
                    });
                });
            };
        };
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
        var testbutton, bartext;
        window = Window.new("Loeffler: H", bounds: Rect(0, 0, 300, 100));
        testbutton = Button(window)
            .states_([["test arduini", Color.black, Color.white]])
            .action_({ arg button; this.testArduini; });
        bartext = StaticText(window).align_(\right).font_(Font(\Sans, 20)).string_("bar");
        barbox = NumberBox(window).step_(1).clipLo_(0)
        .align_(\center).font_(Font(\Sans, 20)).value_(startbar);
        playbutton = Button(window).states_([
            ["play", Color.black, Color.white],
            ["stop", Color.white, Color.red]])
            .action_({ arg button;
                switch (button.value,
                    0, { this.stop; },
                    1, { this.play(barbox.value.asInteger); });
            }).focus;
        window.layout = VLayout(
            testbutton,
            HLayout(bartext, barbox),
            playbutton
        );
        window.front;
    }

    *free {
        this.stop;
        if (window.notNil, { { window.close; window.free; }.defer; });
        ardlisteners.do { arg al; if (al.notNil, { al.stop; al.free; }); };
        arduini.do { arg ard; if (ard.notNil, { ard.close; ard.free; }); };
    }
}
