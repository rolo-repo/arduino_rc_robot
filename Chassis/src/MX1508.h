#ifndef MX1508_h__
#define MX1508_h__

#include "Motor.h"
#include "Constants.h"

class MX1508 : public Motor
{
public:

	MX1508(PIN i_left, PIN i_right) : m_R_ENABLE(i_right), m_L_ENABLE(i_left)
	{
	}

	void forward(SPEED i_speed = 0)
	{
		if (i_speed < 15) { stop();	return; }

		analogWrite(m_R_ENABLE, m_speed = constrain(i_speed, 0, 255));
		digitalWrite(m_L_ENABLE, LOW);
		m_direction = Motor::Direction::FORWARD;
	}

	void backward(SPEED i_speed = 0)
	{
		if (i_speed < 15) { stop();	return; }

		digitalWrite(m_R_ENABLE, LOW);
		analogWrite(m_L_ENABLE, m_speed = constrain(i_speed, 0, 255));
		m_direction = Motor::Direction::BACKWARD;
	}

	//Alias
	void left(const SPEED& i_speed)
	{
		return forward(i_speed);
	}

	void right(const SPEED& i_speed)
	{
		return backward(i_speed);
	}

	void stop()
	{
		digitalWrite(m_R_ENABLE, LOW);
		digitalWrite(m_L_ENABLE, LOW);
		m_direction = Motor::Direction::STOPED;
		m_speed = 0;
	}

	void begin()
	{
		pinMode(m_R_ENABLE, OUTPUT);
		pinMode(m_L_ENABLE, OUTPUT);
		stop();
	}

private:
	PIN m_R_ENABLE;  //right
	PIN m_L_ENABLE;  //left


};

#endif // MX1508_h__