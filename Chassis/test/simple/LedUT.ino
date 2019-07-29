#ifdef UNIT_TEST

// Adapted from:
// https://github.com/mmurdoch/arduinounit/blob/master/examples/basic/basic.ino
#define ENABLE_LOGGER
#include <AUnit.h>
#include "Led.h"


#include "SerialOutput.h"

static int counter;
void digitalWrite(uint8_t pin, uint8_t val)
{
	//LOG_MSG("pin: " << pin << " val:" << val);
	counter++;
}

void analogWrite(uint8_t pin, int val)
{
	counter++;
	//LOG_MSG("pin: " << pin << " val:" << val);
}

class TestLed: public Led
{
public:

	int getSts()
	{
		return (int)m_status;
	}
};



TestLed led;

test(check_on)
{
	LOG_MSG("Test check_ON");
	counter = 0;
	led.turn_on();
	assertEqual(led.getSts(), 0);
	assertEqual(counter, 1);
}

test(check_off)
{
	LOG_MSG("Test check_OFF");
	counter = 0;
	led.turn_off();
	assertEqual(led.getSts(), 1);
	assertEqual(counter, 1);
}

test(check_blynk)
{
	LOG_MSG("Test check_Blynk");
	led.turn_off();
	counter = 0;
	led.blynk();
	assertEqual(led.getSts(), 0);
	assertEqual(counter, 1);
	led.blynk();
	assertEqual(led.getSts(), 1);
	assertEqual(counter, 2);
}

test(fade)
{
	LOG_MSG("Test check_Fade");
	counter = 0;
	LOG_MSG("==Fade 1000==");
	long time = millis();
	led.fade(1000);
	assertEqual(counter, 102);
	assertEqual(led.getSts(), 1);
	assertMoreOrEqual( (short ) ( millis() - time), 1000);
	LOG_MSG("==Fade 100==");
	counter = 0;
	time = millis();
	led.fade(100);
	assertEqual(counter, 102);
	assertEqual(led.getSts(), 1);
	assertMoreOrEqual((short)(millis() - time), 100);
	LOG_MSG("==Fade 500==");
	counter = 0;
	time = millis();
	led.fade(500);
	assertEqual(counter,102);
	assertEqual(led.getSts(), 1);
	assertMoreOrEqual((short)(millis() - time), 500);
}

test(rapid_blink)
{
	LOG_MSG("Test check_rapid_blynk");
	led.turn_off();
	LOG_MSG("==Rapid blink 1000==");
	counter = 0;
	long time = millis();
	led.rapid_blynk(1000);
	assertEqual(led.getSts(), 1);
	assertMoreOrEqual((short)(millis() - time), 1000);
	assertLessOrEqual((short)(millis() - time), 1010);

	LOG_MSG("==Rapid blink 100==");
	counter = 0;
	time = millis();
	led.rapid_blynk(100);
	assertEqual(led.getSts(), 1);
	assertMoreOrEqual((short)(millis() - time), 100);
	assertLessOrEqual((short)(millis() - time), 110);

	LOG_MSG("==Rapid blink 500==");
	counter = 0;
	time = millis();
	led.rapid_blynk(500);
	assertEqual(led.getSts(), 1);
	assertMoreOrEqual((short)(millis() - time), 500);
	assertLessOrEqual((short)(millis() - time), 510);
}


class TestRGBLed : public RGBLed
{
public : 
	TestRGBLed():RGBLed(1,2,3){}

	char getLastUsedLed()
	{
		return this->m_lastUsedLed;
	}

	Led led(LedType i_led)
	{
		return getLed(i_led);
	}
};

TestRGBLed rgb_led;

test(rgb_check_on)
{
	LOG_MSG("Test rgb_check_on");
	counter = 0;
	rgb_led.turn_on(RGBLed::LedType::Blue);
	assertEqual( rgb_led.getLastUsedLed() , static_cast<char> ( RGBLed::LedType::Blue ) );
	assertEqual(counter, 4);

	counter = 0;
	rgb_led.turn_on(RGBLed::LedType::Green);
	assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Green));
	assertEqual(counter, 4);

	counter = 0;
	rgb_led.turn_on(RGBLed::LedType::Red);
	assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Red));
	assertEqual(counter, 4);
}

test(rgb_blynk)
{
	LOG_MSG("Test rgb_blynk");
	rgb_led.turn_off();
	rgb_led.turn_on(RGBLed::LedType::Blue);
	rgb_led.turn_off();
	counter = 0;
	for ( int i = 0 ; i  < 3 ; i++)  
	{
		rgb_led.blynk();
		assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Red));

		rgb_led.blynk();
		assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Green));

		rgb_led.blynk();
		assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Blue));

	}
}


test(rgb_rapid_blynk)
{
	LOG_MSG("Test rgb_rapid_blynk");
	rgb_led.turn_off();
	rgb_led.turn_on(RGBLed::LedType::Blue);
	rgb_led.turn_off();
	counter = 0;
	long time = millis();
	LOG_MSG("==Rapid blink 130==");
	rgb_led.rapid_blynk(130);
	assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Red));
	assertMoreOrEqual((short)(millis() - time), 130);
	assertLessOrEqual((short)(millis() - time), 140);

	time = millis();
	LOG_MSG("==Rapid blink 150==");
	rgb_led.rapid_blynk(150);
	assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Green));
	assertMoreOrEqual((short)(millis() - time), 150);
	assertLessOrEqual((short)(millis() - time), 160);

	time = millis();
	LOG_MSG("==Rapid blink 500==");
	rgb_led.rapid_blynk(500);
	assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Blue));
	assertMoreOrEqual((short)(millis() - time), 500);
	assertLessOrEqual((short)(millis() - time), 510);
}


/*
test(incorrect) {
  int x = 1;
  assertEqual(x, 1);
}
*/
void setup() {
  delay(1000); // wait for stability on some boards to prevent garbage Serial
  Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
 // while(!Serial); // for the Arduino Leonardo/Micro only
}

void loop() {
  // Should get:
  // TestRunner summary:
  //    1 passed, 1 failed, 0 skipped, 0 timed out, out of 2 test(s).
  aunit::TestRunner::run();
}
#endif // TEST
