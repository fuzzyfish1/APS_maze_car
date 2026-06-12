#include <Arduino.h>

// Line following sensor GND -> Arduino GND
// Line Following Sensor Vin -> Arduino 5V

// we use analog rather than the digital tuning with trimpots, it is easier for us but it can be done that way
// line follow left analog pin
#define LFL_NLG_PIN A6
#define LFL_DIG_PIN 3

#define LFR_NLG_PIN A7
#define LFR_DIG_PIN 2

// direction control pins
#define IN1 4
#define IN2 5
#define IN3 6
#define IN4 7

// needs PWM pins
#define EN1_PIN 9
#define EN2_PIN 10

/**
 * System -
 *   OS:  [Linux Mint 22.1 x86 Cinnamon]
 *   IDE: [CLion + PlatformIO]
 * Author: Zain Ali
 *
 * APS ReadySetStem LineFollowing Car Examples
 * Line Following Car Kit
 * Follows a tape line
 * Docs + links:
 * http://handsontec.com/dataspecs/L298N%20Motor%20Driver.pdf
 * https://www.st.com/resource/en/datasheet/l298.pdf
 *
*/

#define TAPE_FLOOR_THRESH 0 // fill in this number
#define SPD 100 // PWM 0 - 255
#define INTERVAL_OF_STOP 2000 // amount of time to be not seeing tape before robot decides to stop
#define INTERVAL_OF_STOP_ABRUPT 5000

// an example, do this for turnRight, and turnLeft
void forward() {
	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, LOW);
	// Serial.println("forward");
}

void turnRight() {
	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	// build truth table and fill in the digitalWrite

	// Serial.println("turnRight");
}

void turnLeft() {
	// what analogWrite could go here

	// build table and fill this in as well
	// Serial.println("turnLeft");
}

// given because we want coasting
void stop() {
	analogWrite(EN1_PIN, 0);
	analogWrite(EN2_PIN, 0);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, LOW);
	digitalWrite(IN3, LOW);
	digitalWrite(IN4, LOW);
	// Serial.println("stop");
}

void setup() {

	// add pinmode for the other line following sensor
	pinMode(LFL_DIG_PIN, INPUT);

	// repeat for IN2 - IN4
	pinMode(IN1, OUTPUT);

	Serial.begin(115200);
	while (!Serial);
	Serial.println("Ready");

}

void loop() {

	// fill in reading the pins here
	int leftAnalog = 2;
	int rightAnalog = 1;
	// watch these in the Serial Plotter

	// how do we know if we are on the tape/line
	bool leftSeeTape = true;
	bool rightSeeTape = true;

	static unsigned long lastPrint = 0;
	if (millis() - lastPrint > 100) {
		lastPrint = millis();
		// Serial.print("left: ");
		// Serial.print(digitalRead(LFL_DIG_PIN));
		Serial.print("LAnalog: ");
		Serial.print(leftAnalog);

		// Serial.print("right: ");
		// Serial.print(digitalRead(LFR_DIG_PIN));
		Serial.print(", RAnalog: ");
		Serial.print(rightAnalog);

		Serial.print(", seeRightTape: ");
		Serial.print(rightSeeTape);
		Serial.print(", seeLeftTape: ");
		Serial.println(leftSeeTape);
	}

	/** to test out the motors, uncomment one of these lines, then comment them back in the final code */

	// forward();
	// delay(1000);

	// turnRight();
	// delay(1000);

	// turnLeft();
	// delay(1000);

	static long lastRight = 0;
	static long lastLeft = 0;
	static long lastForward = 0;

	// think about all the combinations of leftSeeTape and rightSeeTape
	// as you decide what inputs lead to what outputs
	if (!leftSeeTape && !rightSeeTape) {

	} else if (leftSeeTape && rightSeeTape) {

	} else if (!leftSeeTape && rightSeeTape) {

	} else if (leftSeeTape && !rightSeeTape) {

	}
}