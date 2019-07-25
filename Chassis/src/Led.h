#include "SerialOutput.h"
#include "Constants.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class Led
{
	friend class RGBLed;
public:
	
	enum class Brightness { _20, _50, _100 };
	
	Led( PIN i_ledPin = LED_BUILTIN ) :m_pin(i_ledPin) 
	{ 
		pinMode(m_pin, OUTPUT);
		m_status = LedStS::OFF;
		digitalWrite(m_pin, LOW);
	}

	void turn_on(Brightness i_brtness = Brightness::_50 );
	void turn_off()
	{
		digitalWrite(m_pin, LOW );
		m_status = LedStS::OFF;
	}
	void blynk(Brightness i_brtness = Brightness::_50 )
	{
		(m_status == LedStS::OFF) ? turn_on(i_brtness) : turn_off();
	}

	void rapid_blynk( unsigned long i_time_ms );
	void fade(unsigned long  i_time_ms);

private:
	enum class LedStS { ON, OFF, BLYNK };
	PIN m_pin;

	LedStS m_status;
};

class RGBLed
{
public:
	enum class  LedType: char { Red = 0b001 ,Green = 0b010 , Blue = 0b100 };

	RGBLed(PIN i_green, PIN i_blue, PIN i_red) : m_green(i_green), m_blue(i_blue), m_red(i_red) {
		m_lastUsedLed = (char)LedType::Green;
	}

	void turn_on( LedType i_type )
	{
		turn_off();
		getLed(i_type).turn_on(Led::Brightness::_100);
	}

	void turn_off ()
	{
		m_red.turn_off();
		m_blue.turn_off();
		m_green.turn_off();
	}

	void blynk ()
	{
		turn_on( (LedType) ( ( m_lastUsedLed == static_cast<char>(LedType::Blue) ) ? static_cast<char>(LedType::Red) : m_lastUsedLed << 1 ) ) ;
	}

	void rapid_blynk(unsigned long i_time_ms);

	void fade(unsigned long  i_time_ms, LedType i_type);

	void fade(unsigned long  i_time_ms);

private:

	Led& getLed( LedType i_type )
	{
		m_lastUsedLed = static_cast<char>(i_type);

		switch (i_type)
		{
		case LedType::Red:
			return m_red;
		case LedType::Blue:
			return m_blue;
		case LedType::Green:
			return m_green;
		}
	}

	Led m_green;
	Led m_blue;
	Led m_red;
	
	char m_lastUsedLed;
};

void Led::turn_on( Brightness i_brtness )
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
	this->turn_off();

	unsigned char brightness = 0;
	char fadeAmount = max( 5, ( 255 * 30 * 2 ) / i_time_ms );
	
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

	this->turn_off();
}

void RGBLed::fade(unsigned long  i_time_ms, LedType i_type)
{
	turn_off();

	this->getLed( i_type ).fade(i_time_ms);
}

void RGBLed::fade( unsigned long  i_time_ms )
{
	this->fade( i_time_ms / 3 , (LedType)((m_lastUsedLed == static_cast<char>(LedType::Blue)) ? static_cast<char>(LedType::Red) : m_lastUsedLed << 1));
	this->fade( i_time_ms / 3 , (LedType)((m_lastUsedLed == static_cast<char>(LedType::Blue)) ? static_cast<char>(LedType::Red) : m_lastUsedLed << 1));
	this->fade( i_time_ms / 3 , (LedType)((m_lastUsedLed == static_cast<char>(LedType::Blue)) ? static_cast<char>(LedType::Red) : m_lastUsedLed << 1));
}

void RGBLed::rapid_blynk(unsigned long i_time_ms)
{
	i_time_ms = max(i_time_ms, 120); // min is 120 ms

	while ((i_time_ms -= 30) > 0)
	{
		blynk();
		delay(30);
	}
}
