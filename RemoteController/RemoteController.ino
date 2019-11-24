/*
	Name:       RFController.ino
	Created:	03/18/19 17:14:28
	Author:     NTNET\ROMANL
*/
#ifndef UNIT_TEST
//#define ENABLE_LOGGER
#include <SPI.h>          // библиотека для работы с шиной SPI
#include "nRF24L01.h"     // библиотека радиомодуля
#include "RF24.h"         // ещё библиотека радиомодуля
#include <EEPROM.h>

#include "RFcom.h"
//++addr &= EEPROM.length() - 1;

#include "ScopedGuard.h"
#include "SerialOutput.h"
//#include "Constants.h"

#define OLED_RESET 0  // GPIO0
#include "U8g2lib.h"

#include "Button.h"
#include "Switch.h"
#include "InteruptEncoder.h"

#include "Led.h"

#define PIN unsigned int

#define STS_LED_PIN 2
#define SLCR_B_PIN A1

#define ENC_PIN1 3
#define ENC_PIN2 A0

#define S1_PIN 4
#define S2_PIN 5
#define S3_PIN 6
#define S4_PIN 7
#define B1_PIN 8

#define CE_PIN 9
#define SCN_PIN 10

//////////////////////////////////////////////////////////////////////////
#define J1_V_PIN A7  // SPEED
#define J1_H_PIN A6
#define J2_V_PIN A3
#define J2_H_PIN A2  //STEERING

#define J1_PIN J1_V_PIN
#define J2_PIN J1_H_PIN
#define J3_PIN J2_H_PIN
#define J4_PIN J2_V_PIN

char  MAX = 127;
char  MIN = -127;

char buff_4[5];

constexpr short speed = 0;
constexpr short steering = 1;
constexpr short not_asighned = 2;

#define HEADER_FONT u8g2_font_9x15B_tr
#define SMALL_FONT u8g2_font_micro_tn
#define MEDIUM_FONT u8g2_font_6x12_tr
#define BIG_FONT HEADER_FONT//u8g2_font_10x20_mr

//U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);
//U8GLIB_SH1106_128X64 display(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);
U8G2_SH1106_128X64_NONAME_2_HW_I2C display(U8G2_R0, /* reset=*/ OLED_RESET);
#define DISPLAY(...) display.firstPage(); do { __VA_ARGS__ ;} while( display.nextPage() );

struct Joystick
{
	enum Side { UP = 0, DOWN = 1, RIGHT = 0, LEFT = 1 };
private:
	unsigned short zero = 0;
	unsigned short sens[2] = { 1023 , 0 };
	short m_max = MAX;
	short m_min = MIN;

	short function;

	//typedef short(Joystick::*adjust)(const long&) const ;
	using formula = short(Joystick::*)(const long&) const;
	formula m_formula;
//public:
	bool m_switched = false;
	PIN m_pin = 0;

	mutable short m_lastRead = 0;
public:
	Joystick(PIN i_pin) : m_pin(i_pin), function(not_asighned), m_formula(&parabola127){}

	short read() const
	{
		short value = analogRead(m_pin);
		
		if (m_lastRead && abs( value - m_lastRead ) > 300 )
		{
			return m_lastRead;
		}

		m_lastRead = value;

		if (abs(value - zero) < 10)
			return 0;
		
		if ( !m_switched)
		{
			return constrain((this->*m_formula)((value >= zero)
				? map(value, zero, 1023 - sens[UP], 0, m_max)
				: map(value, 0 + sens[DOWN], zero, m_min, 0)
				)
				, MIN, MAX);
		}
		else
		{
			return constrain((this->*m_formula)((value >= zero)
				? map( value, zero, 1023 - sens[UP], 0, m_min )
				: map( value, 0 + sens[DOWN], zero, m_max, 0 )
				)
				, MIN, MAX);
		}
		
	}
private:
	short linear(const long& i_value) const
	{
		return i_value;
	}

	short parabola127(const long& i_value) const
	{
		return (short)(float)(i_value * i_value) * ( ( i_value > 0 ) - ( i_value < 0) ) / (float)m_max;
	}

public:
	unsigned short save(unsigned short i_idx = 0)
	{
		unsigned short index = i_idx;

		EEPROM.update(index++, 0b11001100);

		const unsigned char *t = (const unsigned char*)sens;

		for (auto i = 0; i < sizeof(sens); i++)
		{
			EEPROM.update(index++, t[i]);
		}

		EEPROM.update(index++, (unsigned char)zero);
		EEPROM.update(index++, (unsigned char)(zero >> 8));
		EEPROM.update(index++, (unsigned char)m_switched);

		return index;
	}

