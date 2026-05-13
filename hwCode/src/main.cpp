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

/* Line Following Car Kit
 * Follows a tape line
 * Docs + links:
 * http://handsontec.com/dataspecs/L298N%20Motor%20Driver.pdf
 * https://www.st.com/resource/en/datasheet/l298.pdf
 *
 * The program needs a bit of tuning, the car overshoots in speed but going any lower will cause the car to not move
 * the sensors think the floor is tape, and masking tape and paper look exactly the same in IR resulting in surfaces it runs on to matter a lot
 * due to the positioning of the sensors, it would be significantly easier to have the car stay in between 2 lines (flip all the turnright() and turnleft())
 *
 * MAZE SOLN would look similar, follow the left edge/tape barrier
 * build and populate a map datastructure and run through the whole thing, prebuild a map to run through
 * we could explore Dijkstra, A*, random Mouse, left/right hand rules, deadend filling, negative potential field, Machine learning algorithms
 * and have the kids decide and build their own algorithms with it
 *
 * adding encoders/an accelerometer would let me add PID control and odometry, which would make real maze solutions actually possible
*/


// PROGRAM CTRL SHI
#define TAPE_FLOOR_THRESH 150 // value for when an analog signal is considered tape
#define SPD 100 // PWM 0 - 255 # for how fast to go on straights
#define SPD_TURN 100 // "" for how fast to go on turns
#define INTERVAL_OF_FUCKED 1000 // amount of time to be not seeing tape before robot decides to stop

void setup() {
	Serial.begin(9600);

	pinMode(LFL_DIG_PIN, INPUT);
	pinMode(LFR_DIG_PIN, INPUT);

	pinMode(IN1, OUTPUT);
	pinMode(IN2, OUTPUT);
	pinMode(IN3, OUTPUT);
	pinMode(IN4, OUTPUT);

	while (!Serial);

	Serial.println("Ready");

	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

}

// day 1 is also figuring out forward, turning and stop functions
void forward() {
	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, LOW);
}

void turnRight() {

	analogWrite(EN1_PIN, SPD_TURN);
	analogWrite(EN2_PIN, SPD_TURN);

	digitalWrite(IN1, HIGH);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, LOW);
}

void turnLeft() {

	analogWrite(EN1_PIN, SPD_TURN);
	analogWrite(EN2_PIN, SPD_TURN);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, HIGH);

}

void stop() {

	digitalWrite(IN1, HIGH);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, HIGH);

}


void loop() {

	// day 1: understand the line following sensor
	int left = analogRead(LFL_NLG_PIN);
	int right = analogRead(LFR_NLG_PIN);

	Serial.print("left: ");
	Serial.print(digitalRead(LFL_DIG_PIN));
	Serial.print("   ");
	Serial.print(left);

	Serial.print("right: ");
	Serial.print(digitalRead(LFR_DIG_PIN));
	Serial.print("    ");
	Serial.print(right);

	/** Day 1:
	 * left/right is 1 when over tape 0 when not
	 * TAPE_FLOOR_THRESH is something that has to be tuned
	 * the > or < sign also changes depending on if the floor is darker or lighter IR than the tape in class
	 */
	left = left > TAPE_FLOOR_THRESH;
	right = right > TAPE_FLOOR_THRESH;

	Serial.print(" //right: ");
	Serial.print(right);
	Serial.print("left: ");
	Serial.println(left);

	/** Day 2:
	 * basic line following program
	 * this is what almost everyone else uses
	 * also setting up and building a maze will take up some amount of time
	 */
	/*
	if (left && right) {
		forward();

	} else if (left && !right) {
		turnRight();

	} else if (!left && right) {
		turnLeft();

	} else if (!left && !right) {
		stop();
	}
	*/

	/** Day 3:
	 * Basically allows the last command to persist for some # of seconds
	 * The robot has some static friction that stops us from moving slow
	 * When we move to fast inertia generally carries us over the line
	 * this is a hardware limitation we cannot currently fix
	 * we compensate for this by persisting the last instruction for some amount of time
	 * this way even when we don't see the line due to overshoot we can guess where it is
	 * this is not perfect but fuck it we ball
	 */

	static long lastRight = millis();
	static long lastLeft = millis();

	if (!left && !right) {

		// if the last instruction was to turn left , let it persist for INTERVAL_OF_FUCKED ms
		if (lastRight > lastLeft && millis() - lastRight < INTERVAL_OF_FUCKED) {
			turnRight();

		} else if (lastLeft > lastRight && millis() - lastLeft < INTERVAL_OF_FUCKED) {
			turnLeft();

		} else {
			stop();
		}

	} else if (left && right) {
		forward();

	} else if (left && !right) {
		turnRight();
		lastRight = millis();

	} else if (!left && right) {
		turnLeft();
		lastLeft = millis();

	}
}
