////////////////////////////////////////////////////////////////////////////////////////////
// ENGINE V 0.1.0
////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __PEGGYLIB_H__
#define __PEGGYLIB_H__

#define FB_W 25
#define FB_H 25

// peggy is a 25x25 LED display
// so 4 bit per LED => 25 * 4 = 100 bits per row => 12.5 bytes => rounded to 13 bytes per row
// 25 rows * 13 bytes per row == 325
#define PEGGY_W 13
#define PEGGY_H 25
#define DISP_BUFFER_SIZE 325
#define FPS 70
#define MAX_BRIGHTNESS 15
#define SOFT_SERIAL_BPS 9600	

uint8_t frameBuffer[DISP_BUFFER_SIZE];

// user defined main loop
void engineFrameUpdate();

// peggy internal stuff
#ifdef _WIN32
void uartInit() {};
void displayInit(void) {};
void enablePullupsForButtons() {};
#else
void enablePullupsForButtons()
{
	PORTB |= (1 << 0);
	PORTC |= (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
}

void uartInit()
{
	Serial.begin(SOFT_SERIAL_BPS);
}

void displayInit(void)
{
	// set outputs to 0
	PORTC = 0;
	// leave serial pins alone
	PORTD &= (1 << 1) | (1 << 0);

	// need to set output for SPI clock, MOSI, SS and latch.  Eventhough SS is not connected,
	// it must apparently be set as output for hardware SPI to work.
	DDRB = (1 << DDB5) | (1 << DDB3) | (1 << DDB2) | (1 << DDB1);
	// SCL/SDA pins set as output
	DDRC = (1 << DDC5) | (1 << DDC4);
	// set portd pins as output, but leave RX/TX alone
	DDRD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD4) | (1 << DDD3) | (1 << DDD2);

	// enable hardware SPI, set as master and clock rate of fck/2
	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);

	// setup the interrupt. => probably for this below: SIGNAL(TIMER0_COMPA_vect)
	TCCR0A = (1 << WGM01); // clear timer on compare match

	TCCR0B = (1 << CS01); // timer uses main system clock with 1/8 prescale

	OCR0A = (F_CPU >> 3) / 25 / 15 / FPS; // Frames per second * 15 passes for brightness * 25 rows
	TIMSK0 = (1 << OCIE0A);	// call interrupt on output compare match
}

// called by timer interrupt, reads framebuffer and set the leds
void setCurrentRow(uint8_t row, uint8_t spi1, uint8_t spi2, uint8_t spi3, uint8_t spi4)
{
	uint8_t portD;
	uint8_t portC;

	// precalculate the port values to set the row.
	if (row < 15)
	{
		row++;
		portC = ((row & 3) << 4);
		portD = (row & (~3));
	}
	else
	{
		row = (row - 14) << 4;
		portC = ((row & 3) << 4);
		portD = (row & (~3));
	}

	// set all rows to off
	PORTD = 0;
	PORTC = 0;

	// set row values.  Wait for xmit to finish.
	// Note: wasted cycles here, not sure what I could do with them,
	// but it seems a shame to waste 'em.
	SPDR = spi1;
	while (!(SPSR & (1 << SPIF)))  {}
	SPDR = spi2;
	while (!(SPSR & (1 << SPIF)))  {}
	SPDR = spi3;
	while (!(SPSR & (1 << SPIF)))  {}
	SPDR = spi4;
	while (!(SPSR & (1 << SPIF)))  {}

	// Now that the 74HC154 pins are split to two different ports,
	// we want to flip them as quickly as possible.  This is why the
	// port values are pre-calculated	
	PORTB |= (1 << 1);

	PORTD = portD;
	PORTC = portC;

	PORTB &= ~((1 << 1));
}

uint8_t * rowPtr;
uint8_t currentRow = 0;
uint8_t currentBrightness = 0;
uint8_t currentBrightnessShifted = 0;