	unsigned short load(unsigned short i_idx = 0)
	{
		unsigned short index = i_idx;

		if (0b11001100 == EEPROM.read(index++))
		{
			unsigned char *t = (unsigned char*)sens;
			
			for (auto i = 0; i < sizeof(sens); i++)
			{
				t[i] = EEPROM.read(index++);
			}

			zero = EEPROM.read(index++) | EEPROM.read(index++) << 8;

			m_switched = EEPROM.read(index++) & 0x1;
		}

		return index;
	}

	void drawBar( short i_x0, short i_y0, short i_maxW, short i_maxH) const
	{
		short value = this->read();

		bool horizontal = (i_maxW > i_maxH);

		display.setDrawColor(1);
		display.drawFrame(i_x0, i_y0, i_maxW, i_maxH);

		//make a single bit black frame , 4 because of 2 for begin and 2 for end
		i_x0 += 2;
		i_y0 += 2;
		i_maxW -= 4;
		i_maxH -= 4;

		short x_mid = (horizontal) ? (i_x0 + i_maxW / 2) : i_x0;
		short y_mid = (horizontal) ? i_y0 : (i_y0 + i_maxH / 2);


		if (horizontal)
		{
			if (value > 0)
			{
				display.setDrawColor(1);
				display.drawBox(x_mid, y_mid, map(value, 0, MAX , 0, i_maxW / 2), i_maxH);
			}
			else
			{
				display.setDrawColor(1);
				display.drawBox(i_x0, i_y0, i_maxW / 2, i_maxH);
				display.setDrawColor(0);
				display.drawBox(i_x0, i_y0, map(MIN - value, MIN, 0, i_maxW / 2, 0), i_maxH);
			}

			display.setDrawColor(0);
			display.drawVLine(x_mid - 2, y_mid - 2, i_maxH + 2);
		}
		else //vertical
		{
			if (value > 0)
			{
				display.setDrawColor(1);
				display.drawBox(i_x0, i_y0, i_maxW, i_maxH / 2);
				display.setDrawColor(0);
				display.drawBox(i_x0, i_y0, i_maxW, map(MAX - value, 0, MAX, 0, i_maxH / 2));
			}
			else
			{
				display.setDrawColor(1);
				display.drawBox(i_x0, y_mid, i_maxW, map(value, MIN, 0, i_maxH / 2, 0));
			}

			display.setDrawColor(0);
			display.drawHLine(i_x0 - 2, y_mid, i_maxW + 4);
		}
	}

	void drawDetails( const char* i_name , short i_x0, short i_y0, short i_maxW, short i_maxH, bool i_flip = false) const
	{
		short h = i_maxH;
		short w = i_maxW;
		short x = i_x0;
		short y = i_y0;
		short s = 2;

		display.setFont(BIG_FONT);
		display.setFontMode(1);//transparent mode
		display.setDrawColor(2);
		display.setFontPosCenter();
		if ( x - i_maxW  < 0)
		{
			unsigned char nameSize = display.drawStr(x + s, y + h/2 + s, i_name);

			x += nameSize + s;
			w -= nameSize + s;
		}
		else
		{
			unsigned char nameSize = display.drawStr( x + w - display.getStrWidth(i_name) - s, y + h/2 + s, i_name);

			w -= ( nameSize + s ) ;
		}
		
		/*
		 _______
		 | -127 |
		    <>  |
		*/

		static constexpr char* plus = "+";
		static constexpr char* minus = "-";

	    display.drawFrame(x, y, w, h);
		display.setDrawColor(2);
		display.setFontMode(1);//transparent mode
		display.setFont(SMALL_FONT);
		display.setFontPosBaseline();
		display.drawStr( x + w / 2 - 2 * display.getMaxCharWidth(), y + s + display.getMaxCharHeight() , itoa((this->read()), buff_4, 10));
		display.drawStr( x + w / 2, y + s + 2 * display.getMaxCharHeight(), (m_switched) ? plus : minus );
	}

	short trim(unsigned short i_trimValue)
	{
		if (read() > 0)
		{
			sens[UP] = constrain( i_trimValue, 0 , 1023 - zero  );
		}
		else
		{
			sens[DOWN] = constrain( i_trimValue, 0, zero );
		}

		return read();
	}

	short setMaxMin (unsigned short i_value)
	{
		if ( read() > 0 )
		{
			m_max = constrain(i_value, 0, MAX);
		}
		else
		{
			m_min = constrain(-i_value, MIN, 0);
		}

		return read();
	}

