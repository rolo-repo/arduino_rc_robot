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

#include "RFcom.h"
//++addr &= EEPROM.length() - 1;


#include "SerialOutput.h"
//#include "Constants.h"

#define OLED_RESET 0  // GPIO0
#include "U8g2lib.h"

#include "Button.h"
#include "Switch.h"

#define PIN unsigned int

/*
#ifndef ENABLE_LOGGER
#define LED2_PIN 1
#define B2_PIN 0
#endif
*/

#define STS_LED_PIN A0
#define TUNE_PIN A1

#define ENC_PIN1 2
#define ENC_PIN2 3

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

short  MAX = 127;
short  MIN = -127;


constexpr short speed = 0;
constexpr short steering = 1;
constexpr short not_asighned = 2;

enum class LedAction { ON , OFF , BLYNK };

#define HEADER_FONT u8g2_font_9x15B_tr
#define SMALL_FONT u8g2_font_micro_tn
#define MEDIUM_FONT u8g2_font_6x12_tn
#define BIG_FONT u8g2_font_10x20_mr

struct Joystick
{
    enum Side { UP = 0, DOWN = 1, RIGHT = 0, LEFT = 1, LMAX = 0, LMIN = 1 };
    
    unsigned short zero = 0;
    unsigned short sens[2] = { 0 , 0 };
    short limits[2] = { MAX , MAX } ;
   
    short function;
    bool isSwitched = false;

    PIN m_pin;

    Joystick(PIN i_pin) : m_pin(i_pin) , function( not_asighned ){}

    int read()
    {
       int value = analogRead(m_pin);
       return ( value >= zero )
           ? map(value, zero, 1023 - sens[UP], 0, MAX) 
           : map(value, 0 + sens[DOWN], zero, MIN, 0);
    }
};

Joystick J1 (J1_PIN);
Joystick J2 (J2_PIN);
Joystick J3 (J3_PIN);
Joystick J4 (J4_PIN);

#define  J1_V_0 J1.zero
#define  J1_H_0 J2.zero

#define  J2_V_0 J4.zero
#define  J2_H_0 J3.zero

//////////////////////////////////////////////////////////////////////////

//#define P1_PIN A1  // Battery
//#define P2_PIN A0  // Selector

//const PIN SELECTOR_PIN = P1_PIN;

//////////////////////////////////////////////////////////////////////////
PIN  SPEED_PIN = J1_V_PIN;
PIN  STEERING_PIN = J2_H_PIN;

unsigned short &zeroSpeed = J1_V_0;
unsigned short &zeroSteering = J2_H_0;

unsigned short &sensSpeedUP   = J1.sens[0];
unsigned short &sensSpeedDOWN = J1.sens[1];
unsigned short &sensSteeringL = J3.sens[1];
unsigned short &sensSteeringR = J3.sens[0];
//////////////////////////////////////////////////////////////////////////
#define DISPLAY_128_64
#define DISPLAY_I2C_ADDRESS 0x78
#define D_ZERO_X 1
#define D_ZERO_Y 4
#define D_HIGHT 63
#define D_WIDTH 127
#define D_HALF_WIDTH D_WIDTH / 2
#define D_HALF_HIGHT D_HIGHT / 2

unsigned char CHANNEL = 0x64;

unsigned char address[][6] = { "1Node" };

//U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);
//U8GLIB_SH1106_128X64 display(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);
U8G2_SH1106_128X64_NONAME_2_HW_I2C display(U8G2_R0, /* reset=*/ OLED_RESET);
#define DISPLAY(...) display.firstPage(); do { __VA_ARGS__ ;} while( display.nextPage() );

enum Mode {
    MAIN_SCREEN = 1,   
    MENU_SCREEN = 2,
    LAST = 256
};

Mode mode = MAIN_SCREEN;

RF24 radio(CE_PIN, SCN_PIN);

using CurrentScreen = void (*)();//pointer to a function responsible for screen representation
CurrentScreen showScreen;

void switchMode( Mode i_mode );
void activateStatusLed(LedAction i_action);
void handle_interupt();

