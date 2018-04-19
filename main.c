
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include "d_led.h"		// Do³¹czenie biblioteki wyœwietlacza led.

#define KEY_12H_24H 	(1<<PD2) 	// Pin dla klawisza odpowiedzialnego za prze³¹czenie trybu godzinowego zegara 12/24H.
#define BUZZER 			(1<<PD3) 	// Pin dla brzêczka sygnalizuj¹cego.
#define SET_TIME 		(1<<PD4) 	// Pin dla klawisza odpowiedzialnego za uruchamianie trybu ustawiania godziny.
#define INCREASE_VALUE 	(1<<PD5) 	// Pin dla klawisza odpowiedzialnego za zwiêkszanie godziny/minut.
#define DECREASE_VALUE 	(1<<PD6) 	// Pin dla klawisza odpowiedzialnego za zmniejszanie godziny/minut.
#define SET_MM_HH 		(1<<PD7) 	// Pin dla klawisza odpowiedzialnego za prze³¹czenie miêdzy ustawianiem godzin lub minut.
#define SIGNAL_LED		(1<<PC7) 	// Pin dla diody sygnalizuj¹cej wciœniêcie przycisku.

uint8_t key_down (uint8_t KEY);		// Funkcja obs³uguj¹ca naciskanie klawiszy.
void set_time_function (void);		// Funkcja ustawiania godziny.
void time_form (void);				// Funkcja ustawiania formatu godziny.
void blink_digits (void);			// Funkcja odpowiedzialna za mruganie  ca³ego wyœwietlacza.
void blink_hh_digits (void);		// Funkcja odpowiedzialna za mruganie  godzniy.
void blink_mm_digits (void);		// Funkcja odpowiedzialna za mruganie  minut.
bool interval (uint16_t millis);	// Funkcja wystawiaj¹ca flagê co zadany w ms interwa³.

uint8_t D,  mj, md, gj ,gd ; 	 	// m - minuty, g - godziny, j- cyfra jednoœci, d - cyfra dziesi¹tek
uint8_t b=0;					 	// Zmienne prze³¹czaj¹ca stany.
static uint8_t s=0;
volatile static uint16_t i=0;		// Zmienna zegarowa
volatile static uint32_t _interval=0;
uint8_t hour_quantity;
uint16_t blink_ms=200; 				// Jak czêsto  mrugac?


