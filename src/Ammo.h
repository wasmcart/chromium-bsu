/*
 * Copyright (c) 2000 Mark B. Allan. All rights reserved.
 *
 * "Chromium B.S.U." is free software; you can redistribute
 * it and/or use it and/or modify it under the terms of the
 * "Clarified Artistic License"
 */
#ifndef Ammo_h
#define Ammo_h

//====================================================================
class ActiveAmmo
{
public:
	ActiveAmmo();
	~ActiveAmmo();

	void	init(float p[3], float v[3], float d);

	inline void	updatePos(float sa=1.0f) { pos[0]+=vel[0]*sa; pos[1]+=vel[1]*sa; pos[2]+=vel[2]*sa;}

	float	pos[3];
	float	vel[3];
	float	damage;

	ActiveAmmo *back;
	ActiveAmmo *next;

private:
	static int ammoCount;
};


#endif // Ammo_h
