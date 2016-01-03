#ifndef _WIN32
#include "Arduino.h"
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/sleep.h> 
#include <avr/pgmspace.h> 
#endif

#include <tools.h>
#include <peggylib.h>

#include "Slot.h"
#include "Luminion.h"

////////////////////////////////////////////////////////////////////////////////////////////
// SAPIN
////////////////////////////////////////////////////////////////////////////////////////////

const float sapinDefaultrightness = 0.45f;

#define _ 0
#define O 15
const uint8_t sapin[25 * 25] PROGMEM = {
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,O,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,O,_,O,_,O,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,O,_,O,_,O,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,O,_,O,_,O,_,O,_,O,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,O,_,O,_,O,_,O,_,O,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,O,_,_,
	_,_,_,_,_,_,_,_,_,_,O,_,O,_,O,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,O,_,O,_,O,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,_,_,O,_,O,_,O,_,_,_,_,_,_,_,_,_,_
};
#undef _
#undef O

const float GLOBAL_SPEED = 1.0f;
const float DT = 1.0f / 70.0f;

class greenLine
{
public:
	greenLine() {
		start = 255;
		end = 0;
	};

	uint8_t start;
	uint8_t end;
};

uint8_t greenLines_startY = 255;
uint8_t greenLines_endY = 0;
greenLine greenLines[FB_H];

void initGreenLines()
{
	for (int y = 0; y < FB_H; y++)
	{
		for (int x = 0; x < FB_W; x++)
		{
			if (getPixel(x, y) && isGreen(x, y))
			{
				greenLines[y].start = min_u8(greenLines[y].start, x);
				greenLines[y].end = max_u8(greenLines[y].end, x);
				greenLines_startY = min_u8(greenLines_startY, y);
				greenLines_endY = max_u8(greenLines_endY, y);
			}
		}
	}
}

void updateGreenLines()
{
	static float curY = 100.0f; // if using 1.0f instead of 100 => fail on arduino, precision ...
	const float speed = 100.0f / 60.0f;
	curY -= speed/4.0f;
	if (curY <= 0.0f)
		curY += 100.0f;
	float T = GLOBAL_SPEED * curY/100.0f;
	uint8_t iY = greenLines_startY + uint8_t(T*(1 + greenLines_endY - greenLines_startY));
	for (int y = 0; y < FB_H; y++)
	{
		if (greenLines[y].start <= greenLines[y].end)
		{
			uint8_t b = (iY == y) ? 15 : (uint8_t)(15.0f * sapinDefaultrightness);
			for (int x = greenLines[y].start; x <= greenLines[y].end; x += 2)
				setPixel(x, y, b);
		}
	}
}

void renderSapin(float brightness)
{
	clearFB();
	for (int i = 0; i<25 * 25; i++)
	{
		uint8_t b = (uint8_t)(pgm_read_byte_near(sapin + i));
		setPixel(i % 25, i / 25, (uint8_t)(b*brightness));
	}
	initGreenLines();
}

////////////////////////////////////////////////////////////////////////////////////////////
// LUMINIONS
////////////////////////////////////////////////////////////////////////////////////////////

const int MAX_LUMINION_SLOTS = 120; // 100
int nbLuminionSlots = 0;
Slot luminionSlots[MAX_LUMINION_SLOTS];

void initLuminionSlotsFromFB()
{
	for (int y = 0; y < 25; y++)
		for (int x = 0; x < 25; x++)
			if (nbLuminionSlots < MAX_LUMINION_SLOTS)
			{
				int nbGreenAround = 0;
				for (int dx = (x - 1); dx <= (x + 1); dx++)
					for (int dy = (y - 1); dy <= (y + 1); dy++)
						nbGreenAround += (dx != x || dy != y) && isGreen(dx, dy) && getPixel(dx, dy) > 0;

				if ((isRed(x, y) && nbGreenAround >= 3) || (isBlue(x, y) && nbGreenAround >= 2))
					luminionSlots[nbLuminionSlots++].reset(x, y);					
			}
}

Luminion* spawnLuminionOnSapin(float luminionSpeed)
{
	int nbTries = 3;
	while (nbTries--)
	{		
		int slot = random(0, nbLuminionSlots - 1);
		Slot* pSlot = luminionSlots + slot;
		if (Luminion::closestDistance(pSlot->x, pSlot->y) > 3.0f)
		{
			Luminion* pL = Luminion::create(pSlot, luminionSpeed);
			if (pL)
				return pL;
		}
	}
	return 0;
}

Slot StarSlot;
Luminion* pStar = 0;

void chooseStarSlot()
{
	static uint8_t side = 0;
	const uint8_t w = 3;
	const uint8_t h = 6;

	StarSlot.x = random(1, w) * 2;
	StarSlot.y = random(1, h) * 2;

	if ((side++ & 1) || (random(0, 100)<5))
		StarSlot.x = FB_W - 1 - StarSlot.x;
}

void initStar()
{
	chooseStarSlot();
	pStar = Luminion::create(&StarSlot, 1.0f);
	pStar->reserve();
	pStar->setBig(true);
}

void updateStar()
{	
	if (!pStar->isAlive())
	{
		chooseStarSlot();

		float r = random(0, 1000) / 1000.f;
		float mul = 0.5f;
		float min = 0.9f * mul;
		float max = 1.1f * mul;
		float speed = min + r * (max - min);

		pStar->restart(&StarSlot, speed);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// INIT / MAIN LOOP
////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{	
	engineInit();
	clearFB();
	renderSapin(sapinDefaultrightness);
	initStar();
	initLuminionSlotsFromFB();
}

//void testOneLuminon()
//{
//	static Luminion *pL = 0;
//	if (!pL)
//		pL = Luminion::create(luminionSlots, 1.0f);
//	else if (!pL->isAlive())
//		pL->revive();
//}

void engineFrameUpdate()
{
	updateStar();
	updateGreenLines();

	//float r = random(0, 1000) / 1000.f;
	//float min = 0.5f;
	//float max = min * 2.0f;
	//spawnLuminionOnSapin(min + r * (max - min));

	Luminion::renderAll(GLOBAL_SPEED*DT);
}

void loop()
{
	// keep this empty
}
