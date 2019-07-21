#ifndef Motor_h__
#define Motor_h__

class Motor
{
public:
	enum class Direction { FORWARD, BACKWARD , STOPED };
	typedef  unsigned char SPEED;


	virtual void forward( SPEED i_speed ) = 0;
	virtual void backward( SPEED i_speed ) = 0;
	virtual void stop() = 0;
	virtual void begin() = 0;

	virtual Direction  getDirection() const { return m_direction; }

protected:
	Direction m_direction;
};

#endif // Motor_h__