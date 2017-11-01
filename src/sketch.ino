/*
 * Motor control for stepper half turns
 */

// motor pins
#define stepPin        11
#define dirPin         12
#define sensorPin      13

// speeds
#define pulseWidth      5
#define halfTurnSteps 200 // steps for a half turn
#define accelSteps     90 // number of steps accelerating
#define decelLook      10 // start looking for sensor this many steps early
#define minPulse      500
#define maxPulse     2300
#define durationGlissMult 0.935
#define durationGlissDiff 160

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
int halfTurns;
unsigned int duration;
unsigned int midDuration;
float startPitch;
float midPitch;
float endPitch;
boolean dir;
boolean recentre;
boolean newData = false;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(sensorPin, INPUT_PULLUP);
    pinMode(A0, INPUT_PULLUP);
    pinMode(A1, INPUT_PULLUP);
    pinMode(A2, INPUT_PULLUP);
    pinMode(A3, INPUT_PULLUP);
    digitalWrite(dirPin, HIGH);
    Serial.begin(38400);
    findsensor(maxPulse);
    defaults();
    sendid();
}

void defaults() {
    halfTurns = 1;
    duration = 500;
    midDuration = 500;
    startPitch = 72;
    midPitch = 48;
    endPitch = 60;
    dir = true;
    recentre = true;
}

void sendid() {
    if (!digitalRead(A0)) {
        Serial.write(sReady + 0);
    } else if (!digitalRead(A1)) {
        Serial.write(sReady + 1);
    } else if (!digitalRead(A2)) {
        Serial.write(sReady + 2);
    } else if (!digitalRead(A3)) {
        Serial.write(sReady + 3);
    }
}

int pitchToPulse(float pitch) {
    /* map MIDI pitch to motor pulse duration */
    float i = pitch - 71;
    float p = 1 - log10((i / 51.035) + 1);
    return max(412, (pow(p, 4.8) * 700) - 260);
}

float pitchbyteToMidi(byte pitch) {
    return (pitch / 2.0) + 36;
}

void flashled() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
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
    int pulse = pitchToPulse(pitch);
    int startstoppulse = max(maxPulse, pulse);
    digitalWrite(LED_BUILTIN, HIGH);
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
    digitalWrite(LED_BUILTIN, LOW);
}

void gliss(int halfturns, float startpitch, float endpitch, bool dir) {
    int glisssteps = (halfTurnSteps * halfturns) - (accelSteps * 2);
    int startpulse = pitchToPulse(startpitch);
    int endpulse = pitchToPulse(endpitch);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, max(maxPulse, startpulse), startpulse));
    }
    for (int i=0; i<glisssteps; i++) {
        // TODO log map
        onestep(map(i, 0, glisssteps, startpulse, endpulse));
    }
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, endpulse, max(maxPulse, endpulse)));
        if (i>(accelSteps - decelLook) && digitalRead(sensorPin)) break;
    }
    findsensor(max(maxPulse, endpulse));
    digitalWrite(LED_BUILTIN, LOW);
}

int timedturn(int halfturns, int duration, bool dir) {
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
    digitalWrite(LED_BUILTIN, HIGH);
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
    digitalWrite(LED_BUILTIN, LOW);
    return pulse;
}

void durationturn(int duration, float pitch, bool dir, bool recentre) {
    unsigned long stopTime;
    unsigned long startTime = millis();
    int pulse = pitchToPulse(pitch);
    int startstoppulse = max(maxPulse, pulse);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, startstoppulse, pulse));
    }
    stopTime = startTime + duration - (millis() - startTime);
    while(millis() < stopTime) {
        // avoid polling millis too often
        // use map() here to match pitch in _durationgliss
        for (int i=0; i<10; i++) onestep(map(i, 0, 10, pulse, pulse));
    }
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, pulse, startstoppulse));
    }
    if (recentre) findsensor(startstoppulse);
    digitalWrite(LED_BUILTIN, LOW);
}