SIGNAL(TIMER0_COMPA_vect)
{
	// there are 15 passes through this interrupt for each row per frame.
	// ( 15 * 25) = 375 times per frame.
	// during those 15 passes, a led can be on or off.
	// if it is off the entire time, the perceived brightness is 0/15
	// if it is on the entire time, the perceived brightness is 15/15
	// giving a total of 16 average brightness levels from fully on to fully off.
	// currentBrightness is a comparison variable, used to determine if a certain
	// pixel is on or off during one of those 15 cycles.   currentBrightnessShifted
	// is the same value left shifted 4 bits:  This is just an optimization for
	// comparing the high-order bytes.

	currentBrightnessShifted += 16; // equal to currentBrightness << 4

	if (++currentBrightness >= MAX_BRIGHTNESS)
	{
		currentBrightnessShifted = 0;
		currentBrightness = 0;
		if (++currentRow > 24)
		{
			currentRow = 0;
			rowPtr = frameBuffer;
			engineFrameUpdate();
		}
		else
		{
			rowPtr += 13;
		}
	}

	// rather than shifting in a loop I manually unrolled this operation
	// because I couldnt seem to coax gcc to do the unrolling it for me.
	// (if much more time is taken up in this interrupt, the serial service routine
	// will start to miss bytes)
	// This code could be optimized considerably further...

	uint8_t * ptr = rowPtr;
	uint8_t p, a, b, c, d;

	a = b = c = d = 0;

	// pixel order is, from left to right on the display:
	//  low order bits, followed by high order bits
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		a |= 1;
	if ((p & 0xf0) > currentBrightnessShifted)	a |= 2;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		a |= 4;
	if ((p & 0xf0) > currentBrightnessShifted)	a |= 8;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		a |= 16;
	if ((p & 0xf0) > currentBrightnessShifted)	a |= 32;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		a |= 64;
	if ((p & 0xf0) > currentBrightnessShifted)	a |= 128;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		b |= 1;
	if ((p & 0xf0) > currentBrightnessShifted)	b |= 2;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		b |= 4;
	if ((p & 0xf0) > currentBrightnessShifted)	b |= 8;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		b |= 16;
	if ((p & 0xf0) > currentBrightnessShifted)	b |= 32;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		b |= 64;
	if ((p & 0xf0) > currentBrightnessShifted)	b |= 128;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		c |= 1;
	if ((p & 0xf0) > currentBrightnessShifted)	c |= 2;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		c |= 4;
	if ((p & 0xf0) > currentBrightnessShifted)	c |= 8;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		c |= 16;
	if ((p & 0xf0) > currentBrightnessShifted)	c |= 32;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		c |= 64;
	if ((p & 0xf0) > currentBrightnessShifted)	c |= 128;
	p = *ptr++;
	if ((p & 0x0f) > currentBrightness)  		d |= 1;
	//if ((p & 0xf0) > currentBrightnessShifted)  d|=2;

	setCurrentRow(currentRow, d, c, b, a);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// CORE V 0.1.0
////////////////////////////////////////////////////////////////////////////////////////////

unsigned int getOffset(uint8_t x, uint8_t y)
{
	return (x >> 1) + y * PEGGY_W;
}

bool isValid(uint8_t x, uint8_t y)
{
	return x < FB_W && y < FB_H;
}

uint8_t getPixel(uint8_t x, uint8_t y)
{
	if (!isValid(x, y))
		return 0;

	uint8_t c = frameBuffer[getOffset(x,y)];
	if (x & 1)
		c = (c & 0xf0) >> 4;
	else
		c = c & 0x0f;
	return c;
}

#include <assert.h>

void setPixel(uint8_t x, uint8_t y, uint8_t b)
{
	if (!isValid(x, y))
		return;

	if (b > MAX_BRIGHTNESS)
		b = MAX_BRIGHTNESS;

	uint8_t *pDst = frameBuffer + getOffset(x, y);
	if (x & 1)
		*pDst = (*pDst & 0x0f) | (b << 4);
	else
		*pDst = b | (*pDst & 0xf0);

	assert(getPixel(x, y) == b);
}

void clearFB()
{
	for (int i = 0; i<DISP_BUFFER_SIZE; i++)
		frameBuffer[i] = 0;
}

// W R W R
// G B G B
// W R W R
// G B G B

bool isWhite(int x, int y)
{
	return !(x & 1) && !(y & 1);
}

bool isGreen(int x, int y)
{
	return !(x & 1) && (y & 1);
}

bool isBlue(int x, int y)
{
	return (x & 1) && (y & 1);
}

bool isRed(int x, int y)
{
	return (x & 1) && !(y & 1);
}

void renderTestPattern()
{
	uint8_t *ptr = frameBuffer;
	for (int x = 0; x < FB_W; x++)
	{
		for (int y = 0; y < FB_H; y++)
		{
			uint8_t b = (y * 15) / FB_H;
			setPixel(x, y, (x & 1) ? b : (15-b));
		}
	}
}

void engineInit()
{	
	enablePullupsForButtons();
	uartInit();
	displayInit();
#ifndef _WIN32
	randomSeed(analogRead(0));
	sei();	
#endif
	renderTestPattern();
}

void renderTestAnimation()
{
	static float b = 0.f;
	static float bDir = 0.25f * 3.0f;

	static s8 offX = 0;
	b = clampf(b + bDir, 1.f, 15.f);
	if (b == 1.f || b == 15.f)
	{
		bDir = -bDir;
		if (b == 1.f)
		{
			offX++;
			offX %= FB_W;
		}
	}

	clearFB();
	for (int y = 0; y < FB_H;y++)
		for (int x = 0; x < FB_W; x++)
			setPixel((x + (s8)(offX)) % FB_W, y, ((x == y) || (x == (FB_H - 1 - y))) ? (uint8_t)b : 0);
}

////////////////////////////////////////////////////////////////////////////////////////////
// WIN32 EMULATION
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define PROGMEM 
#define pgm_read_byte_near(x) *(x)
#include <time.h>
#include <SDL.h>
#include <iostream>

void setup();

namespace SDL_Support
{
	SDL_Window *win = nullptr;
	SDL_Renderer *ren = nullptr;
	SDL_Surface *led_bmp = nullptr;
	SDL_Texture *led_tex = nullptr;

	const unsigned int margin = 8;

	bool init()
	{
		std::string imagePath = "led.bmp";
		led_bmp = SDL_LoadBMP(imagePath.c_str());
		if (led_bmp == nullptr)
		{
			std::cout << "cannot load bitmap " << imagePath.c_str() << " " << SDL_GetError() << std::endl;
			return false;
		}
		
		const unsigned int w = led_bmp->w * FB_W + margin * 2;
		const unsigned int h = led_bmp->h * FB_H + margin * 2;
		win = SDL_CreateWindow("Peggy Emulator", 32, 32, w, h, SDL_WINDOW_SHOWN);
		if (win == nullptr) 
		{
			std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
			return false;
		}

		ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (ren == nullptr)
		{
			std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
			return false;
		}

		led_tex = SDL_CreateTextureFromSurface(ren, led_bmp);
		if (led_tex == nullptr)
		{
			std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
			return false;
		}

		return true;
	}

	void quit()
	{
		if (led_bmp)
			SDL_FreeSurface(led_bmp);
		if (led_tex)
			SDL_DestroyTexture(led_tex);
		if (ren)
			SDL_DestroyRenderer(ren);
		if (win)
			SDL_DestroyWindow(win);
		SDL_Quit();
	}

	void handleKey(SDL_Event e)
	{
	}

	void handleMouse(SDL_Event e)
	{
	}

	bool run()
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				return false;
			else if (e.type == SDL_KEYDOWN)
				handleKey(e);
			else if (e.type == SDL_MOUSEBUTTONDOWN)
				handleMouse(e);			
		}
		return true;
	}

	void render()
	{
		SDL_RenderClear(ren);
		for (auto y = 0; y < FB_H; y++)
			for (auto x = 0; x < FB_W; x++)
			{
				// choose led base color
				Uint8 r = 0, g = 0, b = 0;
				if (isWhite(x, y))
					r = g = b = 255;
				else
				{
					if (isRed(x, y))
						r = 255;
					else if (isBlue(x, y))
						b = 255;
					else
						g = 255;
				}

				// get brightness
				uint8_t br = getPixel(x, y);
				float brightness = br / 15.0f;

				// gamma correction
				brightness = pow(brightness, 1.0f / 2.2f);

				// final color mask
				r = (Uint8)(r * brightness);
				g = (Uint8)(g * brightness);
				b = (Uint8)(b * brightness);
				SDL_SetTextureColorMod(led_tex, r, g, b);

				// dest rect
				SDL_Rect dstRect = { x * led_bmp->w + margin / 2,
					y * led_bmp->h + margin / 2,
					led_bmp->w,
					led_bmp->h };

				// blit
				SDL_RenderCopy(ren, led_tex, NULL, &dstRect);
			}					
		SDL_RenderPresent(ren);
	}
};

int main(int argc, char** argv)
{
	if (SDL_Support::init())
	{		
		setup(); // user
		while (SDL_Support::run())
		{			
			engineFrameUpdate(); // user
			SDL_Support::render();
		}
	}
	SDL_Support::quit();
	return 0;
}

int random(int min, int max)
{
	static bool _init = false;
	if (!_init)
	{
		_init = true;
		srand((unsigned int)time(NULL));
	}

	if (min >= max)
		return min;
	else
		return min + rand() % (max - min);
}

#endif

#endif // __PEGGYLIB_H__