	void reset()
	{
		short t_zero = analogRead(m_pin);
		if (0 != zero)
		{
			sens[UP] += (zero - t_zero);
			sens[DOWN] -= (zero - t_zero);
		}
		zero = t_zero;
	}

	void switchDirection(short i_ind)
	{
		m_switched = i_ind % 5;
	}
};

Joystick joysticks[] = { Joystick(J1_PIN) , Joystick(J2_PIN) ,Joystick(J3_PIN) ,Joystick(J4_PIN) };

#define J1 joysticks[0]
#define J2 joysticks[1]
#define J3 joysticks[2]
#define J4 joysticks[3]

//////////////////////////////////////////////////////////////////////////

PIN  SPEED_PIN = J1_V_PIN;
PIN  STEERING_PIN = J2_H_PIN;

//////////////////////////////////////////////////////////////////////////
#define DISPLAY_128_64
#define DISPLAY_I2C_ADDRESS 0x78
#define D_ZERO_X 1
#define D_ZERO_Y 4
#define D_HIGHT 63
#define D_WIDTH 127
#define D_HALF_WIDTH D_WIDTH / 2
#define D_HALF_HIGHT D_HIGHT / 2
/////////////////////////////////////////////////////////////////////////
volatile Button_t *interuptB1 = 0;
volatile Button_t *interuptB2 = 0;

unsigned char CHANNEL = 0x64;

unsigned char address[][6] = { "1Node" };

enum Mode {
	MAIN_SCREEN = 1,
	MENU_SCREEN = 2,
	LAST = 256
};

Mode mode = MAIN_SCREEN;

RF24 radio(CE_PIN, SCN_PIN);

Led activityLed(STS_LED_PIN);

using Screen = void(*)();//pointer to a function responsible for screen representation
Screen showScreen;

void switchMode(Mode i_mode);
void showSaveScreen(void(*i_yes_action)(), void(*i_no_action)());

volatile bool displayRefresh = true;