long _durationgliss(int duration, float startpitch, float endpitch, bool dir,
        bool recentre, bool accel, bool decel) {
    int startpulse = pitchToPulse(startpitch);
    int endpulse = pitchToPulse(endpitch);
    int accelsteps = accelSteps * accel;
    int decelsteps = accelSteps * decel;
    unsigned long glissTime = (duration * 1000L) -
        (((max(maxPulse, startpulse) + startpulse) * (accelsteps / 2)) +
         ((endpulse + max(maxPulse, endpulse)) * (decelsteps / 2)));
    glissTime = max(0, (glissTime * durationGlissMult) - (durationGlissDiff * 1000L));
    int glisssteps = glissTime / ((startpulse + endpulse) / 2);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(dirPin, dir);
    delayMicroseconds(pulseWidth);
    for (int i=0; i<accelsteps; i++) {
        onestep(map(i, 0, accelsteps, max(maxPulse, startpulse), startpulse));
    }
    for (int i=0; i<glisssteps; i++) {
        onestep(map(i, 0, glisssteps, startpulse, endpulse));
    }
    for (int i=0; i<decelsteps; i++) {
        onestep(map(i, 0, decelsteps, endpulse, max(maxPulse, endpulse)));
    }
    if (recentre) findsensor(max(maxPulse, endpulse));
    digitalWrite(LED_BUILTIN, LOW);
    return glissTime;
}

long durationgliss(int duration, float startpitch, float endpitch, bool dir, bool recentre) {
    return _durationgliss(duration, startpitch, endpitch, dir, recentre, true, true);
}

long doublegliss(int durationone, int durationtwo, float startpitch,
        float midpitch, float endpitch, bool dir, bool recentre) {
    long glissTime;
    glissTime = _durationgliss(durationone, startpitch, midpitch, dir, false, true, false);
    glissTime += _durationgliss(durationtwo, midpitch, endpitch, dir, recentre, false, true);
    return glissTime;
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
        Serial.print(duration, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        int pulse = timedturn(halfTurns, duration, dir);
        Serial.print(millis() - starttime);
        Serial.print(" pulse ");
        Serial.println(pulse, DEC);
    } else if (style == 3) {
        // duration turn
        Serial.print("duration ");
        Serial.print(duration, DEC);
        Serial.print(" pitch ");
        Serial.print(startPitch, DEC);
        Serial.print(" direction ");
        Serial.print(dir, DEC);
        Serial.print(" recentre ");
        Serial.print(recentre, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        durationturn(duration, startPitch, dir, recentre);
        Serial.println(millis() - starttime);
    } else if (style == 4) {
        // duration gliss
        Serial.print("duration ");
        Serial.print(duration, DEC);
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
        long calcdur = durationgliss(duration, startPitch, endPitch, dir, recentre);
        Serial.print(millis() - starttime);
        Serial.print(" calctime ");
        Serial.println(calcdur, DEC);
    } else if (style == 5) {
        // double gliss
        Serial.print("midduration ");
        Serial.print(midDuration, DEC);
        Serial.print(", duration ");
        Serial.println(duration, DEC);
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
        long calcdur = doublegliss(midDuration, duration, startPitch, midPitch, endPitch, dir, recentre);
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
        Serial.println(duration, DEC);
        Serial.print("startpitch ");
        Serial.print(startPitch, DEC);
        Serial.print(", midpitch ");
        Serial.print(midPitch, DEC);
        Serial.print(", endpitch ");
        Serial.println(endPitch, DEC);
        Serial.print("direction ");
        Serial.print(dir, BIN);
        Serial.print(", recentre ");
        Serial.println(recentre, BIN);
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
            flashled();
        } else if (data == rTurn) {
            turn(halfTurns, startPitch, dir);
        } else if (data == rGliss) {
            gliss(halfTurns, startPitch, endPitch, dir);
        } else if (data == rTimedTurn) {
            timedturn(halfTurns, duration, dir);
        } else if (data == rDurationTurn) {
            durationturn(duration, startPitch, dir, recentre);
        } else if (data == rDurationGliss) {
            durationgliss(duration, startPitch, endPitch, dir, recentre);
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
            midDuration = duration; // midDuration is previous duration
            duration = atoi(newDuration);
        } else if (data == rReset) {
            defaults();
        } else if (data == rDoubleGliss) {
            doublegliss(midDuration, duration, startPitch, midPitch, endPitch, dir, recentre);
        } else if (data < 21) {
            true;
        } else if (data < 40) {
            halfTurns = data - 20;
        } else if (data < 113) {
            startPitch = pitchbyteToMidi(data - 40);
        } else if (data < 120) {
            true;
        } else if (data < 193) {
            midPitch = endPitch; // midPitch is previous endPitch
            endPitch = pitchbyteToMidi(data - 120);
        } else if (data < 203) {
            true;
        } else if (data < 212) {
            test(data - 203);
            flashled();
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
    receivedata();
    if (newData) processdata();
}
