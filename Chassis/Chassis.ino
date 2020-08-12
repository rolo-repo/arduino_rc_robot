#define ENABLE_LOGGER
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "RFcom.h"

//#include <EEPROM.h>

#include <Servo.h>

#include "BTS7960.h"
#include "L298N.h"
#include "Led.h"


#include "SerialOutput.h"

#define PIN unsigned int

unsigned char address[][6] = { "1Node" }; // pipe address
//char recieved_data[4];

// long is 4 bytes
// int = short is 2 bytes

// On boards other than the Mega, use of the library disables analogWrite() 
// (PWM) functionality on pins 9 and 10, whether or not there is a Servo on those pins.
//  On the Mega, up to 12 servos can be used without interfering with PWM functionality; 
//  use of 12 to 23 motors will disable PWM on pins 11 and 12

//AF_DCMotor motor(3);


#define OD_MIN_DISTANCE_CM 5

/*
L298N shield
Function	    Ch. A  |  Ch. B
Direction	    D12	   |   D13
PWM	            D3	   |   D11
Brake	        D9	   |   D8
Current Sensing	A0	   |   A1
*/
/*
	L298N connection
	ENA	Pin 11	--- not required
	IN1	Pin 9
	IN2	Pin 8
	IN3	Pin 7
	IN4	Pin 6
	ENB	Pin 10
*/

/*
Radio
	CE   D9    SCN  D10
	SCK  D13
	MOSI D11
	MISO D12
*/

#if defined ARDUINO_AVR_UNO || defined ARDUINO_AVR_NANO
constexpr PIN motorFrdPin = 2; //INT0
constexpr PIN motorSpdPin = 3; //PWM //INT1
constexpr PIN motorBwdPin = 4;

constexpr PIN turretSpdPin  = 5; //PWM
constexpr PIN headLightPin	= 6; //PWM

constexpr PIN radioCE_Pin		= 7;
constexpr PIN radioSCN_Pin		= 8;

constexpr PIN steeringSrvPin	= 9; //no PWM due to servo
//10;//no PWM due to servo

//MOSI 11 used for radio
//MISO 12 used for radio
//SCK  13 used for radio

constexpr PIN trigPin		 = A0;
constexpr PIN turretLeftPin	 = A1;
constexpr PIN turretRightPin = A2;
constexpr PIN backLightPin   = A3;
//A4 SDA
//A5 SCL

#ifdef ARDUINO_AVR_NANO
constexpr PIN  lightSensor = A7; //Analog input only
constexpr PIN  echoPin  = A6; //Analog input only

#endif // ARDUINO_AVR_NANO

#endif ARDUINO_AVR_UNO || defined ARDUINO_AVR_NANO

int SERVO_ZERO = 90;
#define SERVO_MAX_LEFT SERVO_ZERO - 22 //22 is optimal 
#define SERVO_MAX_RIGHT SERVO_ZERO + 22

Led headLight(headLightPin);
Led backLight(backLightPin);

L298N_1PWM motor( motorFrdPin, motorBwdPin, motorSpdPin, []() { backLight.turn_on(Led::Brightness::_100); } );
L298N_1PWM turret( turretLeftPin, turretRightPin, turretSpdPin );
//BTS7960_1PWM  motor(motorFrdPin, motorBwdPin, motorSpdPin, []() { backLight.turn_on(Led::Brightness::_100); });

class ServoSmooth : public Servo
{
public :
	void write( int i_value ) { m_targetAngel = constrain( i_value , SERVO_MAX_LEFT , SERVO_MAX_RIGHT ); }

	void run()
	{
		static long tm = millis();

		if ( abs( read() - m_targetAngel ) < 2 )
			return;

		if ( tm + 15 < millis() )
		{
			 ( read() > m_targetAngel) ?
				Servo::write( read() - 1 ):
				Servo::write( read() + 1 );
			tm = millis();
		}
	}

private:
	short m_targetAngel;
};

ServoSmooth servo;

RF24 radio( radioCE_Pin, radioSCN_Pin );

//return distance in cm
unsigned long getDistance()
{
	digitalWrite(trigPin, LOW);
	delayMicroseconds(2);
	// Sets the trigPin on HIGH state for 10 micro seconds
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigPin, LOW);
	unsigned long duration = pulseIn(echoPin, HIGH); // Reads the echoPin, returns the sound wave travel time in microseconds
	// 340 m/s ->  340  * 100 * 10  / 1000 000  -> 340 / 1000 ->  0.34 mm / msec  -> 0.034 cm/msec
	unsigned long distance = duration * 0.034 / 2;
	return distance;
}

bool isDark ()
{
	int analogValue = analogRead(lightSensor);

	LOG_MSG("Analog reading = " << analogValue);

	if ( analogValue > 900) {
		LOG_MSG("Dark");
		return true;
	}

	return false;
}

void setup() 
{
	using namespace arduino::utils;
	LOG_MSG_BEGIN(115200);


	pinMode( trigPin , OUTPUT );
	pinMode( echoPin , INPUT );
	
	pinMode( headLightPin, OUTPUT );
	pinMode( backLightPin, OUTPUT );

	pinMode( lightSensor, INPUT );

	motor.begin();	
	//LOG_MSG("Servo : " << servo.read());

	servo.attach(steeringSrvPin);
	servo.write( SERVO_ZERO );
	turret.begin();

	headLight.turn_off();
	backLight.turn_off();

	radio.begin(); //активировать модуль
	radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
	radio.setRetries(0, 0);     //(время между попыткой достучаться, число попыток)
	radio.enableAckPayload();   
	radio.setPayloadSize(sizeof(Payload)/*32*/);   
	radio.enableDynamicPayloads();
	radio.openReadingPipe(1, (const uint8_t *)address[0]);    
	radio.setChannel(0x64);

	radio.setPALevel(RF24_PA_MIN); //RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
	radio.setDataRate(RF24_250KBPS); //RF24_2MBPS, RF24_1MBPS, RF24_250KBPS

	radio.powerUp(); //начать работу

	radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
	
	backLight.fade(1000);

   // SERVO_ZERO = EEPROM.read(0);
	LOG_MSG("Init success");
}

