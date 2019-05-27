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

#define PIN unsigned int

unsigned char address[][6] = { "1Node","2Node","3Node","4Node","5Node","6Node" }; // pipe address
//char recieved_data[4];



//AF_DCMotor motor(3);

#define MAX_LEFT 160
#define MAX_RIGHT 60
#define OD_MIN_DISTANCE_CM 5

int SERVO_ZERO = 110;
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

#define _BTS7690_

//Motor
//const PIN dirPin       = 2;
//const PIN enablePin    = 3; //PWM
//const PIN breakPin     = 4;

//const PIN enableF   = 2;
//const PIN forward   = 3;

//const PIN enableB   = 4;
//const PIN backward  = 5;

//Steering Servo 
const PIN servoPin = 6;

//Objects detection
const PIN trigPin = 8;
const PIN echoPin = 7;

//Radio
const PIN radioCE = 9;
const PIN radioSCN = 10;

class Motor
{
    typedef  unsigned char SPEED;

public:
    enum Direction { FORWARD = 0, BACKWARD = ~0 };
    Motor() {}

    bool init()
    {
        PIN enable1 = 2;
        PIN enable2 = 4;
        PIN PWM1 = 3;
        PIN PWM2 = 5;
    
        pinMode(enable1, OUTPUT);
        pinMode(enable2, OUTPUT);
        pinMode(PWM1, OUTPUT);
        pinMode(PWM2, OUTPUT);
        
        attach( /*i_eChannel_1*/ 2, /* i_PWMChannel_1*/ 3, /*i_eChannel_2*/ 4, /*i_PWMChannel_2*/ 5);
    }

#ifdef _BTS7690_
    void attach( PIN i_eChannel_1, PIN i_PWMChannel_1 , PIN i_eChannel_2, PIN i_PWMChannel_2 )
#elif defined _L298NH_
    void attach( /*enable*/ PIN i_eChannel_1, /*speed */ PIN i_speed , /*direction */ PIN i_direction)
#elif defined _L298N_
    void attach(/*enable*/ PIN i_direction, /*speed*/ PIN i_speed, /*break*/ PIN i_break )
#endif
    {
        /*
        Option 1 - 2PWM outputs :
        Connect R_EN and L_EN to VCC(either short on the driver or wire it up), this effectively enables both directions to work.
            Then connect the RPWM and LPWM to 2 separate outputs on the Arduino.

            To control it, use digital write to set the direction you aren't using to 0, and analogwrite to set the other one to the desired setting. To stop it, set both to 0.

            Option 2 - 2 digital outputs, one PWM output :
        Connect R_EN and L_EN to the PWM output.Use analogwrite to set the desired speed.
            Connect RPWM and LPWM to the digital outputs.Set the desired direction to high and the undesired direction to 0.
            */
#ifdef _BTS7690_
        m_pin[0] = i_eChannel_1;
        m_pin[1] = i_PWMChannel_1;
        m_pin[2] = i_eChannel_2;
        m_pin[3] = i_PWMChannel_2;
#endif

        stop();
    }
    
    Motor& forward( SPEED i_speed , unsigned short i_time = 0 )
    {
        LOG_MSG(F("Farward ") << i_speed  );

        if ( i_speed < 10 )
            return stop();

        digitalWrite(m_pin[0], HIGH);
        digitalWrite(m_pin[2], HIGH);
       
        m_speed = constrain( i_speed, 0, 255 );
        analogWrite( m_pin[1], m_speed );
        analogWrite( m_pin[3], 0);

        m_dir = FORWARD;
        return *this;
    }

    Motor& backward( SPEED i_speed , unsigned short i_time = 0)
    {
        LOG_MSG(F("Backward ") << i_speed );
        if ( i_speed < 10 )
            return stop();

        digitalWrite(m_pin[0], HIGH);
        digitalWrite(m_pin[2], HIGH);

        m_speed = constrain( i_speed, 0, 255 );
        m_dir = BACKWARD;
        analogWrite(m_pin[1], 0 );
        analogWrite(m_pin[3], m_speed );

        return *this;
    }

    Motor& stop()
    {
        m_speed = 0;
        digitalWrite( m_pin[0], LOW);
        digitalWrite( m_pin[2], LOW);

        analogWrite( m_pin[1], 0 );
        analogWrite( m_pin[3], 0 );

        return *this;
    }

    Direction getDirection() const{
        return m_dir;
    }

    SPEED getSpeed() const {
        return m_speed;
    }

private:

    void switchDirection()
    {
        stop();
        m_dir = (Direction)(~m_dir);
    }

    PIN m_pin[4];

    Direction m_dir;
    SPEED m_speed;
};


Motor motor;
Servo servo;

RF24 radio( radioCE, radioSCN );

