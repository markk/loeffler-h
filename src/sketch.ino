/*
 * Motor control for stepper half turns
 */

// motor pins
#define stepPin         6
#define dirPin          7
#define sensorPin       9

// speeds
#define pulseWidth       5
#define accelSteps      70
#define halfTurnSteps  200 // steps for a half turn
#define minPulse       500
#define maxPulse      2500

#define durationGlissCompensation 480

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
#define rDurlength      5
#define rLength        12
/*
 * ranges:
    13-20  tests
    21-39  set halfturns
    40-112 set startpitch
   120-192 set endpitch
 */

char serialIncoming[rLength];
char newDuration[rDurlength];
int halfTurns         = 1;
unsigned int duration = 500;
float startPitch      = 72;
float endPitch        = 36;
boolean dir           = true;
boolean recentre      = true;
boolean newData       = false;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(sensorPin, INPUT_PULLUP);
    digitalWrite(dirPin, HIGH);
    Serial.begin(9600);
    findsensor(maxPulse);
    //Serial.write(sReady);
    Serial.println("arduino ready");
}

int pitchToPulse(float pitch) {
    /* map MIDI pitch to motor pulse duration */
    float i = ((71 - constrain(pitch, 36, 72)) * 71) / 33;
    float p = 1 - log10(((72 - i) * 0.125) + 1);
    return 500 + (3500 * p);
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
        for (int i=0; i<10; i++) onestep(pulse);
    }
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, pulse, startstoppulse));
    }
    if (recentre) findsensor(startstoppulse);
    digitalWrite(LED_BUILTIN, LOW);
}

long durationgliss(int duration, float startpitch, float endpitch, bool dir, bool recentre) {
    int startpulse = pitchToPulse(startpitch);
    int endpulse = pitchToPulse(endpitch);
    unsigned long glissTime = max(0, (duration * 1000L) -
        (((maxPulse + startpulse) * (accelSteps / 2)) +
         ((endpulse + maxPulse) * (accelSteps / 2))) -
        (durationGlissCompensation * 1000L));
    int glisssteps = glissTime / ((startpulse + endpulse) / 2);
    digitalWrite(LED_BUILTIN, HIGH);
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
    }
    if (recentre) findsensor(max(maxPulse, endpulse));
    digitalWrite(LED_BUILTIN, LOW);
    return glissTime;
}