long map(const long x, const long in_min, const long in_max, const long out_min, const long out_max)
{
	// if input is smaller/bigger than expected return the min/max out ranges value
	if (x < in_min)
		return out_min;
	else if (x > in_max)
		return out_max;

	else if (in_max == in_min)
		return min(abs(in_max), abs(in_min));

	// map the input to the output range.
	// round up if mapping bigger ranges to smaller ranges
	else  if ((in_max - in_min) > (out_max - out_min))
		return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
	// round down if mapping smaller ranges to bigger ranges
	else
		return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


//1 gate - > 1K put devider 1 to 10K to ground
//2 drain  +
//3 source -
//  d -> s


//B 1K
//C +
//E -
//C -> E



void loop() 
{
	
	using namespace arduino::utils;
	byte pipeNo;
	static short curSteering = 90;
	static short curSpeed = 0;
	static unsigned long lastRecievedTime = millis();
   
	static bool OD_ENABLED = !( 0 == getDistance() ); //If the sensor is connect the value will be different than 0
	static Payload recieved_data;
	short obstacleIteration = 0;

	while ( OD_ENABLED && getDistance() < OD_MIN_DISTANCE_CM )
	{
		LOG_MSG( "Obstacle detected " << getDistance() << " cm");

		if ( motor.getDirection() == Motor::Direction::FORWARD )
		{
			motor.backward( 100 );
			headLight.rapid_blynk(1000);
			if ( servo.read() > SERVO_ZERO )
			{
				servo.write( SERVO_MAX_LEFT );
			}
			else
			{
				servo.write( SERVO_MAX_RIGHT );
			}
			motor.forward( 100 );
			headLight.rapid_blynk(500);
		}
		else
		{
			// do nothing the sensor is head looking
		}
		if ( ++obstacleIteration > 10 )
		{
			LOG_MSG("Not able to avoid obstacle");
			servo.write(SERVO_ZERO);
			headLight.rapid_blynk( 100 );
			motor.backward( 100 );
			headLight.fade(2000);
			return;
		}
	}

	while ( radio.available(&pipeNo) ) 
	{   
		radio.read(&recieved_data, sizeof(recieved_data));

		if (!recieved_data.isValid())
		{
			LOG_MSG(F("Garbage on RF"));
			radio.reUseTX();	//triggers resend of data
			continue;
		}

		lastRecievedTime = millis();

		if (!servo.attached())
			servo.attach(steeringSrvPin);

		LOG_MSG(F("RCV  Speed:") << recieved_data.m_speed 
			 << F(" Direction: ") << ((recieved_data.m_speed > 0) ? F("BACKWARD") : F("FORWARD"))
			 << F(" Steering: ")  << recieved_data.m_steering 
			 << ((recieved_data.m_steering > 0) ? F(" LEFT") : F(" RIGHT"))
			 << F(" Bits ")	<< recieved_data.m_b1 
			 << F(" ")		<< recieved_data.m_b2 
			 << F(" ")		<< recieved_data.m_b3 
			 << F(" ")		<< recieved_data.m_b4 ) ;

		if (recieved_data.m_b3 || isDark() )
		{
			headLight.turn_on(Led::Brightness::_100);
			backLight.turn_on(Led::Brightness::_50);
		}
		else
		{
			headLight.turn_off();
			backLight.turn_off();
		}

		if ( recieved_data.m_speed > 0 )
		{
			motor.backward( curSpeed = map( recieved_data.m_speed, 0, 127, 0, 255 ) );
		}
		else
		{
			motor.forward( curSpeed = map( recieved_data.m_speed, -127 , 0 , 255 , 0 ) );
		}
		
		if ( recieved_data.m_steering > 0 )
		{
			servo.write(curSteering = map( recieved_data.m_steering, 0, 127, SERVO_ZERO, SERVO_MAX_LEFT ));
		}
		else
		{
			servo.write(curSteering = map( recieved_data.m_steering, -127, 0, SERVO_MAX_RIGHT, SERVO_ZERO ));
		}

		if ( recieved_data.m_j4 > 0 )
		{
			turret.right( map ( recieved_data.m_j4, 0, 127, 0, 255 ) );                   
		}
		else
		{
			turret.left(  map( recieved_data.m_j4 , -127, 0, 255, 0 ) );
		}
		
		// Send ack
		payLoadAck.speed = curSpeed;
		payLoadAck.batteryLevel = servo.read();
		radio.writeAckPayload(pipeNo, &payLoadAck, sizeof(payLoadAck));
	}

	if ( lastRecievedTime < millis() - 3 * arduino::utils::RF_TIMEOUT_MS )
	{
		
		LOG_MSG("Lost connection");
	  
		motor.stop();
		turret.stop();
		servo.write(SERVO_ZERO);
		curSteering = servo.read();
		curSpeed = 0;

		if (curSteering == SERVO_ZERO)
			servo.detach();

		backLight.fade(500);
		headLight.fade(500);

		lastRecievedTime = millis();
	}

	
	turret.run();
	motor.run();
	servo.run();
}