int main(void)
{
	// Inicjalizacja pinów

	// Klawisze
	DDRD &=~(KEY_12H_24H | SET_TIME | INCREASE_VALUE | DECREASE_VALUE | SET_MM_HH); // Piny jako wejœcia.
	PORTD|=  KEY_12H_24H | SET_TIME | INCREASE_VALUE | DECREASE_VALUE | SET_MM_HH; 	// Wymuszenie stanu wysokiego.

	// Sygnalizacja
	DDRC |= SIGNAL_LED;	// Pin jako wyjœcie.
	PORTC|= SIGNAL_LED;	// Stan wysoki, LED wy³¹czony.
	DDRD|=BUZZER; 		// Pin jako wyjœcie.
	PORTD&=~BUZZER;		// Stan wysoki, brzêczek wy³¹czony.

	d_led_init();  		// Inicjalizacja wyœwietlacza multipleksowanego.
	sei();				// W³¹czenie globalnego zezwolenia na przerwania.

	hour_quantity=24;	// Ustawienia pocz¹tkowe formatu godzinowego. Wartoœc pocz¹tkowa zegara, godzina wyœwietlana po w³¹czeniu zasilania.
	D=12;
	md=0;
	mj=0;

	// Ustawienia dla Timer2
	TCCR2|= (1<<WGM21); // Tryb CTC.
	TCCR2|= (1<<CS22);  // Preskaler 64.
	OCR2=124; 			// Przerwanie generowane co 1ms. (8000000/64/125=1000Hz)
	TIMSK |= (1<<OCIE2);// Zezwolenie na przerwanie od Compare Match.

		while(1)
		{
			if(key_down(SET_TIME)){ b++;}
			if (b>1)b=0;

			switch (b)
			{
			case 0:			cy4=mj;					// Tryb  pracy.
							cy3=md;
							cy2=gj;
							cy1=gd;
							time_form ();			// Funkcja formatu godziny 12h/24h.

							if (i>=1000)			// Naliczanie minut, godzin.
							{
							mj++;
							if (mj>9)mj=0;			// Licznik minut.
							if (mj==0) md++;
							if (md>5) md=0;
							if (md==0&&mj==0)D++;	// Licznik  godziny.
							if (D>24)D=1;
							i=0;
							}
			break;

			case 1:			set_time_function(); 	// Tryb ustawiania godziny
							time_form ();

			break;
			}
		}
}
void set_time_function (void)
{
	if(key_down(SET_TIME)){ b++;}
	if(key_down(SET_MM_HH)){s++;}
	if(s>2)s=0;
	blink_digits();

	while (s==1)// Ustawianie godziny.
	{
		time_form ();
		blink_hh_digits();

		if (key_down(INCREASE_VALUE)){D++;if (D>24)D=1;}
		if (key_down(DECREASE_VALUE)){D--;if (D<1)D=24;}
		if (key_down(SET_MM_HH))	{s++;}
	}

	while (s==2)// Ustawianie minut.
	{
		time_form ();
		blink_mm_digits();

		if (key_down(INCREASE_VALUE))
					{mj++;
					if (mj>9)mj=0;	//licznik minut
					if (mj==0) md++;
					if (md>5) md=0;
					}

		if (mj==0&&key_down(DECREASE_VALUE)){mj=9;md--;}
		else if (key_down(DECREASE_VALUE))mj--;
		if (md>9)md=5;

		if (key_down(SET_MM_HH)){s++;}

	}
}
void time_form (void)
{

static int e;
int8_t d=0;
if (key_down(KEY_12H_24H)){e++;d++;}	// Zmiana trybu godzinowego 12/24H.

if(e==1)hour_quantity=24;
if(e==2){hour_quantity=12;}

if(hour_quantity==24&&d==1)				// Wyœwietlanie info o trybie godzinowym. 24H
	{
	cy1=2;
	cy2=4;
	cy3=11;
	cy4=NIC;
	while (!interval(1500));
	};

if(hour_quantity==12&&d==1)				// Wyœwietlanie info o trybie godzinowym. 12H
	{
	cy1=1;
	cy2=2;
	cy3=11;
	cy4=NIC;
	while (!interval(1500));
	e=0;
	};

				if ((hour_quantity==12)||(hour_quantity==24)) // Czêœc wspólna dla obu trybów.
					{
						switch (D)
						{
						case 1:  gd=0;gj=1; break;
						case 2:  gd=0;gj=2; break;
						case 3:  gd=0;gj=3; break;
						case 4:  gd=0;gj=4; break;
						case 5:  gd=0;gj=5; break;
						case 6:  gd=0;gj=6; break;
						case 7:  gd=0;gj=7; break;
						case 8:  gd=0;gj=8; break;
						case 9:  gd=0;gj=9; break;
						case 10: gd=1;gj=0; break;
						case 11: gd=1;gj=1; break;
						case 12: gd=1;gj=2; break;
						}
					}

				if (hour_quantity==12)// tryb 12h			// Godziny popo³udniowe i wieczonrne w trybie 12H.
					{
						switch (D)
						{
						case 13: gd=0;gj=1; break;
						case 14: gd=0;gj=2; break;
						case 15: gd=0;gj=3; break;
						case 16: gd=0;gj=4; break;
						case 17: gd=0;gj=5; break;
						case 18: gd=0;gj=6; break;
						case 19: gd=0;gj=7; break;
						case 20: gd=0;gj=8; break;
						case 21: gd=0;gj=9; break;
						case 22: gd=1;gj=0; break;
						case 23: gd=1;gj=1; break;
						case 24: gd=1;gj=2; break;
						}
					}
				if (hour_quantity==24)//tryb 24h			// Godziny popo³udniowe i wieczonrne w trybie 24H.
					{
						switch (D)
						{
						case 13: gd=1;gj=3; break;
						case 14: gd=1;gj=4; break;
						case 15: gd=1;gj=5; break;
						case 16: gd=1;gj=6; break;
						case 17: gd=1;gj=7; break;
						case 18: gd=1;gj=8; break;
						case 19: gd=1;gj=9; break;
						case 20: gd=2;gj=0; break;
						case 21: gd=2;gj=1; break;
						case 22: gd=2;gj=2; break;
						case 23: gd=2;gj=3; break;
						case 24: gd=0;gj=0; break;
						}
					}
}
void blink_digits (void)
{
	while(!(interval(blink_ms)))
		{
		cy1=10;cy2=10;cy3=10;cy4=10;
		if (key_down(SET_MM_HH)){s++;};
		if(key_down(SET_TIME)){ b++;};
		}
	while(!(interval(blink_ms)))
	{
		cy1=gd;cy2=gj;cy3=md;cy4=mj;
		if (key_down(SET_MM_HH)){s++;};
		if(key_down(SET_TIME)){ b++;};
	}
}
void blink_hh_digits (void)
{
	while(!(interval(blink_ms)))
		{
		cy1=10;cy2=10;cy3=md;cy4=mj;
		if (key_down(SET_MM_HH)){s++;};
		}
	while(!(interval(blink_ms)))
		{
		cy1=gd;cy2=gj;cy3=md;cy4=mj;
		if (key_down(SET_MM_HH)){s++;};
		}
}
void blink_mm_digits (void)
{
	while(!(interval(blink_ms)))
		{
		cy1=gd;cy2=gj;cy3=10;cy4=10;
		if (key_down(SET_MM_HH)){s++;};
		}
	while(!(interval(blink_ms)))
	{
		cy1=gd;cy2=gj;cy3=md;cy4=mj;
		if (key_down(SET_MM_HH)){s++;};
	}
}
bool interval(uint16_t millis)
{
	if((_interval%millis)==0){return 1;}
	else return 0;
}
uint8_t key_down (uint8_t KEY)
{
	if (!(PIND&KEY))
		{
		PORTC&=~SIGNAL_LED;
		PORTD|=BUZZER;
		_delay_ms(80);
		PORTC|=SIGNAL_LED;
		PORTD&=~BUZZER;
		if (!(PIND&KEY)){return 1;_interval=0;};
		}
	return 0;
}
ISR (TIMER2_COMP_vect)
{
	i++;						// Zliczanie kolejnych ms na potrzeby obliczania godziny.
	_interval++;
}




















