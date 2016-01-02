class Slot
{
public:
	Slot() : x(0), y(0), grabbed(false) {};
	
	void reset(int _x, int _y) 
	{
		x = _x;
		y = _y;
		grabbed = false;
	}

	void unGrab()
	{
		grabbed = false;
	}

	bool grab() 
	{ 
		if (grabbed) 
			return false; 
		else 
			return (grabbed = true); 
	}	

	uint8_t x, y;

private:
	bool grabbed;
};