volatile bool displayRefresh = true;
void refreshScreen()
{
    displayRefresh = true;
}

Switch_t switch_[] = {
Switch_t( S1_PIN, refreshScreen, refreshScreen),
Switch_t( S2_PIN, refreshScreen, refreshScreen),
Switch_t( S3_PIN, refreshScreen, refreshScreen),
Switch_t( S4_PIN, refreshScreen, refreshScreen)
};


Button_t button1(B1_PIN, []() { DISPLAY(display.drawBox(0, 0, D_WIDTH, D_HIGHT)) }, []() { switchMode(static_cast<Mode>(static_cast<int>(mode) >> 1)); }, []() {  switchMode(static_cast<Mode>(static_cast<int>(mode) << 1)); });

class InteruptEncoder
{
public:
	InteruptEncoder( PIN i_p1 , PIN i_p2 ):m_p1 (i_p1), m_p2(i_p2){}
	
	void begin() volatile
	{
		attachInterrupt(digitalPinToInterrupt(m_p1), handle_interupt, CHANGE);
		attachInterrupt(digitalPinToInterrupt(m_p2), handle_interupt, CHANGE);
		m_val = 0;
	}

	void stop() volatile
	{
		detachInterrupt(digitalPinToInterrupt(m_p1));
		detachInterrupt(digitalPinToInterrupt(m_p2));
	}

	void run() volatile
	{
		int msb = digitalRead(m_p1);
		int lsb = digitalRead(m_p2);

		unsigned char enc_value = (msb << 1 | lsb) | m_prevVal;

		if (enc_value == 0b1101 || enc_value == 0b0100 || enc_value == 0b0010 || enc_value == 0b1011)
			m_val++;
		else if (m_val > 0)
			m_val--;

		m_prevVal = (msb << 1 | lsb) << 2;
	}

	unsigned short val() volatile const
	{
		return m_val;
	}


private:
	
	PIN m_p1;
	PIN m_p2;
	unsigned short m_val = 0;
	unsigned char m_prevVal = 0;
};

volatile InteruptEncoder  encoder1( ENC_PIN1, ENC_PIN2 );

void handle_interupt()
{
	encoder1.run();
	activateStatusLed(LedAction::BLYNK);
	refreshScreen();
}

