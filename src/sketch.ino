/*
 * Motor control for stepper half turns
 */

// motor pins
#define stepPin         9
#define dirPin          7
#define enablePin       5
#define alarmPin        3
#define sensorPin      11
#define idmsbPin       18
#define idlsbPin       19

// speeds
#define pulseWidth      5
#define halfTurnSteps 200 // steps for a half turn
#define accelSteps     90 // number of steps accelerating
#define decelLook      10 // start looking for sensor this many steps early
#define minPulse      412
#define maxPulse     2300
#define durationGlissMul 0.9
#define durationGlissAdd -20

// pitch correction
#define durationTurnMul 1
#define durationTurnAdd 0
#define glissAdd       -0.45

/*************************
 * communication commands
 *************************
 */
#define sReady          6
#define rEOM            0
#define rQuery          2
#define rTurn           3
#define rGliss          4
#define rTimedTurn      5
#define rDurationTurn   6
#define rDurationGliss  7
#define rForwards       8
#define rBackwards      9
#define rRecentre      10
#define rNoRecentre    11
#define rSetDuration   12
#define rReset         13
#define rDoubleGliss   14
#define rSetSustain    15
// array sizes
#define rDurlength      5
#define rLength        25
/*
 * ranges:
    21-39  set halfturns
    40-112 set startpitch
   120-192 set endpitch
   203-211 tests
 */

char serialIncoming[rLength];
char newDuration[rDurlength];
char newSustainDuration[rDurlength];
int halfTurns;
unsigned long Duration;
unsigned long midDuration;
unsigned long sustainDuration;
float startPitch;
float midPitch;
float endPitch;
boolean dir;
boolean recentre;
boolean newData = false;

void setup() {
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(enablePin, OUTPUT);
    pinMode(alarmPin, INPUT_PULLUP);
    pinMode(sensorPin, INPUT_PULLUP);
    pinMode(idmsbPin, INPUT_PULLUP);
    pinMode(idlsbPin, INPUT_PULLUP);
    digitalWrite(dirPin, HIGH);
    digitalWrite(enablePin, LOW);
    Serial.begin(38400);
    findsensor(maxPulse);
    defaults();
    sendid();
}

void defaults() {
    halfTurns = 1;
    Duration = 500;
    midDuration = 500;
    sustainDuration = 50;
    startPitch = 72;
    midPitch = 48;
    endPitch = 60;
    dir = true;
    recentre = true;
}

void sendid() {
    byte id = digitalRead(idmsbPin) << digitalRead(idlsbPin);
    Serial.write(sReady + id);
}

int pitchToPulse(float pitch) {
    /* map MIDI pitch to motor pulse duration */
    float i = pitch - 71;
    float p = 1 - log10((i / 51.035) + 1);
    return max(minPulse, (pow(p, 4.8) * 700) - 260);
}

float pitchbyteToMidi(byte pitch) {
    return (pitch / 2.0) + 36;
}

void onestep(unsigned long pulseTime) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(stepPin, LOW);
    while (pulseTime > 16383) {
        pulseTime -= 1000;
        delay(1);
    }
    delayMicroseconds(pulseTime - pulseWidth);
}

void findsensor(int pulse) {
    while(digitalRead(sensorPin) == LOW) onestep(pulse);
}

void turn(int halfturns, float pitch, bool dir) {
    int fullspeedsteps = (halfTurnSteps * halfturns) - (accelSteps * 2);
    // leave this without durationTurn adjustments
    int pulse = pitchToPulse(pitch);
    int startstoppulse = max(maxPulse, pulse);
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, startstoppulse, pulse));
    }
    for (int i=0; i<fullspeedsteps; i++) {
        onestep(pulse);
    }
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, pulse, startstoppulse));
        if (i>(accelSteps - decelLook) && digitalRead(sensorPin)) break;
    }
    findsensor(startstoppulse);
}

