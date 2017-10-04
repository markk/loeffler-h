/*
 * Motor control for stepper half turns
 */

// motor pins
#define stepPin         6
#define dirPin          7
#define sensorPin       9

// speeds
#define pulseWidth       5
#define halfTurnSteps 1600 // steps for a half turn
#define fastestPulse   450
#define fastestStart  1800
#define decellEnd     2500
#define slowestPulse  5000

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
int accelSteps        = 70;
int minPulse          = 500;
int maxPulse          = 2500;
int startPulse        = 800;
int endPulse          = 1000;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(sensorPin, INPUT_PULLUP);
    digitalWrite(dirPin, HIGH);
    Serial.begin(9600);
    findsensor(slowestPulse);
    Serial.write(sReady);
}

void findsensor(int pulse) {
    while(digitalRead(sensorPin) == LOW) {
        onestep(pulse);
    }
}

void onestep(int pulseTime) {
    int scaledPulse = (pulseTime * 200) / halfTurnSteps;
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(scaledPulse - pulseWidth);
}

void flashled() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
}

void glissturn(int halfturns, int startpulse, int endpulse, int maxpulse, int accelsteps) {
    int glissSteps = (halfTurnSteps * halfturns) - (accelsteps * 2);
    digitalWrite(LED_BUILTIN, HIGH);
    for (int i=0; i<accelsteps; i++) {
        onestep(map(i, 0, accelsteps, maxpulse, startpulse));
    }
    for (int i=0; i<glissSteps; i++) {
        onestep(map(i, 0, glissSteps, startpulse, endpulse));
    }
    for (int i=0; i<accelsteps; i++) {
        onestep(map(i, 0, accelsteps, endpulse, decellEnd));
        if (digitalRead(sensorPin) == HIGH) {
            digitalWrite(LED_BUILTIN, LOW);
            return;
        }
    }
    findsensor(decellEnd);
    digitalWrite(LED_BUILTIN, LOW);
}

void accelturn(int halfturns, int minpulse, int maxpulse, int accelsteps) {
    int fullspeedsteps = (halfTurnSteps * halfturns) - (accelsteps * 2);
    digitalWrite(LED_BUILTIN, HIGH);
    for (int i=0; i<accelsteps; i++) {
        onestep(map(i, 0, accelsteps, maxpulse, minpulse));
    }
    for (int i=0; i<fullspeedsteps; i++) {
        onestep(minpulse);
    }
    for (int i=0; i<accelsteps; i++) {
        onestep(map(i, 0, accelsteps, minpulse, decellEnd));
        if (digitalRead(sensorPin) == HIGH) {
            digitalWrite(LED_BUILTIN, LOW);
            return;
        }
    }
    findsensor(decellEnd);
    digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
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
            accelSteps = map(serialIncoming, 101, 120, 50, 100);
        }
    }
}
