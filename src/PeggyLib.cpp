#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Tools.h"
#include "PeggyLib.h"

void setup()
{
	engineInit();
	clearFB();
}

void engineFrameUpdate()
{
	renderTestAnimation();
}

void loop()
{
	// keep this empty
}