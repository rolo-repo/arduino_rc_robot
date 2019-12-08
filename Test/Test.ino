// Adapted from:
// https://github.com/mmurdoch/arduinounit/blob/master/examples/basic/basic.ino
#ifdef ESP8266

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

class TestLed : public Led
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
	assertMoreOrEqual((short)(millis() - time), 1000);
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
	assertEqual(counter, 102);
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
public:
	TestRGBLed() :RGBLed(1, 2, 3) {}

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
	assertEqual(rgb_led.getLastUsedLed(), static_cast<char> (RGBLed::LedType::Blue));
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
	for (int i = 0; i < 3; i++)
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

#include "Time.h"
#include "TimeManager.h"

arduino::utils::Timer pumpTimer("[PUMP]");

bool tFuncStatus = false;
void tFunc(long& i_time)
{
	LOG_MSG("executing task on " << i_time);

	i_time += 5;
	tFuncStatus = ~tFuncStatus;

	LOG_MSG("rescheduling to" << i_time);
}

test(test_timer)
{
	using namespace arduino::utils;
	TIME.begin();

	pumpTimer.addTask( TIME.getEpochTime() , tFunc);

	unsigned long time = millis();
	LOG_MSG( "Test Timer start on " << TimeValue(TIME.getEpochTime()) << " now() " << now() );

	//while ( millis() < time + 3000 ) {
		pumpTimer.run();
	//}
	assertEqual(tFuncStatus, true);
}

#include "RFcom.h"

test(bits_assigh)
{
	using namespace arduino::utils;
	Payload data;
	data.m_b1 = 1;
	assertEqual(data.m_b1, 1);
	assertEqual(data.m_b2, 0);
	assertEqual(data.m_b3, 0);
	assertEqual(data.m_b4, 0);

	data.m_b1 = 0;
	data.m_b2 = 1;
	data.m_b3 = 0;
	data.m_b4 = 0;
	assertEqual(data.m_b1, 0);
	assertEqual(data.m_b2, 1);
	assertEqual(data.m_b3, 0);
	assertEqual(data.m_b4, 0);

	data.m_bits = 3;

	assertEqual(data.m_b1, 1);
	assertEqual(data.m_b2, 1);
	assertEqual(data.m_b3, 0);
	assertEqual(data.m_b4, 0);

	data.m_bits = 8;
	assertEqual(data.m_b1, 0);
	assertEqual(data.m_b2, 0);
	assertEqual(data.m_b3, 0);
	assertEqual(data.m_b4, 1);

	assertEqual((int)data.m_data, 0);

}


test(cksum)
{
	using namespace arduino::utils;
	Payload data1;

	data1.m_b1 = 1;
	data1.m_b2 = 1;

	data1.m_speed = 100;
	data1.m_steering = -70;

	Payload data2;

	data2.m_b1 = 1;
	data2.m_b2 = 1;

	data2.m_speed = 100;
	data2.m_steering = -70;

	assertNotEqual((int)data2.finalize()->m_cksum, 0);
	assertEqual(data2.finalize()->m_cksum, data1.finalize()->m_cksum);

}
test(compare_equal)
{
	using namespace arduino::utils;
	Payload data1;
	Payload data2;

	data1.m_b1 = 1;
	data1.m_b2 = 1;

	data2.m_b1 = 1;
	data2.m_b2 = 1;

	assertEqual(data1 == data2, true);

	data1.m_b3 = 1;
	data1.m_b4 = 1;

	data2.m_b3 = 1;
	data2.m_b4 = 1;

	assertEqual(data1 == data2, true);

	data1.m_speed = 127;
	data1.m_steering = -127;

	data2 = data1;
	assertEqual(data1 == data2, true);

	data2.m_speed -= 5;
	data1.m_steering += 7;

	assertEqual(data1 == data2, true);

	data1.finalize();
	assertNotEqual((int)data1.finalize()->m_cksum, 0);

	data2 = data1;

	assertEqual(data2.isValid(), true);

}

test(compare_not_equal)
{
	using namespace arduino::utils;
	Payload data1;

	//memset(&data1, 0x0, sizeof(Payload));
	data1.m_b1 = 1;
	data1.m_b2 = 1;
	data1.m_b3 = 1;
	data1.m_b4 = 1;

	Payload data2;
	//memset(&data2, 0x0, sizeof(Payload));
	data2.m_b1 = 1;
	data2.m_b2 = 0;
	data2.m_b3 = 1;
	data2.m_b4 = 1;

	assertEqual(data1 == data2, false);

	data2.m_b1 = 1;
	data2.m_b2 = 1;
	data2.m_b3 = 0;
	data2.m_b4 = 1;

	assertEqual(data1 == data2, false);

	data1.m_speed = 127;
	data1.m_steering = -127;

	data2 = data1;

	data2.m_speed -= 11;

	assertEqual(data1 == data2, false);

	data2.m_steering += 11;

	assertEqual(data1 == data2, false);
}
test(save_and_load)
{
	unsigned char memory[128];
	short index = 0;


	unsigned short zero = 500;
	unsigned short analogLimits[2] = { 100 , 28 };
	bool m_reversed = true;

	//save
	memory[index++] = 0b11001100;

	const unsigned char *t = (const unsigned char*)analogLimits;

	for (auto i = 0; i < sizeof(analogLimits); i++)
	{
		memory[index++] = t[i];
	}

	memory[index++] = (unsigned char)zero;
	memory[index++] = (unsigned char)(zero >> 8);
	memory[index++] = (unsigned char)m_reversed;

	unsigned short lastIndex = index;
	//load
	index = 0;
	assertEqual(memory[index++], 0b11001100);
	for (auto i = 0; i < sizeof(analogLimits); i++)
	{
		assertEqual(t[i], memory[index++]);
	}

	assertEqual(zero, memory[index++] | memory[index++] << 8);
	assertEqual((short)m_reversed, memory[index++] & 0x1);

	assertEqual(lastIndex, index);
}

void setup() {
	delay(1000); // wait for stability on some boards to prevent garbage Serial
	Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
//	
   // while(!Serial); // for the Arduino Leonardo/Micro only
}

void loop() {
	// Should get:
	// TestRunner summary:
	//    1 passed, 1 failed, 0 skipped, 0 timed out, out of 2 test(s).
	aunit::TestRunner::run();
}

#endif // ESP8266