#ifdef UNIT_TEST


#include <AUnit.h>
#define ENABLE_LOGGER
#include "SerialOutput.h"
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
	assertEqual( data2.finalize()->m_cksum, data1.finalize()->m_cksum );

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

	assertEqual( data1 == data2 , true );

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

	assertEqual( data2.isValid() , true);

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