long map(const long x, const long in_min, const long in_max, const long out_min,const long out_max)
{
    // if input is smaller/bigger than expected return the min/max out ranges value
    if ( x < in_min )
        return out_min;
    else if ( x > in_max )
        return out_max;

    else if ( in_max == in_min )
        return min ( abs(in_max) , abs(in_min) );

    // map the input to the output range.
    // round up if mapping bigger ranges to smaller ranges
    else  if ((in_max - in_min) > (out_max - out_min))
        return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
    // round down if mapping smaller ranges to bigger ranges
    else
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

short drawVline( const short &x , const short &hight = D_HIGHT )
{
    display.setFont(SMALL_FONT/*u8g2_font_5x7_tn */);
    display.setDrawColor(1);//white color

    ( x > D_HALF_WIDTH ) ? 
        display.drawStr( x - ( display.getMaxCharWidth() + display.getMaxCharWidth() + display.getMaxCharWidth() ), D_HIGHT - hight + 2 * display.getMaxCharHeight(), String(x).c_str())
        : display.drawStr( x + 2, D_HIGHT - hight + 2 * display.getMaxCharHeight() , String(x).c_str());
    display.drawVLine( x , D_HIGHT - hight , hight );

    return x;
}

void activateStatusLed(LedAction i_action = LedAction::BLYNK )
{
    static unsigned char LED_STS = HIGH;
    switch (i_action)
    {
	case LedAction::OFF:
        digitalWrite( STS_LED_PIN, ( LED_STS = LOW ) );
        break;
	case LedAction::ON:
		digitalWrite( STS_LED_PIN, ( LED_STS = HIGH ) );
        break;
	case LedAction::BLYNK:
		digitalWrite( STS_LED_PIN, ( LED_STS = ( LED_STS == HIGH ) ? LOW : HIGH ) );
        break;
    }  
}

unsigned char scan()
{ 
    const unsigned char num_channels = 128;
    const unsigned char num_of_scans = 20;

	unsigned char values[num_channels] = { 0 };

    unsigned char the_channel = 0;

   // static bool  next = true;//because of the button need to put as static
    Button_t button(B1_PIN, []() { /*next = false;*/switchMode(LAST); });
	encoder1.begin();
    
	// Scan all channels num_reps times
    for ( unsigned char i = 0 ; i < num_of_scans && mode != LAST; i++ )
    {
        for ( unsigned char channel = 0 ; channel < num_channels  && mode != LAST; channel++ )
        {
            button.run();
			//encoder1.run(); -it runs by interupt
 
            // Select this channel
            radio.setChannel( channel );
            // Listen for a little
            radio.startListening();
           
            DISPLAY
            (
                /*display.setFont(u8g2_font_micro_tn);
                display.setFontMode(1);
                display.setDrawColor(1);//white color

                //draw count down
                display.drawStr( 1 , display.getMaxCharHeight() , String( num_of_scans - i ).c_str());
                */
                for ( unsigned char j = 0; j < num_channels; j++ )
                {
                    if (0 == values[j])
                    {
                        display.drawPixel( j, D_HIGHT - 2 );
                    }
                    else
                    {
                        display.drawLine( j , map( values[j], 0, num_of_scans , D_HIGHT - 2 , D_HALF_HIGHT ), j , D_HIGHT - 2 );
                    }
                }

			    the_channel = drawVline( map( encoder1.val() , 0, 1023, 0, D_WIDTH ) , D_HIGHT - 1 - drawTitle( "SEL.CHNL" )  );
            )

            // Did we get a carrier?
            if ( radio.testCarrier() ) 
            {
                activateStatusLed();
                ++values[channel];
            }

            radio.stopListening(); //it is transmitter stop listening at the end    
        }
    }

	encoder1.stop();
    activateStatusLed(LedAction::OFF);
    return the_channel;
}

/*
void drawCoordinates( short x, short y )
{   
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
    unsigned char h = 14; // Hight
    unsigned char w = 20; // Width
    unsigned char s = 5;  // Space
  
    for ( unsigned char i = 0; i < sizeof(switch_) / sizeof(Switch_t); i++ )
    {
        display.setDrawColor(1);//white color
        ( switch_[i].getState()) ? display.drawBox( x, y, w , h ) : display.drawFrame( x, y , w, h ) ;
      
        display.setDrawColor(2);
        display.setFontMode(1);//transparent mode
        display.setFont( MEDIUM_FONT );
        display.setFontPosCenter();
        display.drawStr( ( w - display.getMaxCharWidth() ) / 2  + x + 1 /*border size 1 pixel */ , y +  h >> 1  , String( i + 1 ).c_str() );
        x += (w + s);
    }
}
void drawChannelNumber()
{
    display.setDrawColor(1);//white color
    display.setFont(u8g2_font_7Segments_26x42_mn);
    display.setFontPosBottom();

    String channel = String(CHANNEL);
    unsigned char s = display.getMaxCharWidth() * 3 ; // > 100
    
    if ( CHANNEL < 10 )
    {
        s = display.getMaxCharWidth();
    }
    else if ( CHANNEL < 100 )
    {
        s = display.getMaxCharWidth() * 2;
    }

    display.drawStr( D_WIDTH - 2 - s , D_HIGHT - 2, channel.c_str() );
           
}
void drawBatteryLevel()
{
	using namespace arduino::utils;

    display.setDrawColor(1);//white color
    display.setFont( MEDIUM_FONT );
    //3 digits 100 %
	short val = payLoadAck.batteryLevel;// map(analogRead(P1_PIN), 0, 1023, 0, 100);

    display.drawStr(D_WIDTH - 2 - display.getMaxCharWidth() - display.getMaxCharWidth() - display.getMaxCharWidth() - 3, display.getMaxCharHeight(), String(val).c_str());
  //  display.drawStr(D_WIDTH - 2 - display.getMaxCharWidth(), display.getAscent() + 3, "%");
}
void drawTelemetry()
{
    using namespace arduino::utils;

    unsigned char x0 = 2;
    unsigned char y0 = 20;

    display.setDrawColor(2);
    display.setFontMode(1);//transparent mode
    display.setFont(MEDIUM_FONT);

    display.drawBox( x0 , y0 , 47 - 2 /* line sizes */, 42 );//free space

    display.drawStr( x0 + ( 47 - 2 ) / 2 , y0 + 42 / 2 + display.getMaxCharHeight(), String(payLoadAck.batteryLevel).c_str());
    display.drawStr( x0 + (47 - 2), y0 + 42 / 2 + 2* display.getMaxCharHeight(), String(payLoadAck.speed).c_str());
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

short trimJ_PLUS (const unsigned short &zero, const int &j_val, unsigned short &sens, short barSize )
{
    unsigned short s_val = constrain( encoder1.val(), 0, 1023 );
    sens = constrain( map( s_val, 0, 1023, 1023 - zero, 0), 0, 1023 - zero );
    return constrain( map( j_val, zero, 1023 - sens, 0, barSize), 0, barSize);
}

/*
short trimJ_H_Right(const unsigned short &zero, const int &j_val, unsigned short &sens, short barSize)
{
    unsigned short s_val = constrain( analogRead( SELECTOR_PIN ), 0, 1023 );
    sens = constrain( map( s_val , 0, 1023, 0, 1023 - zero), 0, 1023 - zero);
    return constrain( map( j_val, zero, 1023 - sens, 0, barSize), 0, barSize);
}
*/

short trimJ_MINUS (const unsigned short &zero, const int &j_val, unsigned short &sens, short barSize )
{
    unsigned short s_val = constrain( encoder1.val(), 0, 1023 );
    sens = constrain( map( s_val, 0, 1023, 0, zero ), 0, zero );
    return constrain( map( j_val, 0 + sens, zero, barSize, 0 ), 0, barSize );
}

/*
short trimJ_H_Left (const unsigned short &zero, const int &j_val, unsigned short &sens, short barSize)
{
    unsigned short s_val = constrain(analogRead(SELECTOR_PIN), 0, 1023);
    sens = constrain( map( s_val, 0, 1023, 0, zero ), 0, zero);// 0 - 100%
    return constrain( map( j_val, 0 + sens, zero, barSize, 0 ), 0, barSize );
}*/

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
void showSys()
{
    const char w = 12;
    const char b = 1;
    const char s = 2;
    
    static short joystick = 0;
    Button_t button(B1_PIN, []() { joystick++;}, []() { switchMode(LAST); } , []() { switchMode(LAST); });
	encoder1.begin();
    do
    {
        button.run();

        PIN J_PIN = J1_V_PIN;

        switch ( joystick % 4 )
        {
        case 1:
            J_PIN = J1_H_PIN;
            break;
        case 2:
            J_PIN = J2_H_PIN;
            break;
        case 3:
            J_PIN = J2_V_PIN;
            break;
        }

        short j_val = analogRead(J_PIN);

        DISPLAY
        (
            display.setDrawColor(1);//white color

            unsigned char y = drawTitle("CONF.JOY") + 1;

            unsigned char h = D_HIGHT - ( y + s ); // y - vertical == 48

            unsigned char y0 = y + s + h / 2;

            display.drawFrame( b, y + s , w , h );//j1
            display.drawFrame( D_WIDTH - w - b, y + s , w , h );//j4
            
            display.drawHLine( b - 1, y0, w + 2 );//zero line
            display.drawHLine( D_WIDTH - w - b - 1, y0, w + 2 );//zero line

            unsigned char x0_1       = b + w + s + h / 2;
            unsigned char x0_2       = D_WIDTH - w - s - b - h / 2;

            display.drawFrame( b + w + s, D_HIGHT - w , h , w );//j2
            display.drawFrame( D_WIDTH - w - s - b - h , D_HIGHT - w , h , w );//j3

            display.drawVLine( x0_1 , D_HIGHT - w - 1 , w );//zero line
            display.drawVLine( x0_2 , D_HIGHT - w - 1 , w );//zero line
         
            unsigned char dash_X = b + w + s;
            unsigned char dash_Y = y + s;
            unsigned char dash_W = D_WIDTH - 2 * w - 2 * s;
            unsigned char dash_H = D_HIGHT - y - w - 2 * s;
           
            if ( 0 == joystick % 4 )
            {
                display.drawBox( dash_X, dash_Y , dash_W / 2 - s , dash_H / 2);
  
                if ( j_val >= J1.zero + 30 )
                {
                    unsigned char y_t = trimJ_PLUS( J1.zero , j_val , J1.sens[Joystick::UP], h/2 );
                    display.drawBox( b, y0 - y_t, w, y_t );

                    J1.limits[Joystick::LMAX] = max( J1.limits[Joystick::LMAX], j_val );      
                }
                else if (j_val < J1.zero - 30)
                {     
                    unsigned char y_t = trimJ_MINUS( J1.zero, j_val, J1.sens[Joystick::DOWN], h/2 );
                    display.drawBox( b, y0, w, y_t );

                    J1.limits[Joystick::LMIN] = min( J1.limits[Joystick::LMIN], j_val );   
                }

               //  J1.function = ( analogRead(J3.m_pin) > 1000 ) ? selectedFunction++ % 3  : J1.function;
            }

            if ( 1 == joystick % 4 )
            {
                display.drawBox( dash_X, dash_Y + dash_H / 2 , dash_W /2 - s, dash_H/2 );
               
                if ( j_val >= J2.zero + 30 )
                {
                    unsigned char x_t  = trimJ_PLUS( J2.zero, j_val, J2.sens[Joystick::RIGHT], h / 2 );
                    display.drawBox( x0_1, D_HIGHT - w, x_t, w );

                    J2.limits[Joystick::LMAX] = max( J2.limits[Joystick::LMAX], j_val );            
                }
                else if (j_val < J2.zero - 30)
                {
                    unsigned char x_t = trimJ_MINUS( J2.zero , j_val, J2.sens[Joystick::LEFT], h / 2 );
                    display.drawBox( x0_1 - x_t, D_HIGHT - w, x_t , w );

                    J2.limits[Joystick::LMIN] = min( J2.limits[Joystick::LMIN], j_val ); 
                }

                // J2.function = (analogRead(J3.m_pin) > 1000 ) ? selectedFunction++ % 3 : J2.function;
            }

            if ( 2 == joystick % 4 )
            {
                display.drawBox( dash_X + dash_W/2 , dash_Y + dash_H / 2, dash_W / 2 - s, dash_H / 2 );

                if ( j_val >= J3.zero + 30 )
                {
                    unsigned char x_t = trimJ_PLUS( J3.zero, j_val, J3.sens[Joystick::RIGHT], h / 2);
                    display.drawBox( x0_2, D_HIGHT - w, x_t, w);

                    J3.limits[Joystick::LMAX] = max( J3.limits[Joystick::LMAX], j_val );
                }
                else if (j_val < J3.zero - 30)
                {
                    unsigned char x_t = trimJ_MINUS( J3.zero , j_val, J3.sens[Joystick::LEFT], h / 2);
                    display.drawBox( x0_2 - x_t, D_HIGHT - w, x_t, w);

                    J3.limits[Joystick::LMIN] =  min(J3.limits[Joystick::LMIN], j_val);
                }

              //   J3.function = ( analogRead(J3.m_pin) > 1000 ) ? selectedFunction++ % 3 : J3.function;
            }

            if ( 3 == joystick % 4 )
            {
                display.drawBox( dash_X + dash_W / 2, dash_Y, dash_W / 2 - s, dash_H / 2);

                if ( j_val >= J4.zero + 30)
                {           
                    unsigned char y_t = trimJ_PLUS( J4.zero, j_val , J4.sens[Joystick::UP], h / 2 );
                    display.drawBox( D_WIDTH - b - w, y0 - y_t, w, y_t );

                    J4.limits[Joystick::LMAX] = max( J4.limits[Joystick::LMAX], j_val );
                }
                else if ( j_val - J4.zero - 30 )
                {
                    unsigned char y_t = trimJ_MINUS( J4.zero, j_val, J4.sens[Joystick::DOWN], h / 2);
                    display.drawBox( D_WIDTH - b - w, y0, w, y_t );

                    J4.limits[Joystick::LMIN] = min( J4.limits[Joystick::LMIN], j_val );
                }

              //  J4.function = ( analogRead(J3.m_pin) > 1000 ) ? selectedFunction++ % 3  : J4.function;
            }
        
            button.run();

            display.setFont(BIG_FONT);
            display.setFontMode(1);//transparent mode
            display.setDrawColor(2);
            
            display.drawStr( dash_X + 1, dash_Y + display.getMaxCharHeight() , "J1" );
            display.drawStr( dash_X + 1, dash_Y + dash_H / 2 + display.getMaxCharHeight(), "J2" );

            display.drawStr( dash_X + dash_W - 2* display.getMaxCharWidth() - s - 1,  dash_Y + dash_H / 2 + display.getMaxCharHeight(), "J3" );
            display.drawStr( dash_X + dash_W - 2* display.getMaxCharWidth() - s - 1,  dash_Y + display.getMaxCharHeight(), "J4" );

            unsigned char strSize = display.getStrWidth("XX");
            display.setFont(SMALL_FONT);

            display.drawStr( dash_X + strSize + s , dash_Y + 2 * display.getMaxCharHeight() - 2, String(  map( J1.limits[Joystick::LMAX], J1.zero, 1023 - J1.sens[Joystick::UP], 0, MAX)).c_str());
            display.drawStr( dash_X + strSize + s , dash_Y + 3 * display.getMaxCharHeight() , String(-(map( J1.limits[Joystick::LMIN], J1.sens[Joystick::DOWN], J1.zero, MIN, 0))).c_str());

            display.drawStr( dash_X + strSize + s, dash_Y + 2 * display.getMaxCharHeight() - 2 + dash_H / 2 , String(map(J2.limits[Joystick::LMAX], J2.zero, 1023 - J2.sens[Joystick::UP], 0, MAX)).c_str());
            display.drawStr( dash_X + strSize + s, dash_Y + 3 * display.getMaxCharHeight() + dash_H / 2 , String(-(map(J2.limits[Joystick::LMIN], J2.sens[Joystick::DOWN], J2.zero, MIN, 0))).c_str());

            display.drawStr( dash_X - strSize + dash_W - 5 * display.getMaxCharWidth(), dash_Y + 2 * display.getMaxCharHeight() - 2 + dash_H / 2, String( map(J3.limits[Joystick::LMAX], J3.zero, 1023 - J3.sens[Joystick::UP], 0, MAX)).c_str());
            display.drawStr( dash_X - strSize + dash_W - 5 * display.getMaxCharWidth(), dash_Y + 3 * display.getMaxCharHeight() + dash_H / 2, String( -( map(J3.limits[Joystick::LMIN], J3.sens[Joystick::DOWN], J3.zero, MIN, 0))).c_str());

            display.drawStr( dash_X - strSize + dash_W - 5 * display.getMaxCharWidth(), dash_Y + 2 * display.getMaxCharHeight() - 2, String( map(J4.limits[Joystick::LMAX], J4.zero, 1023 - J4.sens[Joystick::UP], 0, MAX)).c_str());
            display.drawStr( dash_X - strSize + dash_W - 5 * display.getMaxCharWidth(), dash_Y + 3 * display.getMaxCharHeight() , String(-( map(J4.limits[Joystick::LMIN], J4.sens[Joystick::DOWN], J4.zero, MIN, 0))).c_str());

         /*
            drawType(dash_X, dash_Y + dash_H / 2 - s  , J1.function );
            drawType(dash_X, dash_Y + dash_H  - s , J2.function);
            drawType(dash_X + dash_W / 2 + s , dash_Y + dash_H - s , J3.function);
            drawType(dash_X + dash_W / 2, dash_Y + dash_H / 2 - s , J4.function);
         */   
        )
    } while ( mode != LAST );

	encoder1.stop();

}

short drawTitle ( const char* i_title ) 
{
   display.setFont(HEADER_FONT);
   display.setFontMode(0);
   display.setDrawColor(1);
   display.drawStr((D_WIDTH - display.getStrWidth(i_title)) / 2 , display.getMaxCharHeight() + D_ZERO_X, i_title);
   display.drawHLine(0, display.getAscent() + 3 , D_WIDTH);

   return display.getAscent() + 3;
}

void showMenuScreen()
{
    struct MenuItem
    {
        typedef  void(*Action)();
        Action m_action;
        const char* m_title;

        MenuItem(const char *i_title, Action i_action = []() {}) : m_action(i_action), m_title(i_title) {}

        /*
        void options( const char* i_opt... )
        {
            va_list args;
            va_start(args, i_opt);
            short i = 0;
            while ( i_opt != '\0' )
            {
                m_options[++i &= sizeof(m_options) / sizeof(String) - 1] = va_arg(args, const char*);
            }
            va_end(args);
        }
        */
    };

    MenuItem menu[] = { 
        MenuItem("SCAN", []() {  CHANNEL = scan(); radio.setChannel(CHANNEL); switchMode(MENU_SCREEN); }),
        MenuItem("THRTL", []() { showSys(); switchMode(MENU_SCREEN);} ),
        MenuItem("MODEL"),
        MenuItem("Back" ,  []() { switchMode(MAIN_SCREEN); }) 
                       };

    short menu_items_count = sizeof (menu) / sizeof( menu[0] );
    short selectedMenu = menu_items_count + 1;

    Button_t button( B1_PIN );
	encoder1.begin();
	do
    { 
        short j = encoder1.val() % menu_items_count;
        
        if ( button.run() || selectedMenu != j )
        {  
            selectedMenu = j;
            
            button = Button_t( B1_PIN, menu[selectedMenu].m_action, []() {}, []() { switchMode(MAIN_SCREEN); });
            
            DISPLAY
            (
               //draw header
               char y = drawTitle("USER-MENU");
               y += 3;
               display.setFont(BIG_FONT/*u8g2_font_8x13_tr*/);
               display.setDrawColor(2);
               display.setFontMode(1);//transparent mode
               for ( char i = 0; i < menu_items_count; i++ )
               {    
                   char x = ( i  % 2 ) ? D_HALF_WIDTH + 1  : 0 ;
                   
                   if ( i != 0  && 0 == ( i % 2 ) ) 
                       y += display.getMaxCharHeight();
                   
                   if ( i == selectedMenu )
                   {
                       display.drawFrame( x , y , D_HALF_WIDTH - 1 , display.getMaxCharHeight());
                   }

                   display.drawStr( x + ( D_HALF_WIDTH - display.getStrWidth(menu[i].m_title) ) / 2, 
                       y + display.getMaxCharHeight() , menu[i].m_title );
               }

              /* drawCoordinates(
                   map(analogRead(STEERING_PIN), 300 , 900, 0, D_WIDTH),
                   map(analogRead(SPEED_PIN), 300, 900, 0, D_HIGHT));

               selectedMenu = -1;*/
            )
        }
   } while ( mode == MENU_SCREEN );

    encoder1.stop();

}

void switchMode( Mode i_mode = LAST ) 
{
    LOG_MSG(F( "Switch from " ) << (short)mode << F(" to ") << (short)i_mode);

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
    
    LOG_MSG_BEGIN(9600);

    display.setI2CAddress(DISPLAY_I2C_ADDRESS);
    display.begin();
   
    delayMicroseconds(200);
    display.clear();


    radio.begin();              //активировать модуль
    delayMicroseconds(200);
    radio.setAutoAck(0);        //режим подтверждения приёма, 1 вкл 0 выкл
    radio.setRetries( 3, 3 );    //(время между попыткой достучаться, число попыток)
    radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
    radio.setPayloadSize(sizeof(Payload)/*32*/);     //размер пакета, в байтах

    radio.openWritingPipe((const uint8_t*)address[0]);   //мы - труба 0, открываем канал для передачи данных
    radio.setChannel(CHANNEL);  //выбираем канал (в котором нет шумов!)

    radio.setPALevel(RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
    radio.setDataRate(RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    //должна быть одинакова на приёмнике и передатчике!
    //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

    radio.powerUp();
    delayMicroseconds(200);
    radio.stopListening();
/*
#ifndef ENABLE_LOGGER
    pinMode(LED2_PIN, OUTPUT);
    pinMode(B2_PIN, INPUT);
#endif
*/
    pinMode(J1_V_PIN, INPUT);
    pinMode(J1_H_PIN, INPUT);

    pinMode(J2_V_PIN, INPUT);
    pinMode(J2_H_PIN, INPUT);

    pinMode(S1_PIN, INPUT);
    pinMode(S2_PIN, INPUT);
    pinMode(S3_PIN, INPUT);
    pinMode(S4_PIN, INPUT);
    pinMode(B1_PIN, INPUT);

    pinMode(ENC_PIN1, INPUT);
    pinMode(ENC_PIN2, INPUT);

    pinMode(STS_LED_PIN, OUTPUT);
    pinMode(TUNE_PIN, OUTPUT);

   // digitalWrite(LED1_PIN, HIGH);

	activateStatusLed(LedAction::ON);

    J1.zero = analogRead(J1.m_pin);
    J2.zero = analogRead(J2.m_pin);

    J3.zero = analogRead(J3.m_pin);
    J4.zero = analogRead(J4.m_pin);

    switchMode(MAIN_SCREEN);

    LOG_MSG(F("Zero values are D1 V,H ") << J1.zero << F(",") << J2.zero);
    LOG_MSG(F("Zero values are D2 V,H ") << J4.zero << F(",") << J3.zero);
}

void loop() 
{ 
    using namespace arduino::utils;

    static Payload transmit_data;
    static unsigned long lastTransmitionTime = millis();
    using namespace arduino::utils;
    
    for ( unsigned char i = 0; i < sizeof(switch_) / sizeof(Switch_t); i++ )
    {
        switch_[i].run();
    }

    button1.run();

    if ( displayRefresh )
    {
        displayRefresh = false;
        showScreen();
    }

    Payload data;

    data.m_b1 = digitalRead(S1_PIN);
    data.m_b2 = digitalRead(S2_PIN);
    data.m_b3 = digitalRead(S3_PIN);
    data.m_b4 = digitalRead(S4_PIN);
    
    data.m_speed = J1.read();
    data.m_steering = J3.read();
    
	data.m_j[2] = J2.read();
	data.m_j[3] = J4.read();

    /*
    data.m_speed = ( analogRead( SPEED_PIN ) >= zeroSpeed ) ?
        map( analogRead(SPEED_PIN), zeroSpeed, 1023 - sensSpeedUP , 0, MAX ) :
        map( analogRead(SPEED_PIN), 0 + sensSpeedDOWN , zeroSpeed, MIN, 0 );

    data.m_steering = ( analogRead( STEERING_PIN ) >= zeroSteering ) ?
        map(analogRead(STEERING_PIN), zeroSteering, 1023 - sensSteeringR , 0, MAX ) :
        map(analogRead(STEERING_PIN), 0 + sensSteeringL , zeroSteering, MIN, 0 );
        */
    if ((data == transmit_data) && lastTransmitionTime > millis() - arduino::utils::RF_TIMEOUT_MS)
    {
        return;//no need to handle ,  nothing changed no timeout occurred
    }

    
    activateStatusLed();
    refreshScreen();

    transmit_data = data;

    LOG_MSG(F("Speed: ") << transmit_data.m_speed << 
            F(" Steering: ") << transmit_data.m_steering << 
            F(" ") << sensSteeringR << 
            F(" ") << sensSteeringL << 
            F(" ") << sensSpeedUP << 
            F(" ") << sensSpeedDOWN);

    lastTransmitionTime = millis();
 
    radio.write( transmit_data.finalize() , sizeof(transmit_data) );

    if ( radio.isAckPayloadAvailable() )
    {
        radio.read( &payLoadAck, sizeof(payLoadAck) );
    }
/*
    ack.batteryLevel = data.m_steering;
    ack.speed        = data.m_speed;
	*/
}
#endif