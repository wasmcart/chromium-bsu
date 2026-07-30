/*
 * Copyright (c) 2000 Mark B. Allan. All rights reserved.
 *
 * "Chromium B.S.U." is free software; you can redistribute
 * it and/or use it and/or modify it under the terms of the
 * "Clarified Artistic License"
 */
#include "EnemyAircraft_Tank.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "Config.h"

#include "define.h"
#include "Ammo.h"
#include "Global.h"
#include "EnemyAmmo.h"
#include "HeroAircraft.h"
#include "ScreenItemAdd.h"

//=================================================================
EnemyAircraft_Tank::EnemyAircraft_Tank(EnemyType et, float p[3], float randFact)
	: EnemyAircraft(et, p, randFact)
{
	init(p, randFact);
}

//----------------------------------------------------------
EnemyAircraft_Tank::~EnemyAircraft_Tank()
{
}

//----------------------------------------------------------
// this is only here to get rid of the IRIX compiler warning...
void EnemyAircraft_Tank::init()
{
	EnemyAircraft::init();
}

//----------------------------------------------------------
void EnemyAircraft_Tank::init(float *p, float randFact)
{
	EnemyAircraft::init(p, randFact);

	damage = baseDamage = -2000.0*game->gameSkill;
	size[0] = 1.9;
	size[1] = 2.1;
	collisionMove = 0.1;
	vel[1] =  0.03*0.60f;

}

//-- NOTE: Many of the firing rates are not adjusted by game->speedAdj
//-- so they will not be totally correct. Should be close enough for jazz, though.
//----------------------------------------------------------
void EnemyAircraft_Tank::update()
{
	float	v[3] = { 0.0, -0.2, 0.0 };
//	float	*hpos = target->getPos();
	float	*hpos = game->hero->getPos();
	float	a = hpos[0]-pos[0];
	float	b = hpos[1]-pos[1];
	float	dist;
	float	ammoSpeed = 0.35;

	int		tmpInt;
	//-- update age
	age += game->speedAdj;
	shootInterval -= game->speedAdj;

	pos[0] += secondaryMove[0]*game->speedAdj;
	pos[1] += secondaryMove[1]*game->speedAdj;
	float s = (1.0-game->speedAdj)+(game->speedAdj*0.7);
	secondaryMove[0] *= s;
	secondaryMove[1] *= s;
	move();

	float	p[3] = { pos[0], pos[1], pos[2] };

	p[1] = pos[1] - 1.7;
	if(fabs(a) < 4.0)
	{
		{
			int sw = (int)shootSwap;
			if(sw == 0 || sw == 8 || sw == 16 )
			{
				if(ageInterval(1)) {
					v[1] = -0.2;
					p[0] = pos[0] + 1.5;
					game->enemyAmmo->addAmmo(0, p, v);
					p[0] = pos[0] - 1.5;
					game->enemyAmmo->addAmmo(0, p, v);
				}
			}
		}
		shootSwap += game->speedAdj;
		if((int)shootSwap >= 100) shootSwap = 0;
	}

	if(!((tmpInt = (int)age/200)%2)) //-- omni shooters
	{
		tmpInt = (int)age%200;
		if(tmpInt < 100)
		{
			preFire = (float)tmpInt/100.0f;
		}
		else if(tmpInt < 170)
		{
			if(ageInterval(10))
			{
				p[1] = pos[1]-0.45;
				b = hpos[1]-p[1];

				p[0] = pos[0];
				a = hpos[0]-p[0];
				dist = fabs(a) + fabs(b);
				shootVec[0] = 2.0*ammoSpeed*a/dist;
				shootVec[1] = 2.0*ammoSpeed*b/dist;
				secondaryMove[0] -= shootVec[0]*0.1;
				secondaryMove[1] -= shootVec[1]*0.1;
				game->enemyAmmo->addAmmo(2, p, shootVec);
				preFire -= 0.4*game->speedAdj;
				if(preFire < 0.0)
					preFire = 0.0;
			}
			else
				preFire += 0.035*game->speedAdj;
		}
		else
			preFire = 0.0;
	}

}

//----------------------------------------------------------
void EnemyAircraft_Tank::move()
{
	Config *config = Config::instance();
	float	*hpos;
	if(target)
		hpos = target->getPos();
	else
		hpos = pos;
	float	diff[2] = { hpos[0]-pos[0], hpos[1]-pos[1] };
	float	v1;


	if(fabs(diff[0]) > 8.0)
		v1 = 0.04;
	else
	{
		v1 = 0.04*(fabs(diff[0])/8.0);
	}
	{
		float d = game->speedAdj;
		float decay99 = powf(0.99f, d);
		vel[1] = decay99*vel[1] + (1.0f-decay99)*v1;

		if(pos[1] < -3.0)
			vel[1] = -0.1;
		else if(pos[1] < 0.0)
			vel[1] *= decay99;

		float decay998 = powf(0.998f, d);
		if(pos[0] < 0.0)
			pos[0] = decay998*pos[0] + (1.0f-decay998)*(-config->screenBoundX()+2.85);
		else
			pos[0] = decay998*pos[0] + (1.0f-decay998)*( config->screenBoundX()-2.85);
	}
	switch(((int)age/50)%8)
	{
		case 2:
			pos[1] += game->speedAdj*(0.05);
			break;
		default:
			pos[1] -= game->speedAdj*(vel[1]);
			break;
	}


	if(pos[0] < -config->screenBoundX())
		pos[0] = -config->screenBoundX();
	if(pos[0] >  config->screenBoundX())
		pos[0] =  config->screenBoundX();
}
