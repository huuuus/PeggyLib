#ifdef _WIN32

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

#endif