long map(const long x, const long in_min, const long in_max, const long out_min, const long out_max)
{
	// if input is smaller/bigger than expected return the min/max out ranges value
	if (x < in_min)
		return out_min;
	else if (x > in_max)
		return out_max;

	else if (in_max == in_min)
		return min(abs(out_max), abs(out_min));

	// map the input to the output range.
	// round up if mapping bigger ranges to smaller ranges
	else  if ((in_max - in_min) > (out_max - out_min))
		return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
	// round down if mapping smaller ranges to bigger ranges
	else
		return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void refreshScreen()
{
	displayRefresh = true;
}

Switch_t switch_[] = {
Switch_t(S1_PIN, refreshScreen, refreshScreen),
Switch_t(S2_PIN, refreshScreen, refreshScreen),
Switch_t(S3_PIN, refreshScreen, refreshScreen),
Switch_t(S4_PIN, refreshScreen, refreshScreen)
};

Button_t  main_button(B1_PIN, []() { DISPLAY(display.drawBox(0, 0, D_WIDTH, D_HIGHT)) },
	[]() { switchMode(static_cast<Mode>(static_cast<int>(mode) >> 1)); },
	[]() { switchMode(static_cast<Mode>(static_cast<int>(mode) << 1)); });

volatile InteruptEncoder_t  slctr(ENC_PIN1, ENC_PIN2);

unsigned char scan()
{
	constexpr unsigned char num_channels = 128;
	constexpr unsigned char num_of_scans = 20;

	unsigned char values[num_channels] = { 0 };

	static unsigned char the_channel = 0;

	// static bool  next = true;//because of the button need to put as static
	Button_t button(B1_PIN,
		[]() { showSaveScreen([]() { CHANNEL = the_channel;  switchMode(LAST); }, []() { switchMode(LAST); }); },
		[]() { /*showSaveScreen([]() { CHANNEL = the_channel;  switchMode(LAST); }, []() { switchMode(LAST); });*/ },
		[]() { showSaveScreen([]() { CHANNEL = the_channel; switchMode(LAST); }, []() { switchMode(LAST); }); });

	slctr.begin();

	// Scan all channels num_reps times
	for (unsigned char i = 0; i < num_of_scans && mode != LAST; i++)
	{
		for (unsigned char channel = 0; channel < num_channels && mode != LAST; channel++)
		{
			button.run();
			//encoder1.run(); -it runs by interupt

			// Select this channel
			radio.setChannel(channel);
			// Listen for a little
			radio.startListening();

			the_channel = constrain(slctr.val(), 0, num_channels);
			char* channelStr = itoa(the_channel, buff_4, 10);
			DISPLAY
			(
				for (unsigned char j = 0; j < num_channels; j++)
				{
					if (0 == values[j])
					{
						display.drawPixel(j, D_HIGHT - 2);
					}
					else
					{
						display.drawLine(j, map(values[j], 0, num_of_scans, D_HIGHT - 2, D_HALF_HIGHT), j, D_HIGHT - 2);
					}
				}

			//the_channel = drawVline( constrain(slctr.val(), 0, D_WIDTH ), D_HIGHT - 1 - drawTitle("SEL.CHNL"));
			//Draw vertical line representing selected channel
			[channelStr](unsigned char hight)
			{
				display.setFont(SMALL_FONT/*u8g2_font_5x7_tn */);
				display.setFontPosBaseline();
				display.setDrawColor(1);//white color

				(the_channel > D_HALF_WIDTH) ?
					display.drawStr(the_channel - (display.getMaxCharWidth() + display.getMaxCharWidth() + display.getMaxCharWidth()), D_HIGHT - hight + 2 * display.getMaxCharHeight(), channelStr)
					: display.drawStr(the_channel + 2, D_HIGHT - hight + 2 * display.getMaxCharHeight(), channelStr);
				display.drawVLine(the_channel, D_HIGHT - hight, hight);
			}(D_HIGHT - 1 - drawTitle("SEL.CHNL"));

			)//DISPLAY

			// Did we get a carrier?
			if (radio.testCarrier())
			{
				activityLed.blynk();
				++values[channel];
			}

			radio.stopListening(); //it is transmitter stop listening at the end    
		}
	}
	activityLed.turn_off();
	return the_channel;
}

/*
void drawCoordinates( short x, short y )
{do not remove me 
//    display.setFont(u8g2_font_5x7_tn);
	(y > D_HIGHT * 3/4 ) ? display.drawStr(D_WIDTH - 30 , y - 1, String(y).c_str()) : display.drawStr(D_WIDTH - 30, y + 8, String(y).c_str());
	display.drawHLine(0, y, D_WIDTH - 1);

	( x > D_WIDTH - (6 + 5 + 5) ) ? display.drawStr( x - (6+5+5) , 8, String(x).c_str()) : display.drawStr( x + 2 , 8 , String(x).c_str());
	display.drawVLine( x, 0, D_HIGHT - 1);
}
*/

void drawSwitches()
{
	unsigned char x = D_ZERO_X;
	unsigned char y = D_ZERO_Y;  // Border 2 pixels
	constexpr unsigned char h = 14; // Hight
	constexpr unsigned char w = 20; // Width
	constexpr unsigned char s = 5;  // Space

	for (unsigned char i = 0; i < sizeof(switch_) / sizeof(Switch_t); i++)
	{
		display.setDrawColor(1);//white color
		(switch_[i].getState()) ? display.drawBox(x, y, w, h) : display.drawFrame(x, y, w, h);

		display.setDrawColor(2);
		display.setFontMode(1);//transparent mode
		display.setFont(MEDIUM_FONT);
		display.setFontPosCenter();
		display.drawStr((w - display.getMaxCharWidth()) / 2 + x + 1 /*border size 1 pixel */, y + h - display.getMaxCharHeight() / 2 , itoa((i + 1), buff_4, 10));
		x += (w + s);
	}
}
void drawChannelNumber()
{
	display.setDrawColor(1);//white color
	display.setFont(u8g2_font_7Segments_26x42_mn);
	display.setFontPosBottom();

	unsigned char s = display.getMaxCharWidth() * 3; // > 100

	if (CHANNEL < 10)
	{
		s = display.getMaxCharWidth();
	}
	else if (CHANNEL < 100)
	{
		s = display.getMaxCharWidth() * 2;
	}

	display.drawStr(D_WIDTH - 2 - s, D_HIGHT, itoa(CHANNEL, buff_4, 10));

}
void drawBatteryLevel()
{
	using namespace arduino::utils;

	display.setDrawColor(1);//white color
	display.setFont(MEDIUM_FONT);
	//3 digits 100 %
	short val = payLoadAck.batteryLevel;// map(analogRead(P1_PIN), 0, 1023, 0, 100);

	display.drawStr(D_WIDTH - 2 - display.getMaxCharWidth() - display.getMaxCharWidth() - display.getMaxCharWidth() - 3, display.getMaxCharHeight(), itoa(val, buff_4, 10));
	//  display.drawStr(D_WIDTH - 2 - display.getMaxCharWidth(), display.getAscent() + 3, "%");
}
void drawTelemetry()
{
	using namespace arduino::utils;

	constexpr unsigned char x0 = D_ZERO_X;
	constexpr unsigned char y0 = 20;
	constexpr unsigned char t_w = 44;
	constexpr unsigned char t_h = 44;
	constexpr unsigned char bar_w = t_w / 4;

	display.setDrawColor(1);

	for (char i = 0; i < 4; i++)
	{
		joysticks[i].drawBar(x0 + i * (bar_w + 1), y0, bar_w, t_h);
	}

}
void showMainScreen()
{
	DISPLAY
	(
		//  display.drawFrame(1, 1, D_WIDTH, D_HIGHT);
		drawSwitches();
		drawBatteryLevel();
		drawChannelNumber();
		drawTelemetry();
	)
}

/*
void drawType(const short &x, const short &y, short i_function)
{
	const char* str = "--";

	if (i_function == speed )
		str = "speed";
	else if (i_function == steering )
		str = "steering";

	display.drawStr(x, y, str);
}
*/


void joystickTrim ( void * i_pValue )
{
	Joystick *p_joystick = static_cast<Joystick*>(i_pValue);

	if (p_joystick->read() > 20 || p_joystick->read() < -20)
	{
		p_joystick->trim( slctr.val() * 5);
	}
	else
	{
		slctr.begin();
	}
}

void joystickSwitchDirection(void * i_pValue)
{
	static_cast<Joystick*>(i_pValue)->switchDirection(slctr.val());
}

void joystickSetMaxMin( void *p_joystick)
{

	static_cast<Joystick*>(p_joystick)->setMaxMin( slctr.val() * 5 );
}

void showJoyCfgScreen( const char* i_title , void (*action)(void*) )
{
	const char w = 12;
	const char b = 1;
	const char s = 2;

	Joystick *p_joystick = 0;

	static short joystick = 0;

	Button_t button(B1_PIN, []() { joystick++; },
		[]() { joystick++; },
		[]() { showSaveScreen([]() { J4.save(J3.save(J2.save(J1.save()))); switchMode(LAST); },
		[]() { switchMode(LAST); }); }
	);

	slctr.begin();
	do
	{
		button.run();

		DISPLAY
		(
		unsigned char y = drawTitle(i_title) + 1;// drawTitle("CONF.JOY") + 1;

		unsigned char h = D_HIGHT - (y + s); // y - vertical == 48

		unsigned char y0 = y + s + h / 2;

		unsigned char dash_X = b + w + s;
		unsigned char dash_Y = y + s;
		unsigned char dash_W = D_WIDTH - 2 * w - 2 * s;
		unsigned char dash_H = D_HIGHT - y - w - 2 * s;

		J1.drawBar( b, y + s, w, h );
		J4.drawBar( D_WIDTH - w - b, y + s, w, h );

		J2.drawBar( b + w + s, D_HIGHT - w, h, w);
		J3.drawBar( D_WIDTH - w - s - b - h, D_HIGHT - w, h, w);

		J1.drawDetails("J1", dash_X, dash_Y, dash_W / 2 - s, dash_H / 2);
		J4.drawDetails("J4", dash_X + dash_W/2 , dash_Y, dash_W / 2 - s , dash_H / 2 ,true);

		J2.drawDetails("J2", dash_X, dash_Y + dash_H / 2 + s, dash_W / 2 - s, dash_H / 2 );
		J3.drawDetails("J3", dash_X +  dash_W/2 ,dash_Y + dash_H / 2 + s, dash_W / 2 - s, dash_H / 2 ,true);

		//display.setDrawColor(1);//white color
		display.setDrawColor(2); // white overlapping

		if (0 == joystick % 4)
		{
			if (p_joystick != &J1)
				slctr.begin();

			display.drawBox(dash_X, dash_Y, dash_W / 2 - s, dash_H / 2);
			p_joystick = &J1;

			//  J1.function = ( analogRead(J3.m_pin) > 1000 ) ? selectedFunction++ % 3  : J1.function;
		}

		else if (1 == joystick % 4)
		{
			if (p_joystick != &J2)
				slctr.begin();

			display.drawBox(dash_X, dash_Y + dash_H / 2 + s, dash_W / 2 - s, dash_H / 2 );
			p_joystick = &J2;
		}

		else if (2 == joystick % 4)
		{
			if (p_joystick != &J3)
				slctr.begin();

			display.drawBox(dash_X + dash_W / 2, dash_Y + dash_H / 2 + s, dash_W / 2 , dash_H / 2 );
			p_joystick = &J3;
		}

		else if (3 == joystick % 4)
		{
			if (p_joystick != &J4)
				slctr.begin();

			display.drawBox(dash_X + dash_W / 2, dash_Y, dash_W / 2 , dash_H / 2 );
			p_joystick = &J4;
		}
		)

		action((void*)p_joystick);
		button.run();
	} while (mode != LAST);
}

short drawTitle(const char* i_title)
{
	display.setFont(HEADER_FONT);
	display.setFontMode(0);
	display.setDrawColor(1);
	display.setFontPosBottom();

	display.drawStr((D_WIDTH - display.getStrWidth(i_title)) / 2, display.getMaxCharHeight() + D_ZERO_Y, i_title);
	display.drawHLine(0, D_ZERO_Y + display.getAscent() + 3, D_WIDTH);

	return D_ZERO_Y + display.getAscent() + 3;
}

void showSaveScreen(void(*i_yes_action)(), void(*i_no_action)())
{
	static const char* yes = "YES";
	static const char* no = "NO";

	static bool exit = false;
	static void(*yes_action)();
	static void(*no_action)();

	yes_action = i_yes_action;
	no_action = i_no_action;

	Button_t save_button(B1_PIN, []() {  (slctr.val() % 2) ? yes_action() : no_action(); exit = true; });
	//interuptB1 = &save_button;

	ScopedGuard guard = makeScopedGuard([]() { interuptB1 = 0; exit = false; });

	short x = 0, y = 0, y0 = 0, space = 10;

	do
	{
		DISPLAY
		(
			save_button.run();
		y0 = drawTitle("SAVE");

		display.setFont(BIG_FONT);
		display.setFontMode(0);
		display.setDrawColor(1);
		display.drawStr(D_WIDTH / 2 - display.getStrWidth(yes) - space, (D_HIGHT - y0) / 2 + display.getMaxCharHeight(), yes);
		display.drawStr(D_WIDTH / 2 + space + display.getMaxCharWidth(), (D_HIGHT - y0) / 2 + display.getMaxCharHeight(), no);

		y = (D_HIGHT - y0) / 2 - 2;

		if (slctr.val() % 2)
		{
			x = D_WIDTH / 2 - display.getStrWidth(yes) - space - 3;
			display.drawFrame(x, y, display.getStrWidth(yes) + 6, display.getMaxCharHeight() + 3);
		}
		else
		{
			x = D_WIDTH / 2 + space + display.getMaxCharWidth() - 3;
			display.drawFrame(x, y, display.getStrWidth(no) + 6, display.getMaxCharHeight() + 3);
		}
		)
	} while (!exit);
}

void showMenuScreen()
{
	struct MenuItem
	{
		typedef  void(*Action)();
		Action m_action;
		const char* m_title;

		MenuItem(const char *i_title, Action i_action = []() {}) : m_action(i_action), m_title(i_title) {}
	};

	MenuItem menu[] = {
		MenuItem("SCAN",   []() { scan(); if (radio.getChannel() != CHANNEL) radio.setChannel(CHANNEL); switchMode(MENU_SCREEN); }),
		MenuItem("THRTL",  []() { showJoyCfgScreen( "TRIM" , &joystickTrim ); switchMode(MENU_SCREEN); }),
		MenuItem("DIREC",  []() { showJoyCfgScreen("DIRECT" , &joystickSwitchDirection); switchMode(MENU_SCREEN); }),
		MenuItem("MIN-MAX",[]() { showJoyCfgScreen("MIN-MAX" , &joystickSetMaxMin); switchMode(MENU_SCREEN); }),
		MenuItem("RESET",  []() { DISPLAY(drawTitle(__DATE__); );  showSaveScreen([]() { for (short i = 0; i < EEPROM.length(); i++) { EEPROM.write(i, 0); } } , []() { switchMode(MENU_SCREEN); }); }),
		MenuItem("Back" ,  []() { switchMode(MAIN_SCREEN); })
	};

	unsigned char menu_items_count = sizeof(menu) / sizeof(menu[0]);
	unsigned char selectedMenu = menu_items_count + 1;

	Button_t button(B1_PIN);

	slctr.begin();

	//ScopedGuard guard = makeScopedGuard([]() { slctr.stop(); });

	do
	{
		if (button.run() || selectedMenu != slctr.val() % menu_items_count)
		{
			selectedMenu = slctr.val() % menu_items_count;

			button = Button_t(B1_PIN, menu[selectedMenu].m_action, []() {}, []() { switchMode(MAIN_SCREEN); });

			DISPLAY
			(
				//draw header
				char y = drawTitle("USER-MENU");
				y += 3;
				display.setFont(MEDIUM_FONT/*u8g2_font_8x13_tr*/);
				display.setDrawColor(2);
				display.setFontMode(1);//transparent mode
				for (char i = 0; i < menu_items_count; i++)
				{
					char x = (i % 2) ? D_HALF_WIDTH + 1 : 0;

					if (i != 0 && 0 == (i % 2))
						y += display.getMaxCharHeight();

					if (i == selectedMenu)
					{
						display.drawFrame(x, y, D_HALF_WIDTH - 1, display.getMaxCharHeight());
					}

					display.drawStr(x + (D_HALF_WIDTH - display.getStrWidth(menu[i].m_title)) / 2,
						y + display.getMaxCharHeight(), menu[i].m_title);
				}

			/* drawCoordinates(
				 map(analogRead(STEERING_PIN), 300 , 900, 0, D_WIDTH),
				 map(analogRead(SPEED_PIN), 300, 900, 0, D_HIGHT));

			 selectedMenu = -1;*/
			)

			activityLed.rapid_blynk(100);
		}
	} while (mode == MENU_SCREEN);
}

void switchMode(Mode i_mode = LAST)
{
	LOG_MSG(F("Switch from ") << (short)mode << F(" to ") << (short)i_mode);

	using namespace arduino::utils;

	switch (i_mode)
	{
	case MENU_SCREEN:
		showScreen = showMenuScreen;
		refreshScreen();
		mode = MENU_SCREEN;
		break;
	case MAIN_SCREEN:
		showScreen = showMainScreen;
		mode = MAIN_SCREEN;
		refreshScreen();
		break;
	default:
		mode = i_mode;
		refreshScreen();
	}
}

void setup()
{
	using namespace arduino::utils;

	LOG_MSG_BEGIN(115200);

	display.setI2CAddress(DISPLAY_I2C_ADDRESS);
	display.begin();

	delayMicroseconds(200);
	display.clear();
	radio.begin();              //активировать модуль
	delayMicroseconds(200);
	radio.setAutoAck(0);        //режим подтверждения приёма, 1 вкл 0 выкл
	radio.setRetries(3, 3);    //(время между попыткой достучаться, число попыток)
	radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
	radio.setPayloadSize(sizeof(Payload)/*32*/);     //размер пакета, в байтах

	radio.openWritingPipe((const uint8_t*)address[0]);   //мы - труба 0, открываем канал для передачи данных
	radio.setChannel(CHANNEL);  //выбираем канал (в котором нет шумов!)

	radio.enableDynamicPayloads();

	radio.setPALevel(RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
	radio.setDataRate(RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
	//должна быть одинакова на приёмнике и передатчике!
	//при самой низкой скорости имеем самую высокую чувствительность и дальность!!

	radio.powerUp();
	delayMicroseconds(200);
	radio.stopListening();

	pinMode(J1_V_PIN, INPUT);
	pinMode(J1_H_PIN, INPUT);

	pinMode(J2_V_PIN, INPUT);
	pinMode(J2_H_PIN, INPUT);

	pinMode(S1_PIN, INPUT);
	pinMode(S2_PIN, INPUT);
	pinMode(S3_PIN, INPUT);
	pinMode(S4_PIN, INPUT);
	pinMode(B1_PIN, INPUT);

	pinMode(ENC_PIN1, INPUT_PULLUP);
	pinMode(ENC_PIN2, INPUT_PULLUP);
	/*

	  Пин 	|  Пин порта| Линия PCINT| Группа PCINT
	  ---------------------------------------------
		0	|		0	|	16		 |	2
		1	|		1	|	17		 |	2
		2	|		2	|	18		 |	2
		3	|		3	|	19		 |	2
		4	|		4	|	20		 |	2
		5	|		5	|	21		 |	2
		6	|		6	|	22		 |	2
		7	|		7	|	23		 |	2
		8	|		0	|	0		 |	0
		9	|		1	|	1		 |	0
		10	|		2	|	2		 |	0
		11	|		3	|	3	 	 | 	0
		12	|		4	|	4		 | 	0
		13	|		5	|	5		 | 	0
		A0	|		0	|	8		 |  1
		A1	|		1	|	9		 |	1
		A2	|		2	|	10		 |	1
		A3	|		3	|	11		 |	1
		A4	|		4	|	12		 |	1
		A5	|		5	|	13		 |	1
	*/
	pinMode(STS_LED_PIN, OUTPUT);
	pinMode(SLCR_B_PIN, INPUT);

	PCICR = 0b00000111; // enable interrupt on all 3 types

	PCMSK2 = 0b11110000; // Pin 4,5,6,7 - switches
	PCMSK1 = 0b00000011; // Pin A0 A1 - encoder
	PCMSK0 = 0b00000001; // Pin 8 - main button

	attachInterrupt(digitalPinToInterrupt(ENC_PIN1), []() {	slctr.run(); activityLed.blynk(); }, CHANGE);

	J4.load(J3.load(J2.load(J1.load())));

	J1.reset(); J2.reset(); J3.reset(); J4.reset();

	switchMode(MAIN_SCREEN);
	/*
		LOG_MSG(F("Zero values are D1 V,H ") << J1.zero << F(",") << J2.zero);
		LOG_MSG(F("Zero values are D2 V,H ") << J4.zero << F(",") << J3.zero);*/

	activityLed.fade(1000);
}

void loop()
{
	using namespace arduino::utils;

	static Payload transmit_data;
	static unsigned long lastTransmitionTime = millis();
	using namespace arduino::utils;

	for (unsigned char i = 0; i < sizeof(switch_) / sizeof(Switch_t); i++)
	{
		switch_[i].run();
	}

	main_button.run();

	if (displayRefresh)
	{
		displayRefresh = false;
		showScreen();
	}

	Payload data;

	data.m_b1 = switch_[0].getState();
	data.m_b2 = switch_[1].getState();
	data.m_b3 = switch_[2].getState();
	data.m_b4 = switch_[3].getState();

	data.m_speed = J1.read();
	data.m_steering = J3.read();

	data.m_j[2] = J2.read();
	data.m_j[3] = J4.read();

	if ((data == transmit_data) && lastTransmitionTime > millis() - arduino::utils::RF_TIMEOUT_MS)
	{
		return;//no need to handle ,  nothing changed no timeout occurred
	}

	activityLed.blynk();
	refreshScreen();

	transmit_data = data;

	LOG_MSG(F("Speed: ") << transmit_data.m_speed <<
		F(" Steering: ") << transmit_data.m_steering);

	lastTransmitionTime = millis();

	radio.write(transmit_data.finalize(), sizeof(transmit_data));

	if (radio.isAckPayloadAvailable())
	{
		activityLed.rapid_blynk(200);
		radio.read(&payLoadAck, sizeof(payLoadAck));
	}
	/*
		ack.batteryLevel = data.m_steering;
		ack.speed        = data.m_speed;
		*/
}

ISR(PCINT2_vect) {
	if (!(PIND & (1 << PD0))) {/* Arduino pin 0 interrupt*/ }
	if (!(PIND & (1 << PD1))) {/* Arduino pin 1 interrupt*/ }
	if (!(PIND & (1 << PD2))) {/* Arduino pin 2 interrupt*/ }
	if (!(PIND & (1 << PD3))) {/* Arduino pin 3 interrupt*/ }
	if (!(PIND & (1 << PD4))) {/* Arduino pin 4 interrupt*/ LOG_MSG(F("S0")); }
	if (!(PIND & (1 << PD5))) {/* Arduino pin 5 interrupt*/ LOG_MSG(F("S1")); }
	if (!(PIND & (1 << PD6))) {/* Arduino pin 6 interrupt*/ LOG_MSG(F("S2")); }
	if (!(PIND & (1 << PD7))) {/* Arduino pin 7 interrupt*/ LOG_MSG(F("S3")); }
}

ISR(PCINT1_vect) {
	if (!(PINC & (1 << PC0))) {/* Arduino pin A0 interrupt*/ LOG_MSG(F("A0")); slctr.run(); activityLed.blynk(); }
	if (!(PINC & (1 << PC1))) {/* Arduino pin A1 interrupt*/ LOG_MSG(F("A1")); if (interuptB1) interuptB1->run(); }
	if (!(PINC & (1 << PC2))) {/* Arduino pin A2 interrupt*/ }
	if (!(PINC & (1 << PC3))) {/* Arduino pin A3 interrupt*/ }
	if (!(PINC & (1 << PC4))) {/* Arduino pin A4 interrupt*/ }
	if (!(PINC & (1 << PC5))) {/* Arduino pin A5 interrupt*/ }
}

ISR(PCINT0_vect) {
	if (!(PINB & (1 << PB0))) {/* Arduino pin 8 interrupt*/ LOG_MSG(F("8")); if (interuptB2) interuptB2->run(); }
	if (!(PINB & (1 << PB1))) {/* Arduino pin 9 interrupt*/ }
	if (!(PINB & (1 << PB2))) {/* Arduino pin 10 interrupt*/ }
	if (!(PINB & (1 << PB3))) {/* Arduino pin 12 interrupt*/ }
	if (!(PINB & (1 << PB4))) {/* Arduino pin 11 interrupt*/ }
	if (!(PINB & (1 << PB5))) {/* Arduino pin 13 interrupt*/ }
}


#endif