void gliss(int halfturns, float startpitch, float endpitch, bool dir) {
    int glisssteps = (halfTurnSteps * halfturns) - (accelSteps * 2);
    int startpulse = pitchToPulse(startpitch + glissAdd);
    int endpulse = pitchToPulse(endpitch + glissAdd);
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, max(maxPulse, startpulse), startpulse));
    }
    for (int i=0; i<glisssteps; i++) {
        onestep(map(i, 0, glisssteps, startpulse, endpulse));
    }
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, endpulse, max(maxPulse, endpulse)));
        if (i>(accelSteps - decelLook) && digitalRead(sensorPin)) break;
    }
    findsensor(max(maxPulse, endpulse));
}

int timedturn(int halfturns, unsigned long duration, bool dir) {
    int fullspeedsteps = (halfTurnSteps * halfturns) - (accelSteps * 2);
    unsigned long pulse = ((duration * 1000L) - (maxPulse * accelSteps)) /
                          (accelSteps + fullspeedsteps);
    int localAccelSteps = accelSteps;
    if (pulse > maxPulse) {
        localAccelSteps = 0;
        fullspeedsteps = halfTurnSteps * halfturns;
        pulse = (duration * 1000L) / fullspeedsteps;
    } else {
        pulse = max(pulse, minPulse);
    }
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    for (int i=0; i<localAccelSteps; i++) {
        onestep(map(i, 0, localAccelSteps, maxPulse, pulse));
    }
    for (int i=0; i<fullspeedsteps; i++) {
        onestep(pulse);
    }
    for (int i=0; i<localAccelSteps; i++) {
        onestep(map(i, 0, localAccelSteps, pulse, maxPulse));
        if (i>(localAccelSteps - decelLook) && digitalRead(sensorPin)) break;
    }
    findsensor(maxPulse);
    return pulse;
}

long _durationgliss(unsigned long duration, float startpitch, float endpitch,
        int sustainstart, int sustainend, bool dir, bool recentre) {
    int startsustainpulse = pitchToPulse(startpitch + glissAdd - 1);
    int startpulse = pitchToPulse(startpitch + glissAdd);
    int endpulse = pitchToPulse(endpitch + glissAdd);
    int endsustainpulse = pitchToPulse(endpitch + glissAdd - 1);
    int accelsteps = map(startpitch, 48, 72, accelSteps-10, accelSteps+10);
    int decelsteps = map(endpitch, 48, 72, accelSteps-10, accelSteps+10);
    unsigned long glissTime = ((duration - sustainstart) * 1000L) -
        (((max(maxPulse, startpulse) + startpulse) * (accelsteps / 2)) +
         ((endpulse + max(maxPulse, endpulse)) * (decelsteps / 2)));
    glissTime = max(0, (glissTime * durationGlissMul) + (durationGlissAdd * 1000L));
    int glisssteps = glissTime / ((startpulse + endpulse) / 2);
    int sustainstartsteps = (sustainstart * 1000L) / startpulse;
    int sustainendsteps = (sustainend * 1000L) / endpulse;
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    // accelerando
    for (int i=0; i<accelsteps; i++) {
        onestep(map(i, 0, accelsteps, max(maxPulse, startsustainpulse), startsustainpulse));
    }
    // sustain start
    for (int i=0; i<sustainstartsteps; i++) {
        onestep(startsustainpulse);
    }
    // glissando
    for (int i=0; i<glisssteps; i++) {
        onestep(map(i, 0, glisssteps, startpulse, endpulse));
    }
    // sustain end
    for (int i=0; i<sustainendsteps; i++) {
        onestep(endsustainpulse);
    }
    // decelerando
    for (int i=0; i<decelsteps; i++) {
        onestep(map(i, 0, decelsteps, endsustainpulse, max(maxPulse, endsustainpulse)));
    }
    if (recentre) findsensor(max(maxPulse, endsustainpulse));
    return glissTime;
}

