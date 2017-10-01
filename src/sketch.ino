/*
 * Motor control for stepper half turns
 */
#define debug           0
// motor pins
#define stepPin         6
#define dirPin          7
#define enablePin       8
#define sensorPin       9

// speeds
#define pulseWidth      5
#define halfTurn      200 // steps for a half turn
#define accelSteps     70 // accelerating steps: must be <= halfTurn/2
#define minPulse      500 // fastest step min 500
#define maxPulse     2500 // slowest step

/*************************
 * communication commands
 *************************
 */
#define rQuery          2
#define rForward        3
#define rBackward       4
#define sReady          6
#define sSwitch        10
/*
 * 11-20 half turns (-10)
 * 21-40 speed?
 */

// serial input
byte serialIncoming     = 0;
byte switchState        = HIGH;
byte switchLast         = HIGH;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    //pinMode(enablePin, OUTPUT);
    //digitalWrite(enablePin, HIGH);
    pinMode(sensorPin, INPUT_PULLUP);
    digitalWrite(dirPin, HIGH);
    Serial.begin(9600);
    findsensor();
    Serial.write(sReady);
}

void findsensor() {
    while(digitalRead(sensorPin) == LOW) {
        onestep(maxPulse);
    }
}

void onestep(int pulseTime) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(pulseTime - pulseWidth);
}

void flashled() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
}

void accelturn(int halfturns) {
    int fullSpeedSteps = (halfTurn * halfturns) - (accelSteps * 2);
    digitalWrite(LED_BUILTIN, HIGH);
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, maxPulse, minPulse));
    }
    for (int i=0; i<fullSpeedSteps; i++) {
        onestep(minPulse);
    }
    for (int i=0; i<accelSteps; i++) {
        onestep(map(i, 0, accelSteps, minPulse, maxPulse));
        /*
        if (digitalRead(sensorPin) == HIGH) {
            digitalWrite(LED_BUILTIN, LOW);
            return;
        }
        */
    }
    //findsensor();
    digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
    if (Serial.available()) {
        serialIncoming = Serial.read();

        if (serialIncoming == rQuery) {
            findsensor();
            Serial.write(sReady);
            flashled();
        } else if (10 < serialIncoming < 21) {
            accelturn(serialIncoming - 10);
        }
    }
}
