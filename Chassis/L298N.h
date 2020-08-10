#pragma once

#include <Constants.h>
#include "Motor.h"

#include <SerialOutput.h>

class  L298N_1PWM : public Motor
{
protected:
	void(*callback_break)();
public:
	/*
		EN	-  | - PWM pin

		L_PWM -| digital pin
		R_PWM -| digital pin
	*/
	L298N_1PWM(PIN i_forward, PIN i_backward, PIN i_speed  /*PWM pin */, void(*i_callback_break)() = []() {} ) : m_L_PWM(i_forward), m_R_PWM(i_backward), m_ENABLE(i_speed)
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

	virtual void forward( SPEED i_speed )
	{
		LOG_MSG(F("Motor speed ") << i_speed);

		if (i_speed < m_deadZone)
			return stop();

		if (m_direction != Motor::Direction::FORWARD) {
			stop();
		}

		if ( i_speed < m_speed )
			callback_break();
		digitalWrite(m_L_PWM, HIGH);
		digitalWrite(m_R_PWM, LOW);

		m_direction = Direction::FORWARD;
		analogWrite( m_ENABLE, m_speed = constrain(i_speed, 0, 255) );
	}

	virtual void backward( SPEED i_speed )
	{
		LOG_MSG(F("Motor speed ") << i_speed);

		if (i_speed < m_deadZone) {
			return stop();
		}

		if (m_direction != Motor::Direction::BACKWARD) {
			stop();
		}

		if ( i_speed < m_speed )
			callback_break();

		digitalWrite(m_L_PWM, LOW);
		digitalWrite(m_R_PWM, HIGH);

		m_direction = Direction::BACKWARD;
		analogWrite(m_ENABLE, m_speed = constrain(i_speed, 0, 255));
	}

	virtual void stop()
	{
		LOG_MSG(F("Motor speed ") << m_speed);

		m_speed = 0;
		digitalWrite( m_ENABLE, LOW );
		m_direction = Direction::STOPED;
		callback_break();
		yield();
	}


private:

	PIN m_R_PWM;  // backward
	PIN m_ENABLE; //speed
	PIN m_L_PWM;  // forward

protected:
	short m_deadZone = 40;
};