void test(int style) {
    if (style == 0) {
        // random turn
        int halfturns = random(1, 10);
        int pitch = random(36, 72);
        Serial.print("halfturns ");
        Serial.print(halfturns, DEC);
        Serial.print(" pitch ");
        Serial.print(pitch, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        turn(halfturns, pitch, pitch % 2);
        Serial.println(millis() - starttime);
    } else if (style == 1) {
        // random gliss
        int startpitch = random(36, 72);
        int endpitch = random(36, 72);
        int halfturns = random(1, 10);
        Serial.print("startpitch ");
        Serial.print(startpitch, DEC);
        Serial.print(" endpitch ");
        Serial.print(endpitch, DEC);
        Serial.print(" halfturns ");
        Serial.print(halfturns, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        gliss(halfturns, startpitch, endpitch, halfturns % 2);
        Serial.println(millis() - starttime);
    } else if (style == 2) {
        // random timed turn
        int halfturns = random(1, 4);
        int duration = random(300, 5000);
        Serial.print("halfturns ");
        Serial.print(halfturns, DEC);
        Serial.print(" duration ");
        Serial.print(duration, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        int pulse = timedturn(halfturns, duration, duration % 2);
        Serial.print(millis() - starttime);
        Serial.print(" pulse ");
        Serial.println(pulse, DEC);
    } else if (style == 3) {
        // random duration turn
        int pitch = random(36, 72);
        int duration = random(300, 5000);
        Serial.print("pitch ");
        Serial.print(pitch, DEC);
        Serial.print(" duration ");
        Serial.print(duration, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        durationturn(duration, pitch, duration % 2, false);
        Serial.println(millis() - starttime);
    } else if (style == 4) {
        // random duration gliss
        int startpitch = random(36, 72);
        int endpitch = random(36, 72);
        int duration = random(300, 5000);
        Serial.print("startpitch ");
        Serial.print(startpitch, DEC);
        Serial.print(" endpitch ");
        Serial.print(endpitch, DEC);
        Serial.print(" duration ");
        Serial.print(duration, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        long calcdur = durationgliss(duration, startpitch, endpitch, duration % 2, false);
        Serial.print(millis() - starttime);
        Serial.print(" calctime ");
        Serial.println(calcdur, DEC);
    } else if (style == 5) {
        // scale
        for (int i=36; i<73; i++) {
            long dur = 666;
            Serial.print("pitch ");
            Serial.print(i, DEC);
            Serial.print(" duration ");
            Serial.print(dur, DEC);
            Serial.print(" time ");
            unsigned long starttime = millis();
            durationturn(dur, i, i % 2, false);
            Serial.println(millis() - starttime);
        }
    } else if (style == 6) {
        // show settings
        Serial.print("halfturns ");
        Serial.print(halfTurns, DEC);
        Serial.print(", duration ");
        Serial.println(duration, DEC);
        Serial.print("startpitch ");
        Serial.print(startPitch, DEC);
        Serial.print(", endpitch ");
        Serial.println(endPitch, DEC);
        Serial.print("direction ");
        Serial.print(dir, BIN);
        Serial.print(", recentre ");
        Serial.println(recentre, BIN);
    } else {
        // show test styles
        Serial.println("13: 0x0d: turn");
        Serial.println("14: 0x0e: gliss");
        Serial.println("15: 0x0f: timedturn");
        Serial.println("16: 0x10: durationturn");
        Serial.println("17: 0x11: durationgliss");
        Serial.println("18: 0x12: scale");
        Serial.println("19: 0x13: show settings");
    }
    delay(333);
}

void processdata() {
    for (byte i=0; i<rLength; i++) {
        if (serialIncoming[i] == rEOM) {
            newData = false;
            return;
        } else if (serialIncoming[i] == rQuery) {
            findsensor(maxPulse);
            //Serial.write(sReady);
            Serial.println("arduino ready");
            flashled();
        } else if (serialIncoming[i] == rTurn) {
            turn(halfTurns, startPitch, dir);
        } else if (serialIncoming[i] == rGliss) {
            gliss(halfTurns, startPitch, endPitch, dir);
        } else if (serialIncoming[i] == rTimedTurn) {
            timedturn(halfTurns, duration, dir);
        } else if (serialIncoming[i] == rDurationTurn) {
            durationturn(duration, startPitch, dir, recentre);
        } else if (serialIncoming[i] == rDurationGliss) {
            durationgliss(duration, startPitch, endPitch, dir, recentre);
        } else if (serialIncoming[i] == rForwards) {
            dir = true;
        } else if (serialIncoming[i] == rBackwards) {
            dir = false;
        } else if (serialIncoming[i] == rRecentre) {
            recentre = true;
        } else if (serialIncoming[i] == rNoRecentre) {
            recentre = false;
        } else if (serialIncoming[i] == rSetDuration) {
            for (byte d=0; d<rDurlength; d++) {
                newDuration[d] = serialIncoming[++i];
            }
            duration = atoi(newDuration);
        } else if (serialIncoming[i] < 21) {
            test(serialIncoming[i] - 13);
        } else if (serialIncoming[i] < 40) {
            halfTurns = serialIncoming[i] - 20;
        } else if (serialIncoming[i] < 112) {
            startPitch = pitchbyteToMidi(serialIncoming[i] - 40);
        } else if (serialIncoming[i] < 120) {
            true;
        } else if (serialIncoming[i] < 192) {
            endPitch = pitchbyteToMidi(serialIncoming[i] - 120);
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