//returns distance in cm
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
   
    pinMode( 2, OUTPUT );
    pinMode( 3, OUTPUT );
    pinMode( 4, OUTPUT );
    pinMode( 5, OUTPUT );

    pinMode( A0, OUTPUT );

    motor.init();
    servo.attach( servoPin);

    radio.begin(); //активировать модуль
    radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
    radio.setRetries(0, 0);     //(время между попыткой достучаться, число попыток)
    radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
    radio.setPayloadSize(sizeof(Payload)/*32*/);     //размер пакета, в байтах
   
    radio.openReadingPipe(1, address[0]);      //хотим слушать трубу 0
    radio.setChannel(0x64);  //выбираем канал (в котором нет шумов!)

    radio.setPALevel(RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
    radio.setDataRate(RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
    //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

    radio.powerUp(); //начать работу
    radio.startListening();  //начинаем слушать эфир, мы приёмный модуль

    SERVO_ZERO = EEPROM.read(0);
}

void left( unsigned char i_angle )
{
    servo.write( i_angle);
   // delay(500);
}

void right( unsigned char i_angle )
{
    servo.write(i_angle);
    //delay(500);
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

void loop() 
{
    using namespace arduino::utils;
    byte pipeNo;
    static short curSteering = 90;
    static unsigned long lastRecievedTime = millis();
    static bool OD_ENABLED = !( 0 == getDistance() ); //If the sensor is connect the value will be different than 0
    static Payload recieved_data;
    static short LED = HIGH;
    short obstacleIteration = 0;
    while ( OD_ENABLED && getDistance() < OD_MIN_DISTANCE_CM )
    {
        LOG_MSG( "Obstacle detected " << getDistance() << " cm");

        if ( motor.getDirection() == Motor::FORWARD )
        {
            motor.backward( 100 );
            delay(1000);
            if ( servo.read() > SERVO_ZERO )
            {
                left( MAX_LEFT );
            }
            else
            {
                right( MAX_RIGHT ); 
            }
            motor.forward( 100 );
            delay(500);
        }
        else
        {
            // do nothing the sensor is head looking
        }
        if ( ++obstacleIteration > 10 )
        {
            LOG_MSG("Not abale to avoid obstacle");
            servo.write(SERVO_ZERO);
            delay(100);
            motor.backward( 100 );
            delay(2000);
            return;
        }
    }
   
    while ( radio.available(&pipeNo) ) 
    {   
        digitalWrite(A0, LED = ( LED == HIGH ) ? LOW : HIGH );
       
        radio.read(&recieved_data, sizeof(recieved_data));
       
        //bool sts[3];

       // radio.whatHappened( sts[0], sts[1], sts[2] );

       // LOG_MSG( " " << sts[0] << sts[1] << sts[2]);

        if ( ! recieved_data.isValid() )
            continue;

        lastRecievedTime = millis();

        LOG_MSG(F("RCV  Speed:") << recieved_data.m_speed 
             << F(" Direction: ") << ((recieved_data.m_speed > 0) ? F("FORWARD") : F("BACKWARD"))
             << F(" Steering: ")  << recieved_data.m_steering 
             << ((recieved_data.m_steering > 0) ? F(" LEFT") : F(" RIGHT"))) ;


        if ( recieved_data.m_speed > 0 )
        {
            motor.forward( map( recieved_data.m_speed, 0, 127, 0, 255 ) );
        }
        else
        {
            motor.backward( map( recieved_data.m_speed, -127 , 0 , 255 , 0 ) );
        }
        
        if ( recieved_data.m_steering > 0 )
        {
            left( curSteering = map( recieved_data.m_steering, 0, 127, SERVO_ZERO, MAX_LEFT ));
        }
        else
        {
            right( curSteering = map( recieved_data.m_steering, -127, 0, MAX_RIGHT , SERVO_ZERO ));
        }
       

      //  servo.write(map(recieved_data.m_steering, -127, 127, MAX_RIGHT+ 50 , MAX_LEFT ));
        
       // LOG_MSG("New  Speed:" << motor.getSpeed() << " Steering: " << curSteering << " Direction: " << (( motor.getDirection() == Motor::FORWARD ) ? "FORWARD" : "BACKWARD" ) );
    
      //  servo.write( curSteering );
       
        if ( recieved_data.m_bits.m_b1 )
        {
            LOG_MSG("Update zero servo " << servo.read());
            EEPROM.update( 0, curSteering );
        }

        ack.speed = motor.getSpeed();

        ack.batteryLevel = servo.read();
        radio.writeAckPayload( pipeNo, &ack, sizeof(ack) );

       // motor.setSpeed(curSpeed);
    }
    
    if ( lastRecievedTime < millis() - 3 * arduino::utils::RF_TIMEOUT_MS )
    {
        LOG_MSG("Lost connection");
        lastRecievedTime = millis();
        motor.stop();
        servo.write(SERVO_ZERO);
        curSteering = servo.read();
    }
}