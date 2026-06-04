#include <Arduino.h>
#include <Adafruit_MPU6050.h>

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

// MPU 6050 sensor wiring
#define ACL_SDA_PIN A5
#define ACL_SCL_PIN A4

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
 * https://www.invensense.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 * https://lastminuteengineers.com/mpu6050-accel-gyro-arduino-tutorial/
 * ^^ grab MPU6050 images from ^^, it explains MEMS devices very well
 *
 * MAZE SOLN would look similar, follow the left edge/tape barrier
 * build and populate a map datastructure and run through the whole thing, prebuild a map to run through
 * we could explore Dijkstra, A*, random Mouse, left/right hand rules, deadend filling, negative potential field, Machine learning algorithms
 * and have the kids decide and build their own algorithms with it
 *
 * adding encoders/an accelerometer would let me add PID control and odometry, which would make real maze solutions actually possible
 *
 * fill up the class a bit with trying different things in the classroom to see what would make a good map
 *
*/

// PROGRAM CTRL SHI

#define TAPE_FLOOR_THRESH 200 // 150 for paper value for when an analog signal is considered tape
#define SPD 100 // PWM 0 - 255
#define INTERVAL_OF_STOP 2000 // amount of time to be not seeing tape before robot decides to stop
#define INTERVAL_OF_STOP_ABRUPT 5000
// day 1 + day 2 code

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

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, LOW);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, LOW);
	// Serial.println("turnRight");
}

void turnLeft() {
	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, LOW);
	digitalWrite(IN4, LOW);

	// Serial.println("turnLeft");
}

void stop() {
	analogWrite(EN1_PIN, 0);
	analogWrite(EN2_PIN, 0);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, LOW);
	digitalWrite(IN3, LOW);
	digitalWrite(IN4, LOW);
	// Serial.println("stop");
}

void stopMotors() {

}

void setup() {

	pinMode(LFL_DIG_PIN, INPUT);
	pinMode(LFR_DIG_PIN, INPUT);

	pinMode(IN1, OUTPUT);
	pinMode(IN2, OUTPUT);
	pinMode(IN3, OUTPUT);
	pinMode(IN4, OUTPUT);

	Serial.begin(115200);

	while (!Serial);

	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	Serial.println("Ready");

}

void loop() {
	/** Day 1:
	 * connecting all the wires (not the MPU 6050), reading all the sensors, spinning the motors once or twice, going forward/backwards
	 * left/right is 1 when over tape 0 when not
	 * TAPE_FLOOR_THRESH is something that has to be tuned by the kids every single class, things like heat, color and material effects the
	 * IR sensor in a light spectrum we cannot see, black tape and black table might look very different to IR sensor
	 * but can then look the same when heated with your hand

	 * the > or < sign also changes depending on if the floor is darker or lighter IR than the tape in class
	 * > for black ink path on white printer paper
	 */

	int leftAnalog = analogRead(LFL_NLG_PIN);
	int rightAnalog = analogRead(LFR_NLG_PIN);

	bool leftSeeTape = leftAnalog > TAPE_FLOOR_THRESH;
	bool rightSeeTape = rightAnalog > TAPE_FLOOR_THRESH;

	static unsigned long lastPrint = 0;

	// print once a second, serial takes forever otherwise
	// DEBUG STATEMENTS IF NEEDED

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

	/** Day 2: Bang Bang Line following logic
	 * a very basic program, to bounce on the inside/outside of tape (depending on how you setup the >< signs earlier)
	 *
	 * The first solution to inertia they will solve is to
	 * let the last command to persist for some # of time
	 * The robot has some static friction that stops us from moving slow
	 * When we move to fast inertia generally carries us over the line
	 * this is a hardware limitation we cannot currently fix
	 * we compensate for this by persisting the last instruction for some amount of time
	 * this way even when we don't see the line due to overshoot we can guess where it is
	 */

	static long lastRight = 0;
	static long lastLeft = 0;
	static long lastForward = 0;

	if (!leftSeeTape && !rightSeeTape) {

		forward();
		/**UNIT3 : Persistance with a reset*/

		// if the last instruction was to turn right (it passed over the left edge but overshot), let it persist for INTERVAL_OF_STOPED ms
		if ( lastRight > lastForward && lastRight > lastLeft && millis() - lastRight < INTERVAL_OF_STOP) {
			turnRight();

		} else if (lastLeft > lastForward && lastLeft > lastRight && millis() - lastLeft < INTERVAL_OF_STOP) {
			turnLeft();

		// if the line just ends abruptly ... turn right to hopefully find the line again, also give it more time to figure this out
		// go straight if the tape is smaller than
		} else if (millis() - lastForward < INTERVAL_OF_STOP_ABRUPT) {
			forward();

		// we stuck and can't see anything for a while just give up, ... loop will restart it after 2s in tape/flipped over
		} else {
			stop();
			// the delay allows some random amt of time before it goes off into the world
			delay(2000);
		}

		/** UNIT 3: ^^*/

	} else if (leftSeeTape && rightSeeTape) {
		forward();
		lastForward = millis();

	} else if (!leftSeeTape && rightSeeTape) {
		turnRight();
		lastRight = millis();

	} else if (leftSeeTape && !rightSeeTape) {
		turnLeft();
		lastLeft = millis();
	}
}
