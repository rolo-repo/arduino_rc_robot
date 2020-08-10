#ifndef Motor_h__
#define Motor_h__

class Motor
{
public:
	enum class Direction { FORWARD, BACKWARD , STOPED };
	typedef  unsigned char SPEED;


	virtual void forward( SPEED i_speed ) = 0;
	virtual void backward( SPEED i_speed ) = 0;
	
	//alias
	virtual void right(SPEED i_speed) { forward(i_speed); }
	virtual void left(SPEED i_speed) { backward(i_speed); }

	virtual void stop() = 0;
	virtual void begin() = 0;
	virtual void run(){};

	Direction  getDirection() const { return m_direction; }
	SPEED      getSpeed() const { return m_speed;  }
protected:
	Direction m_direction;
	SPEED	  m_speed;
};

#endif // Motor_h__