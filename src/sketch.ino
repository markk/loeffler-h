/*
 * Motor control for stepper half turns
 */

// motor pins
#define stepPin         6
#define dirPin          7
#define enablePin       8

// speeds
#define halfTurnSteps  2000
#define stepPulse      38

/*************************
 * communication commands
 *************************
 */
#define rQuery          2
#define sReady          6
#define rForward        7
#define rBackward       8
#define sSwitch        10
#define sDone          11
/*
 * 20-255 speed - maps inversely from pulseSlow to pulseFast
 */

// serial input
byte serialIncoming     = 0;
byte switchState        = HIGH;
byte switchLast         = HIGH;

void setup() {
    pinMode(stepPin, OUTPUT);
    //pinMode(dirPin, OUTPUT);
    //pinMode(enablePin, OUTPUT);
    //digitalWrite(enablePin, HIGH);
    Serial.begin(9600);
    Serial.write(sReady);
}

void halfturn() {
    for (int i=0; i<halfTurnSteps; i++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(stepPulse);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(stepPulse);
    }
}

void loop() {

    /*
    switchState = digitalRead(switchPin);
    if (switchState != switchLast) {
        switchLast = switchState;
        Serial.write(sSwitch + switchState);
        delay(200); // debounce switch
    }

    if (Serial.available()) {
        serialIncoming = Serial.read();

        if (serialIncoming > 19) {
            steppulse = map(serialIncoming, 20, 255, pulseSlow, pulseFast);
        } else if (serialIncoming == rQuery) {
            Serial.write(sReady);
        } else if (serialIncoming == rForward) {
            digitalWrite(dirPin, HIGH);
            halfturn(steppulse);
        } else if (serialIncoming == rBackward) {
            digitalWrite(dirPin, LOW);
            halfturn(steppulse);
        }
        Serial.write(sDone);
    }

    for (int i=0; i<500; i++) {
        float waitTime = 100;
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(int(waitTime));
        digitalWrite(stepPin, LOW);
        delayMicroseconds(int(waitTime));
        waitTime -= (100 - 10) / 500;
    }
    for (int i=0; i<1500; i++) {
        float waitTime = 10;
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(int(waitTime));
        digitalWrite(stepPin, LOW);
        delayMicroseconds(int(waitTime));
        waitTime += (100 - 10) / 1500;
    }
    */

    halfturn();
    delay(700);
    halfturn();
    delay(350);
    halfturn();
    delay(350);
    halfturn();
    delay(350);
    halfturn();
    delay(1150);
    halfturn();
    halfturn();
    halfturn();
    halfturn();
    halfturn();
    halfturn();
    delay(350);
    halfturn();
    delay(350);
    halfturn();
    delay(467);
    halfturn();
    delay(467);
    halfturn();
    delay(467);
    halfturn();
    halfturn();
    delay(1150);
    halfturn();
    delay(350);
    halfturn();
    delay(350);
    halfturn();
    delay(4600);

}
