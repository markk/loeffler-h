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
#define rQuery          2
#define rAccelturn      3
#define rGlissturn      4
#define sReady          6

byte serialIncoming   = 0;
int halfTurns         = 1;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(sensorPin, INPUT_PULLUP);
    digitalWrite(dirPin, HIGH);
    Serial.begin(9600);
    findsensor(maxPulse);
    Serial.write(sReady);
}

int pitchToPulse(float pitch) {
    /* map MIDI pitch to motor pulse duration */
    float i = ((71 - constrain(pitch, 36, 72)) * 71) / 33;
    float p = 1 - log10(((72 - i) * 0.125) + 1);
    return 500 + (3500 * p);
}

float pitchbyteToMidi(byte pitch) {
    return (pitch / 2) + 36;
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

int timedturn(int duration, int halfturns, bool dir) {
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
        int halfturns = random(1, 10);
        int duration = random(300, 5000);
        Serial.print("halfturns ");
        Serial.print(halfturns, DEC);
        Serial.print(" duration ");
        Serial.print(duration, DEC);
        Serial.print(" time ");
        unsigned long starttime = millis();
        int pulse = timedturn(duration, halfturns, duration % 2);
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
    }
    delay(333);
}

void loop() {
    test(4);
    /*
    if (Serial.available()) {
        serialIncoming = Serial.read();

        if (serialIncoming == rQuery) {
            findsensor(slowestPulse);
            Serial.write(sReady);
            flashled();
        } else if (serialIncoming == rAccelturn) {
            accelturn(halfTurns, minPulse, maxPulse, accelSteps);
        } else if (serialIncoming == rGlissturn) {
            glissturn(halfTurns, startPulse, endPulse, maxPulse, accelSteps);
        } else if (serialIncoming < 11) {
            flashled();
        } else if (serialIncoming < 21) {
            halfTurns = serialIncoming - 10;
        } else if (serialIncoming < 41) {
            minPulse = map(serialIncoming, 21, 40, fastestPulse, fastestStart);
        } else if (serialIncoming < 61) {
            maxPulse = map(serialIncoming, 41, 60, fastestStart, slowestPulse);
        } else if (serialIncoming < 81) {
            startPulse = map(serialIncoming, 61, 80, fastestPulse, slowestPulse);
        } else if (serialIncoming < 101) {
            endPulse = map(serialIncoming, 81, 100, fastestPulse, slowestPulse);
        } else if (serialIncoming < 121) {
            accelSteps = map(serialIncoming, 101, 120, halfTurnSteps/4, halfTurnSteps/2);
        }
    }
    */
}
