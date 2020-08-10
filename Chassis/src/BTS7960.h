#ifndef BTS7960_h__
#define BTS7960_h__

#include <Constants.h>
#include "Motor.h"

#include <SerialOutput.h>

class BTS7960_1PWM : public Motor
{
protected :
	void(*callback_break)();
public:
	/*
		R_EN -|
			  |- PWM pin
		L_EN -|

		L_PWM -| digital pin
		R_PWM -| digital pin
	*/
	BTS7960_1PWM(PIN i_forward, PIN i_backward, PIN i_speed  /*PWM pin */, void(*i_callback_break)() = []() {} ) : m_L_PWM(i_forward), m_R_PWM(i_backward), m_ENABLE(i_speed)
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
		LOG_MSG( F("Motor speed ") << i_speed );

		if (i_speed < m_deadZone)
			return stop();

		if ( m_direction != Motor::Direction::FORWARD ){
			stop();
		}

		digitalWrite( m_L_PWM, HIGH);
		digitalWrite( m_R_PWM , LOW);

		//analogWrite( m_ENABLE, constrain(i_speed, 0, 255));

		m_direction = Direction::FORWARD;
		setSpeed(i_speed);
	}

	virtual void backward( SPEED i_speed )
	{
		LOG_MSG(F("Motor speed ") << i_speed);

		if ( i_speed < m_deadZone ) {
			return stop();
		}

		if ( m_direction != Motor::Direction::BACKWARD ){
			stop();
		}

		digitalWrite(m_L_PWM, LOW);
		digitalWrite(m_R_PWM, HIGH);

	//	analogWrite( m_ENABLE, constrain(i_speed, 0, 255));

		m_direction = Direction::BACKWARD;
		setSpeed(i_speed);
	}

	virtual void stop()
	{
		LOG_MSG(F("Motor speed ") << m_speed);

		setSpeed( 0 );
		m_direction = Direction::STOPED;
		if ( 0 == m_speed )
		{
			digitalWrite(m_L_PWM, LOW);
			digitalWrite(m_R_PWM, LOW);
		
		}
		callback_break();
	}

protected:
	virtual void setSpeed( SPEED i_speed )
	{
		m_speed = constrain( i_speed, 0, 255 );
		analogWrite( m_ENABLE, m_speed );
	}

protected:
	
	PIN m_R_PWM;  // backward
	PIN m_ENABLE; //speed
	PIN m_L_PWM;  // forward

	short m_deadZone = 30;
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

		if (i_speed < m_deadZone)
			return stop();

		analogWrite(m_L_PWM, constrain(i_speed, 0, 255));
		analogWrite(m_R_PWM, 0);

		m_direction = Direction::FORWARD;
	}

	virtual void backward(SPEED i_speed)
	{
		LOG_MSG(F("Motor -- move backward speed ") << i_speed);

		if (i_speed < m_deadZone)
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

	short m_deadZone = 30;
};

class BTS7960_1PWM_Smooth : public BTS7960_1PWM
{
private:
	SPEED m_targetSpeed;

public :
	BTS7960_1PWM_Smooth(PIN i_forward, PIN i_backward, PIN i_speed  /*PWM pin */, void(*i_callback_break)()) : BTS7960_1PWM (i_forward, i_backward, i_speed, i_callback_break) 
	{
		m_targetSpeed = 0;
	}

	void run()
	{
		static long tm = millis();

		if ( m_speed == m_targetSpeed )
			return;

		if ( tm + 50 < millis() )
		{
			LOG_MSG(F("BTS7960_1PWM_Smooth requested speed ") << m_speed);
			
			if ( m_speed > m_targetSpeed )
			{
				m_speed -= max((m_speed - m_targetSpeed) / 10, 1);
				callback_break();//breaking
			}
			else
			{
				m_speed += max((m_targetSpeed - m_speed) / 10, 1);
			}

			if ( m_speed < m_deadZone )
			{
				digitalWrite(m_L_PWM, LOW);
				digitalWrite(m_R_PWM, LOW);
			}

			analogWrite( m_ENABLE, constrain(m_speed, 0, 255) );
			tm = millis();
		}
	}

protected:
    void setSpeed( SPEED i_speed )
	{
		m_targetSpeed = i_speed;
	}


};


#endif // BTS7960_h__
