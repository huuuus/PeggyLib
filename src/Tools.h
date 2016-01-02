typedef unsigned char uint8_t;
typedef char s8;

const float pi = 3.1415f;

int clampi(int v, int min, int max)
{
	if (v<min)
		return min;
	if (v>max)
		return max;
	return v;
}

float clampf(float v, float min, float max)
{
	if (v<min)
		return min;
	if (v>max)
		return max;
	return v;
}

float sinLerp(float t)
{
	float v;
	if (t >= 0.5f)
		t = 1.0f - t;
	t *= 2.f;
	v = t*t * (3.f - 2.f * t);

	//v = sin(t * pi);

	//v = 0.5f * (1.0f - cos(t * pi * 2.0f));

	return clampf(v, 0.f, 1.f);
}

uint8_t min_u8(uint8_t x, uint8_t y)
{
	if (x < y)
		return x;
	else
		return y;
}

uint8_t max_u8(uint8_t x, uint8_t y)
{
	if (x > y)
		return x;
	else
		return y;
}