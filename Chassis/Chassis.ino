/*
    Name:       RFCar.ino
    Created:	03/18/19 11:43:20
    Author:     NTNET\ROMANL
*/
#define ENABLE_LOGGER
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "SerialOutput.h"
#include "RFcom.h"

#include <EEPROM.h>

#include <Servo.h>

#include "Motor.h"
#include "BTS7960.h"
#include "Led.h"

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

#define MAX_LEFT 15
#define MAX_RIGHT 75
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

//5 //PWM
constexpr PIN backLightPin		= 6; //PWM

constexpr PIN radioCE_Pin		= 7;
constexpr PIN radioSCN_Pin		= 8;

constexpr PIN steeringSrvPin	= 9; //no PWM due to servo
constexpr PIN turretSrvPin		= 10;//no PWM due to servo

//MOSI 11 used for radio
//MISO 12 used for radio
//SCK  13 used for radio

constexpr PIN headLightPin  = A0;
constexpr PIN trigPin		= A1;
constexpr PIN echoPin		= A2;
constexpr PIN cannonSrvPin  = A3;
//A4 SDA
//A5 SCL

#ifdef ARDUINO_AVR_NANO
//A6
//A7
#endif // ARDUINO_AVR_NANO

#endif ARDUINO_AVR_UNO || defined ARDUINO_AVR_NANO

int SERVO_ZERO = 45;

BTS7960_1PWM motor(motorFrdPin, motorBwdPin, motorSpdPin);
Servo servo;

RF24 radio( radioCE_Pin, radioSCN_Pin );

Led headLight( headLightPin );
Led backLight( backLightPin );

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

void setup() 
{
    using namespace arduino::utils;
    LOG_MSG_BEGIN(115200);

    pinMode( trigPin , OUTPUT );
    pinMode( echoPin , INPUT );
	
	pinMode( headLightPin, OUTPUT );
	pinMode( backLightPin, OUTPUT );

	motor.begin();
    servo.attach( steeringSrvPin );
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

	static bool lightsOn = false;
    while ( OD_ENABLED && getDistance() < OD_MIN_DISTANCE_CM )
    {
        LOG_MSG( "Obstacle detected " << getDistance() << " cm");

        if ( motor.getDirection() == Motor::Direction::FORWARD )
        {
            motor.backward( 100 );
			headLight.rapid_blynk(1000);
            if ( servo.read() > SERVO_ZERO )
            {
                servo.write( MAX_LEFT );
            }
            else
            {
				servo.write( MAX_RIGHT );
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
		payLoadAck.speed = 0;

		payLoadAck.batteryLevel = servo.read();
		radio.writeAckPayload(pipeNo, &payLoadAck, sizeof(payLoadAck));

        radio.read(&recieved_data, sizeof(recieved_data));
       /* bool sts[3];

        radio.whatHappened( sts[0], sts[1], sts[2] );

        LOG_MSG( " " << sts[0] << sts[1] << sts[2]);*/

        if ( ! recieved_data.isValid() )
            continue;

        lastRecievedTime = millis();

        LOG_MSG(F("RCV  Speed:") << recieved_data.m_speed 
             << F(" Direction: ") << ((recieved_data.m_speed > 0) ? F("FORWARD") : F("BACKWARD"))
             << F(" Steering: ")  << recieved_data.m_steering 
             << ((recieved_data.m_steering > 0) ? F(" LEFT") : F(" RIGHT"))
		     << F("Bits") << recieved_data.m_b1 << F(" ") << recieved_data.m_b2 << F(" ") << recieved_data.m_b3 << F(" ") << recieved_data.m_b4 ) ;
		
		short prevSpeed = curSpeed;
		
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
			servo.write( curSteering = map( recieved_data.m_steering, 0 , 127 ,SERVO_ZERO, MAX_LEFT ));
        }
        else
        {
			servo.write( curSteering = map( recieved_data.m_steering, -127, 0, MAX_RIGHT , SERVO_ZERO ));
        }
		
		if (  recieved_data.m_b3 )
		{
			headLight.turn_on( Led::Brightness::_100 );
			backLight.turn_on( Led::Brightness::_50 );
		}
		else
		{
			headLight.turn_off();
			backLight.turn_off();
		}
		 
		if ( ( curSpeed - prevSpeed ) < 0 )
		{
			backLight.turn_on( Led::Brightness::_100 );
		}

		payLoadAck.speed = curSpeed;

		payLoadAck.batteryLevel = servo.read();
        radio.writeAckPayload( pipeNo, &payLoadAck, sizeof(payLoadAck) );
    }

    if ( lastRecievedTime < millis() - 3 * arduino::utils::RF_TIMEOUT_MS )
    {
		
        LOG_MSG("Lost connection");
      
        motor.stop();
        servo.write(SERVO_ZERO);
        curSteering = servo.read();
		curSpeed = 0;
		backLight.fade(500);
		headLight.fade(500);

		lastRecievedTime = millis();
    }
}
