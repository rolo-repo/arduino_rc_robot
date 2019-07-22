#include "SerialOutput.h"
#include "Constants.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class Led
{
public:
	
	enum class Brightness { _20, _50, _100 };
	
	Led( PIN i_ledPin = LED_BUILTIN ) :m_pin(i_ledPin) 
	{ 
		pinMode(m_pin, OUTPUT);
		m_status = LedStS::OFF;
		digitalWrite(m_pin, LOW);
	}

	void on(Brightness i_brtness = Brightness::_50 );
	void off()
	{
		digitalWrite(m_pin, LOW );
		m_status = LedStS::OFF;
	}
	void blynk(Brightness i_brtness = Brightness::_50 )
	{
		(m_status == LedStS::OFF) ? on(i_brtness) : off();
	}

	void rapid_blynk( unsigned long i_time_ms );
	void fade(unsigned long  i_time_ms);

private:
	enum class LedStS { ON, OFF, BLYNK };
	PIN m_pin;

	LedStS m_status;
};

void Led::on( Brightness i_brtness )
{
	switch (i_brtness)
	{
	case Brightness::_20:
		analogWrite( m_pin, (int ) map ( 20 , 0, 100 , 0 , 127 ) );
		break;
	case Brightness::_50:
		analogWrite(m_pin, ( int ) map( 50, 0, 100, 0, 127 ));
		break;
	case Brightness::_100:
		digitalWrite(m_pin, HIGH );
		break;
	}

	m_status = LedStS::ON;
}

void Led::rapid_blynk( unsigned long i_time_ms )
{
	bool sts_changed = false;
	i_time_ms = max( i_time_ms, 120 ); // min is 120 ms

	while ( ( i_time_ms -= 30 )  > 0 || sts_changed )
	{
		sts_changed = ~sts_changed;
		
		blynk( Brightness::_100 );
		delay( 30 );
	}
}


void Led::fade ( unsigned long i_time_ms )
{
	off();

	unsigned char brightness = 0;
	char fadeAmount = max( 5, 15000 / i_time_ms );
	
	i_time_ms = max(i_time_ms, 120); // min is 120 ms

	while( ( i_time_ms -= 30 ) > 0 )
	{
		analogWrite( m_pin, brightness );
		brightness = brightness + fadeAmount;

		// reverse the direction of the fading at the ends of the fade:
		if (brightness <= 0 || brightness >= 255) 
			fadeAmount = -fadeAmount;
		
		delay( 30 );
	}

	off();
}
