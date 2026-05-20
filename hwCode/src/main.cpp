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

/* Line Following Car Kit
 * Follows a tape line
 * Docs + links:
 * http://handsontec.com/dataspecs/L298N%20Motor%20Driver.pdf
 * https://www.st.com/resource/en/datasheet/l298.pdf
 *
 * https://www.invensense.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 * https://lastminuteengineers.com/mpu6050-accel-gyro-arduino-tutorial/
 * ^^ grab MPU6050 images from ^^, it explains MEMS devices very well
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

// GYRO STUFF
#define GYRO_CAL_SAMPLES   500
#define GYRO_CAL_DELAY_MS  3        // 3ms per sample, 3ms *500 = 1.5s total calibration time
#define GYRO_CAL_VAR_LIMIT 0.0005f  // (rad/s)^2 — if motion detected during cal, warn user
float gyroZBias = 0.0f;            // gyro Z bias, set by calibrateGyro()
Adafruit_MPU6050 mpu;

// PROGRAM CTRL SHI
#define TAPE_FLOOR_THRESH 45
// 150 for paper value for when an analog signal is considered tape
#define SPD 110 // PWM 0 - 255
#define INTERVAL_OF_FUCKED 1200 // amount of time to be not seeing tape before robot decides to stop

// #define Kp 90.f
#define Kd 30.f // 10.f


// day 1 + day 2 code
void forward() {
	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, LOW);
}

void turnRight() {

	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, LOW);
	digitalWrite(IN3, HIGH);
	digitalWrite(IN4, LOW);
}

void turnLeft() {

	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, HIGH);
	digitalWrite(IN3, LOW);
	digitalWrite(IN4, LOW);

}

void stop() {

	digitalWrite(IN1, LOW);
	digitalWrite(IN2, LOW);
	digitalWrite(IN3, LOW);
	digitalWrite(IN4, LOW);

}

/** calibrate the Gyro*/
void calibrateGyro() {
	Serial.println("Calibrating gyro — DO NOT MOVE the car...");
	delay(500);  // let the user let go of the car if they were holding it

	float sumZ   = 0.0f;
	float sumZSq = 0.0f;
	sensors_event_t a, g, temp;

	for (int i = 0; i < GYRO_CAL_SAMPLES; i++) {
		mpu.getEvent(&a, &g, &temp);
		sumZ   += g.gyro.z;
		sumZSq += g.gyro.z * g.gyro.z;
		delay(GYRO_CAL_DELAY_MS);
	}

	float meanZ = sumZ / GYRO_CAL_SAMPLES;
	float varZ  = (sumZSq / GYRO_CAL_SAMPLES) - (meanZ * meanZ);

	gyroZBias = meanZ;

	Serial.print("Gyro Z bias: ");
	Serial.print(gyroZBias, 5);
	Serial.print(" rad/s   variance: ");
	Serial.println(varZ, 6);

	if (varZ > GYRO_CAL_VAR_LIMIT) {
		Serial.println("WARNING: Car was likely moving during calibration!");
		Serial.println("         Heading estimate during search will drift.");
		Serial.println("         Reset the board and try again with the car still.");
	} else {
		Serial.println("Gyro calibrated OK.");
	}
}

// day 3 motor control
void setRightMotor(int speed) {
	if (speed >= 0) {
		digitalWrite(IN1, LOW);
		digitalWrite(IN2, HIGH);
	} else {
		digitalWrite(IN1, HIGH);
		digitalWrite(IN2, LOW);
		speed = -speed;
	}
	analogWrite(EN1_PIN, constrain(speed, 0, 255));
}

void setLeftMotor(int speed) {
	if (speed >= 0) {
		digitalWrite(IN3, HIGH);
		digitalWrite(IN4, LOW);
	} else {
		digitalWrite(IN3, LOW);
		digitalWrite(IN4, HIGH);
		speed = -speed;
	}
	analogWrite(EN2_PIN, constrain(speed, 0, 255));
}

// Differential drive. control > 0 -> turn right (right wheel slower, left wheel faster).
void drive(const int& baseSpeed, const float& control) {
	int leftSpeed  = baseSpeed + (int)control;
	int rightSpeed = baseSpeed - (int)control;
	setLeftMotor(leftSpeed);
	setRightMotor(rightSpeed);
}