void durationturn(unsigned long duration, float pitch, bool dir, bool recentre) {
    _durationgliss(duration, pitch, pitch, 0, 0, dir, recentre);
}

long durationgliss(unsigned long duration, float startpitch, float endpitch,
        bool dir, bool recentre, int sustain) {
    return _durationgliss(duration, startpitch, endpitch, sustain, sustain, dir, recentre);
}

long doublegliss(int durationone, int durationtwo,
        float startpitch, float midpitch, float endpitch,
        int sustainstart, int sustainmid, int sustainend,
        bool dir, bool recentre) {
    int startpulse = pitchToPulse(startpitch + glissAdd);
    int midpulse = pitchToPulse(midpitch + glissAdd);
    int endpulse = pitchToPulse(endpitch + glissAdd);
    int accelsteps = map(startpitch, 48, 72, accelSteps-10, accelSteps+10);
    int decelsteps = map(endpitch, 48, 72, accelSteps-10, accelSteps+10);
    unsigned long glissonetime = ((durationone - sustainstart) * 1000L) -
        ((max(maxPulse, startpulse) + startpulse) * (accelsteps / 2));
    glissonetime = max(0, (glissonetime * durationGlissMul) + (durationGlissAdd * 1000L));
    int glissonesteps = glissonetime / ((startpulse + midpulse) / 2);
    unsigned long glisstwotime = ((durationtwo - sustainstart) * 1000L) -
        ((endpulse + max(maxPulse, endpulse)) * (decelsteps / 2));
    glisstwotime = max(0, (glisstwotime * durationGlissMul) + (durationGlissAdd * 1000L));
    int glisstwosteps = glisstwotime / ((midpulse + endpulse) / 2);
    int startsustainpulse = pitchToPulse(startpitch + glissAdd - 1);
    int sustainstartsteps = (sustainstart * 1000L) / startsustainpulse;
    int midsustainpulse = pitchToPulse(midpitch + glissAdd - 1);
    int sustainmidsteps = (sustainmid * 1000L) / midsustainpulse;
    int endsustainpulse = pitchToPulse(endpitch + glissAdd - 1);
    int sustainendsteps = (sustainend * 1000L) / endsustainpulse;
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    // accelerando
    for (int i=0; i<accelsteps; i++) {
        onestep(map(i, 0, accelsteps, max(maxPulse, startsustainpulse), startsustainpulse));
    }
    // sustain start
    for (int i=0; i<sustainstartsteps; i++) {
        onestep(startsustainpulse);
    }
    // glissando one
    for (int i=0; i<glissonesteps; i++) {
        onestep(map(i, 0, glissonesteps, startpulse, midpulse));
    }
    // sustain mid
    for (int i=0; i<sustainmidsteps; i++) {
        onestep(midsustainpulse);
    }
    // glissando two
    for (int i=0; i<glisstwosteps; i++) {
        onestep(map(i, 0, glisstwosteps, midpulse, endpulse));
    }
    // sustain end
    for (int i=0; i<sustainendsteps; i++) {
        onestep(endsustainpulse);
    }
    // decelerando
    for (int i=0; i<decelsteps; i++) {
        onestep(map(i, 0, decelsteps, endsustainpulse, max(maxPulse, endsustainpulse)));
    }
    if (recentre) findsensor(max(maxPulse, endsustainpulse));
    return glissonetime + glisstwotime;
}

