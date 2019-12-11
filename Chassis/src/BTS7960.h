#ifndef BTS7960_h__
#define BTS7960_h__

#include <Constants.h>
#include "Motor.h"

#include <SerialOutput.h>

class BTS7960_1PWM : public Motor
{
private :
	void(*callback_break)();
public:
	/*
		R_EN -|
			  |- PWM pin
		L_EN -|

		L_PWM -| digital pin
		R_PWM -| digital pin
	*/
	BTS7960_1PWM( PIN i_forward , PIN i_backward , PIN i_speed  /*PWM pin */ , void(*i_callback_break)()) : m_L_PWM(i_forward), m_R_PWM(i_backward), m_ENABLE(i_speed)
	{
		m_direction = Direction::STOPED;
		m_speed = 0;
		callback_break = i_callback_break;
	}

	void begin()
	{
		pinMode(m_R_PWM, OUTPUT);
		pinMode(m_L_PWM, OUTPUT);
		pinMode(m_ENABLE, OUTPUT);
		stop();
	}

	virtual void  forward( SPEED i_speed )
	{
		LOG_MSG( F("Motor -- move forward speed ") << i_speed );

		if (i_speed < 10)
			return stop();

		digitalWrite( m_L_PWM, HIGH);
		digitalWrite( m_R_PWM , LOW);

		analogWrite( m_ENABLE, constrain(i_speed, 0, 255));

		m_direction = Direction::FORWARD;
		setSpeed(i_speed);
	}

	virtual void backward( SPEED i_speed )
	{
		LOG_MSG(F("Motor -- move backward speed ") << i_speed);

		if (i_speed < 10)
			return stop();

		digitalWrite(m_L_PWM, LOW);
		digitalWrite(m_R_PWM, HIGH);

		analogWrite( m_ENABLE, constrain(i_speed, 0, 255));

		m_direction = Direction::BACKWARD;
		setSpeed(i_speed);
	}

	virtual void stop()
	{
		digitalWrite(m_L_PWM, LOW);
		digitalWrite(m_R_PWM, LOW);
		m_direction = Direction::STOPED;
		setSpeed(0);
	}

protected:
	void setSpeed( SPEED i_speed )
	{
		if ( m_speed > i_speed )
			callback_break();

		m_speed = i_speed;
	}

private:
	
	PIN m_R_PWM;  // backward
	PIN m_ENABLE; //speed
	PIN m_L_PWM;  // forward
};


class BTS7960_2PWM : public Motor
{
	/*
		R_EN -|
			  |- 5V pin
		L_EN -|

		L_PWM -| PWM pin
		R_PWM -| PWM pin
	*/
public:

	BTS7960_2PWM(PIN i_forward /*PWM*/ ,PIN i_backward /*PWM*/ ):m_L_PWM(i_forward), m_R_PWM(i_backward){}

	void begin()
	{
		pinMode(m_R_PWM, OUTPUT);
		pinMode(m_L_PWM, OUTPUT);
		stop();
	}
	virtual void  forward(SPEED i_speed)
	{
		LOG_MSG(F("Motor -- move forward speed ") << i_speed);

		if (i_speed < 10)
			return stop();

		analogWrite(m_L_PWM, constrain(i_speed, 0, 255));
		analogWrite(m_R_PWM, 0);

		m_direction = Direction::FORWARD;
	}

	virtual void backward(SPEED i_speed)
	{
		LOG_MSG(F("Motor -- move backward speed ") << i_speed);

		if (i_speed < 10)
			return stop();

		analogWrite(m_R_PWM, constrain(i_speed, 0, 255));
		analogWrite(m_L_PWM, 0);

		m_direction = Direction::BACKWARD;
	}

	virtual void stop()
	{
		analogWrite(m_L_PWM, 0);
		analogWrite(m_R_PWM, 0);
		m_direction = Direction::STOPED;
	}

private :
	PIN m_R_PWM;  // backward
	PIN m_L_PWM;  // forward
};
#endif // BTS7960_h__