void stopMotors() {
	analogWrite(EN1_PIN, 0);
	analogWrite(EN2_PIN, 0);
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

	if (!mpu.begin()) {
		while (1) {
			Serial.println("Failed to find MPU6050 chip");
			delay(1000);
		}
	}

	Serial.println("Ready");

	analogWrite(EN1_PIN, SPD);
	analogWrite(EN2_PIN, SPD);

	mpu.setGyroRange(MPU6050_RANGE_500_DEG);
	mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
	mpu.setAccelerometerRange(MPU6050_RANGE_2_G);

	// calibrateGyro();

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

	sensors_event_t a, g, temp;
	mpu.getEvent(&a, &g, &temp);

	float gyroZ = g.gyro.z - gyroZBias;

	bool leftSeeTape = leftAnalog > TAPE_FLOOR_THRESH;
	bool rightSeeTape = rightAnalog > TAPE_FLOOR_THRESH;

	static unsigned long lastPrint = 0;

	static int32_t currTime = g.timestamp;
	static int32_t lastTime = currTime;
	static float searchHeading = 0.0f;

	searchHeading += gyroZ * (currTime - lastTime);

	// print once a second, serial takes forever
	// DEBUG STATEMENTS IF NEEDED
	if (millis() - lastPrint > 1000) {
		lastPrint = millis();
		Serial.print("left: ");
		Serial.print(digitalRead(LFL_DIG_PIN));
		Serial.print("   ");
		Serial.print(leftAnalog);

		Serial.print("right: ");
		Serial.print(digitalRead(LFR_DIG_PIN));
		Serial.print("    ");
		Serial.print(rightAnalog);

		Serial.print("Acceleration X: ");
		Serial.print(a.acceleration.x);
		Serial.print(", Y: ");
		Serial.print(a.acceleration.y);
		Serial.print(", Z: ");
		Serial.print(a.acceleration.z);
		Serial.print(" m/s^2 ");

		Serial.print(" Rotation X: ");
		Serial.print(g.gyro.x);
		Serial.print(", Y: ");
		Serial.print(g.gyro.y);
		Serial.print(", Z: ");
		Serial.print(g.gyro.z - gyroZBias);
		Serial.print("  rad/s  ");

		// Serial.print("currentError: ");
		// Serial.print(currentError);

		Serial.print("  Temperature: ");
		Serial.print(temp.temperature);
		Serial.print(" degC  ");

		Serial.print(" //right: ");
		Serial.print(rightSeeTape);
		Serial.print("left: ");
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

		// if the last instruction was to turn right (it passed over the left edge but overshot), let it persist for INTERVAL_OF_FUCKED ms
		if ( lastRight > lastForward && lastRight > lastLeft && millis() - lastRight < INTERVAL_OF_FUCKED) {
			turnRight();

		} else if (lastLeft > lastForward && lastLeft > lastRight && millis() - lastLeft < INTERVAL_OF_FUCKED) {
			turnLeft();

		// if the line just ends abruptly ... turn right to hopefully find the line again, also give it more time to figure this out
		} else if (millis() - lastForward < 2 * INTERVAL_OF_FUCKED) {
			turnRight();

		// we fucked just give up
		} else {
			stop();
			// the delay allows some random amt of time before it goes off into the world
			delay(2000);
		}

	} else if (leftSeeTape && rightSeeTape) {
		forward();
		lastForward = millis();

	} else if (leftSeeTape && !rightSeeTape) {
		turnRight();
		lastRight = millis();

	} else if (!leftSeeTape && rightSeeTape) {
		turnLeft();
		lastLeft = millis();
	}

	/**
	if (!leftSeeTape && !rightSeeTape) {

		// if the last instruction was to turn right (it passed over the left edge but overshot), let it persist for INTERVAL_OF_FUCKED ms
		if ( lastRight > lastForward && lastRight > lastLeft && millis() - lastRight < INTERVAL_OF_FUCKED) {
			drive(105, 20 - Kd*gyroZ);

		} else if (lastLeft > lastForward && lastLeft > lastRight && millis() - lastLeft < INTERVAL_OF_FUCKED) {
			drive(105, - 20 - Kd*gyroZ);

			// if the line just ends abruptly ... turn right to hopefully find the line again, also give it more time to figure this out
		} else if (millis() - lastForward < 2 * INTERVAL_OF_FUCKED) {
			drive(105, 20 - Kd*gyroZ);

			// we fucked just give up
		} else {
			drive(0, 20 - Kd*gyroZ);
			// the delay allows some random amt of time before it goes off into the world
			delay(2000);
		}

	} else if (leftSeeTape && rightSeeTape) {
		drive(105, -2 * Kd * gyroZ);
		lastForward = millis();

	} else if (leftSeeTape && !rightSeeTape) {
		drive(105, 20- Kd*gyroZ);
		lastRight = millis();

	} else if (!leftSeeTape && rightSeeTape) {
		drive(105, -25 - Kd*gyroZ);
		lastLeft = millis();
	}
	*/
}