void test(int style) {
    if (style == 0) {
        // turn
        Serial.print("halfturns ");
        Serial.print(halfTurns, DEC);
        Serial.print(" pitch ");
        Serial.print(startPitch, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        turn(halfTurns, startPitch, dir);
        Serial.println(millis() - starttime);
    } else if (style == 1) {
        // gliss
        Serial.print("halfturns ");
        Serial.print(halfTurns, DEC);
        Serial.print(" startpitch ");
        Serial.print(startPitch, DEC);
        Serial.print(" endpitch ");
        Serial.print(endPitch, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        gliss(halfTurns, startPitch, endPitch, dir);
        Serial.println(millis() - starttime);
    } else if (style == 2) {
        // timed turn
        Serial.print("halfturns ");
        Serial.print(halfTurns, DEC);
        Serial.print(" duration ");
        Serial.print(Duration, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        int pulse = timedturn(halfTurns, Duration, dir);
        Serial.print(millis() - starttime);
        Serial.print(" pulse ");
        Serial.println(pulse, DEC);
    } else if (style == 3) {
        // duration turn
        Serial.print("duration ");
        Serial.print(Duration, DEC);
        Serial.print(" pitch ");
        Serial.print(startPitch, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" recentre ");
        Serial.print(recentre, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        durationturn(Duration, startPitch, dir, recentre);
        Serial.println(millis() - starttime);
    } else if (style == 4) {
        // duration gliss
        Serial.print("duration ");
        Serial.print(Duration, DEC);
        Serial.print(" startpitch ");
        Serial.print(startPitch, DEC);
        Serial.print(" endpitch ");
        Serial.print(endPitch, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" recentre ");
        Serial.print(recentre, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        long calcdur = durationgliss(Duration, startPitch, endPitch,
                dir, recentre, sustainDuration);
        Serial.print(millis() - starttime);
        Serial.print(" calctime ");
        Serial.println(calcdur, DEC);
    } else if (style == 5) {
        // double gliss
        Serial.print("midduration ");
        Serial.print(midDuration, DEC);
        Serial.print(", duration ");
        Serial.println(Duration, DEC);
        Serial.print("startpitch ");
        Serial.print(startPitch, DEC);
        Serial.print(" midpitch ");
        Serial.print(midPitch, DEC);
        Serial.print(" endpitch ");
        Serial.print(endPitch, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" recentre ");
        Serial.print(recentre, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        long calcdur = doublegliss(midDuration, Duration,
                startPitch, midPitch, endPitch,
                sustainDuration, sustainDuration, sustainDuration,
                dir, recentre);
        Serial.print(millis() - starttime);
        Serial.print(" calctime ");
        Serial.println(calcdur, DEC);
    } else if (style == 6) {
        // scale
        for (int i=48; i<73; i++) {
            long dur = 1500;
            Serial.print("pitch ");
            Serial.print(i, DEC);
            Serial.print(" duration ");
            Serial.print(dur, DEC);
            Serial.print(" time ");
            unsigned long starttime = millis();
            durationturn(dur, i, i % 2, false);
            Serial.println(millis() - starttime);
            delay(500);
        }
    } else if (style == 7) {
        // arpeggio
        durationturn(4000, 48, true, false);
        delay(1500);
        durationturn(1800, 52, true, false);
        delay(1500);
        durationturn(1800, 55, true, false);
        delay(1500);
        durationturn(4000, 60, true, false);
        delay(1500);
        durationturn(1800, 64, true, false);
        delay(1500);
        durationturn(1800, 67, true, false);
        delay(1500);
        durationturn(4000, 70, true, false);
        delay(1500);
        durationturn(4000, 72, true, false);
    } else if (style == 8) {
        // show settings
        Serial.print("halfturns ");
        Serial.print(halfTurns, DEC);
        Serial.print(", midduration ");
        Serial.print(midDuration, DEC);
        Serial.print(", duration ");
        Serial.println(Duration, DEC);
        Serial.print("startpitch ");
        Serial.print(startPitch, DEC);
        Serial.print(", midpitch ");
        Serial.print(midPitch, DEC);
        Serial.print(", endpitch ");
        Serial.println(endPitch, DEC);
        Serial.print("direction ");
        Serial.print(dir, BIN);
        Serial.print(", recentre ");
        Serial.print(recentre, BIN);
        Serial.print(", sustain ");
        Serial.println(sustainDuration, DEC);
    } else {
        // show test styles
        Serial.println("203: 0xcb: turn");
        Serial.println("204: 0xcc: gliss");
        Serial.println("205: 0xcd: timedturn");
        Serial.println("206: 0xce: durationturn");
        Serial.println("207: 0xcf: durationgliss");
        Serial.println("208: 0xd0: doublegliss");
        Serial.println("209: 0xd1: scale");
        Serial.println("210: 0xd2: arpeggio");
        Serial.println("211: 0xd3: show settings");
    }
    delay(333);
}

void processdata() {
    for (byte i=0; i<rLength; i++) {
        byte data = serialIncoming[i];
        if (data == rEOM) {
            newData = false;
            return;
        } else if (data == rQuery) {
            findsensor(maxPulse);
            sendid();
        } else if (data == rTurn) {
            turn(halfTurns, startPitch, dir);
        } else if (data == rGliss) {
            gliss(halfTurns, startPitch, endPitch, dir);
        } else if (data == rTimedTurn) {
            timedturn(halfTurns, Duration, dir);
        } else if (data == rDurationTurn) {
            durationturn(Duration, startPitch, dir, recentre);
        } else if (data == rDurationGliss) {
            durationgliss(Duration, startPitch, endPitch,
                    dir, recentre, sustainDuration);
        } else if (data == rForwards) {
            dir = true;
        } else if (data == rBackwards) {
            dir = false;
        } else if (data == rRecentre) {
            recentre = true;
        } else if (data == rNoRecentre) {
            recentre = false;
        } else if (data == rSetDuration) {
            for (byte d=0; d<rDurlength; d++) {
                newDuration[d] = serialIncoming[++i];
            }
            midDuration = Duration; // midDuration is previous Duration
            Duration = atol(newDuration);
        } else if (data == rReset) {
            defaults();
        } else if (data == rDoubleGliss) {
            doublegliss(midDuration, Duration,
                    startPitch, midPitch, endPitch,
                    sustainDuration, sustainDuration, sustainDuration,
                    dir, recentre);
        } else if (data == rSetSustain) {
            for (byte d=0; d<rDurlength; d++) {
                newSustainDuration[d] = serialIncoming[++i];
            }
            sustainDuration = atol(newSustainDuration);
        } else if (data < 21) {
        } else if (data < 40) {
            halfTurns = data - 20;
        } else if (data < 113) {
            startPitch = pitchbyteToMidi(data - 40);
        } else if (data < 120) {
        } else if (data < 193) {
            midPitch = endPitch; // midPitch is previous endPitch
            endPitch = pitchbyteToMidi(data - 120);
        } else if (data < 203) {
        } else if (data < 212) {
            test(data - 203);
        }
    }
}

void receivedata() {
    static byte idx = 0;
    char rc;
    if (Serial.available() > 0) {
        rc = Serial.read();
        serialIncoming[idx] = rc;
        if (rc != rEOM) {
            idx++;
            if (idx >= rLength) idx = rLength - 1;
        } else {
            idx = 0;
            newData = true;
        }
    }
}

void loop() {
    /* DEBUG gliss code
    receivedata();
    if (newData) processdata();
    */
    /*
    durationturn(2000, 60, true, false);
    delay(1000);
    durationturn(2000, 67, true, false);
    delay(1000);
    durationturn(2000, 72, true, false);
    delay(1000);
    durationturn(1500, 60, true, false);
    delay(500);
    durationgliss(1500, 60, 72, true, false, 0);
    delay(1500);
    durationgliss(6000, 72, 55, true, false, 900);
    delay(500);
    durationgliss(6000, 55, 72, true, false, 900);
    delay(500);
    doublegliss(6000, 6000, 55, 67, 60, 500, 500, 500, true, false);
    delay(500);
    doublegliss(707, 707, 60, 72, 71, 50, 50, 50, true, false);
    delay(1500);
    */
}
