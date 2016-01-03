const int MAX_LUMINION = 7;

class Luminion
{
public:
	static Luminion* create(Slot* pSlot, float speed)
	{
		Luminion* pFree = 0;
		for (Luminion* pL = l; pL<(l+MAX_LUMINION); pL++)
		{
			if (!pL->isAlive() && !pL->isReserved())
			{
				pFree = pL;
				break;
			}
		}

		if (pFree && pSlot->grab())
		{
			pFree->reset(pSlot, speed);
			return pFree;
		}

		return 0;
	}

	bool isReserved()
	{
		return reserved;
	}

	void reserve()
	{		
		reserved = true;
	}

	void setBig(bool big)
	{
		isBig = big;
	}

	static void renderAll(float dt)
	{
		for (Luminion* pL = l; pL<(l + MAX_LUMINION); pL++)
		{
			if (pL->isAlive())
				if (pL->update(dt))
					pL->render();
		}
	}

	bool isAlive()
	{
		return cur_slot && (b >= 0.f);
	}

	void revive()
	{
		 while (localT>=1.0f)
			 localT -= 1.0f;
		 update(0.f);
	}

	static float closestDistance(float x, float y)
	{
		float min = 999.f;
		for (int i = 0; i < MAX_LUMINION; i++)
		{
			if (l[i].isAlive())
			{
				float dx = x - l[i].cur_slot->x;
				float dy = y - l[i].cur_slot->y;
				float d = sqrt(dx*dx + dy*dy);
				if (d < min)
					min = d;
			}
		}
		return min;
	}	

	void restart(Slot* _slot, float _speed)
	{
		cur_slot = _slot;
		speed = _speed;
		localT = 0.0f;
		b = 0.0f;
	}

private:
	Luminion() :
		cur_slot(0), speed(0), localT(0), b(-1.0f), isBig(0), reserved(false)
	{
	}	

	bool update(float dt)
	{
		localT += dt*speed;
		if (localT > 1.0f)
			b = -1.0f;
		else
			b = sinLerp(localT) * 15.0f;
		return isAlive();
	}

	void reset(Slot* _slot, float _speed)
	{
		if (cur_slot)
			cur_slot->unGrab();
		restart(_slot, _speed);
	}

	void render()
	{
		uint8_t x = cur_slot->x;
		uint8_t y = cur_slot->y;
		uint8_t b1 = (uint8_t)b;		
		setPixel(x, y, b1);

		if (isBig)
		{
			uint8_t b2 = (uint8_t)(b / 2.0f);
			setPixel(x - 2, y, b2);
			setPixel(x, y - 2, b2);
			setPixel(x + 2, y, b2);
			setPixel(x, y + 2, b2);
		}
	}

private:
	Slot* cur_slot;
	float b;
	float localT;
	float speed;
	uint8_t isBig;
	bool reserved;

	static Luminion l[MAX_LUMINION];
};

Luminion Luminion::l[MAX_LUMINION];

