#include "Renderer.h"
/*
 * Copyright (c) 2000 Mark B. Allan. All rights reserved.
 *
 * "Chromium B.S.U." is free software; you can redistribute
 * it and/or use it and/or modify it under the terms of the
 * "Clarified Artistic License"
 */

#ifdef HAVE_CONFIG_H
#include <chromium-bsu-config.h>
#endif

#include "gettext.h"

#include "StatusDisplay.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "compatibility.h"

#if defined(HAVE_APPLE_OPENGL_FRAMEWORK) || defined(HAVE_OPENGL_GL_H)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "Config.h"

#include "define.h"
#include "extern.h"
#include "Global.h"
#include "Explosions.h"
#include "Image.h"


static float statPosAmmo[3] =	{-10.5,  8.00, 25.0 };
static float statPosShld[3] =	{-10.4, -7.80, 25.0 };
static float statClrWarn[4] = 	{ 1.1, 0.6, 0.1, 1.1 };
static float statClrZero[4] = 	{ 0.0, 0.0, 0.0, 0.0 };
static float statClrAmmo[NUM_HERO_AMMO_TYPES][4] = {
										{ 1.0, 0.7, 0.5, 0.6 },
										{ 0.0, 1.0, 0.5, 0.7 },
										{ 0.3, 0.0, 1.0, 0.7 } };

//====================================================================
StatusDisplay::StatusDisplay()
{
	game = Global::getInstance();

	ammoAlpha = 0.0;
	damageAlpha = 0.0;
    shieldAlpha = 0.0;

	enemyWarn = 0.0;

	tipShipShow  = 0;
	tipSuperShow = 0;

	loadTextures();

	blink	= true;
}

StatusDisplay::~StatusDisplay()
{
	deleteTextures();
}

//----------------------------------------------------------
void StatusDisplay::loadTextures()
{
	int i;
	char	filename[128];
#ifdef GL_CLAMP_TO_EDGE
	GLenum clamp = GL_CLAMP_TO_EDGE;
#else
	GLenum clamp = GL_CLAMP;
#endif

	statTex      = Image::load(dataLoc("png/statBar.png"), IMG_NOMIPMAPS, IMG_BLEND1, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	shldTex      = Image::load(dataLoc("png/shields.png"), IMG_SIMPLEMIPMAPS, IMG_BLEND1, clamp, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	topTex       = Image::load(dataLoc("png/stat-top.png"), IMG_NOMIPMAPS, IMG_BLEND1, clamp, GL_LINEAR, GL_NEAREST);
	heroSuperTex = Image::load(dataLoc("png/heroSuper.png"), IMG_NOMIPMAPS, IMG_ALPHA, clamp, GL_LINEAR, GL_LINEAR);
	heroShieldTex = Image::load(dataLoc("png/heroShields.png"), IMG_NOMIPMAPS, IMG_BLEND2, clamp, GL_LINEAR, GL_LINEAR);
	for(i = 0; i < NUM_HERO_AMMO_TYPES; i++)
	{
		sprintf(filename, "png/heroAmmoFlash%02d.png", i);
		heroAmmoFlash[i] = Image::load(dataLoc(filename), IMG_NOMIPMAPS, IMG_ALPHA, clamp, GL_LINEAR, GL_LINEAR);
	}
	useFocus = Image::load(dataLoc("png/useFocus.png"), IMG_NOMIPMAPS, IMG_ALPHA, clamp, GL_LINEAR, GL_LINEAR);
	for(i = 0; i < NUM_HERO_ITEMS; i++)
	{
		sprintf(filename, "png/useItem%02d.png", i);
		useItem[i] = Image::load(dataLoc(filename), IMG_NOMIPMAPS, IMG_ALPHA, clamp, GL_LINEAR, GL_LINEAR);
	}
}

//----------------------------------------------------------
void StatusDisplay::deleteTextures()
{
	int i;
	renderer_delete_textures(1, &statTex);
	renderer_delete_textures(1, &shldTex);
	renderer_delete_textures(1, &heroSuperTex);
	renderer_delete_textures(1, &heroShieldTex);
	for(i = 0; i < NUM_HERO_AMMO_TYPES; i++)
	{
		renderer_delete_textures(1, &heroAmmoFlash[i]);
	}
	for(i = 0; i < NUM_HERO_ITEMS; i++)
	{
		renderer_delete_textures(1, &useItem[i]);
	}
}

//----------------------------------------------------------
void StatusDisplay::darkenGL()
{
	//-- sidebars
	renderer_bind_texture( shldTex);
	renderer_begin(DRAW_QUADS);
	renderer_set_color(0.25, 0.2, 0.2, 0.6);
		renderer_set_texcoord(0.0,  0.0); renderer_vertex( -9.2,  8.5, 25.0);
		renderer_set_texcoord(1.0,  0.0); renderer_vertex(-11.5,  8.5, 25.0);
	renderer_set_color(0.25, 0.25, 0.35, 0.6);
		renderer_set_texcoord(1.0,  1.7); renderer_vertex(-11.5, -8.5, 25.0);
		renderer_set_texcoord(0.0,  1.7); renderer_vertex( -9.2, -8.5, 25.0);

	renderer_set_color(0.25, 0.2, 0.2, 0.6);
		renderer_set_texcoord(1.0, 0.0); renderer_vertex( 11.5,  8.5, 25.0);
		renderer_set_texcoord(0.0, 0.0); renderer_vertex(  9.2,  8.5, 25.0);
	renderer_set_color(0.25, 0.25, 0.35, 0.6);
		renderer_set_texcoord(0.0, 1.7); renderer_vertex(  9.2, -8.5, 25.0);
		renderer_set_texcoord(1.0, 1.7); renderer_vertex( 11.5, -8.5, 25.0);

	renderer_end();
}

//----------------------------------------------------------
void StatusDisplay::drawGL(HeroAircraft	*hero)
{
	Config	*config = Config::instance();
	static	char scoreBuf[32];
	int 	i;
	bool 	statClrWarnAmmo = false;
	float	w = 0.1;
	float	x = 0.0,y,y3;
	float	size[2];
	float	ammoStock;

	if(!hero)
		return;
	if((int)(game->gameTime*50.0f/15) != (int)((game->gameTime - game->speedAdj*0.02f)*50.0f/15))
		blink = !blink;

	ammoAlpha *= (1.0 - 0.04*game->speedAdj);

	float	shields = hero->getShields();
	float	superShields = 0.0;
	float	damage	= hero->getDamage();
	if(shields > HERO_SHIELDS)
	{
		superShields = HERO_SHIELDS-(shields-HERO_SHIELDS);
//		superShields = (shields-HERO_SHIELDS);
		shields = HERO_SHIELDS;
	}

	//-- draw score
	renderer_set_color(1.0, 1.0, 1.0, 0.4);
	renderer_push_matrix();
		sprintf(scoreBuf, "%07d", (int)hero->getScore());
		renderer_translate(-9.0, -8.2, 25.0);
		renderer_scale(0.025, 0.02, 1.0);
		game->text->Render(scoreBuf);
	renderer_pop_matrix();
	//-- draw fps
	if(config->showFPS())
	{
		renderer_push_matrix();
			sprintf(scoreBuf, "%3.1f", game->fps);
			renderer_translate(7.75, 8.0, 25.0);
			renderer_scale(0.018, 0.015, 1.0);
			game->text->Render(scoreBuf);
		renderer_pop_matrix();
	}

	//-- draw ship lives
	renderer_push_matrix();
	renderer_set_color(0.6, 0.6, 0.7, 1.0);
	renderer_bind_texture( game->hero->heroTex);
	renderer_translate(10.2, 7.4, 25.0);
	size[0] = game->hero->getSize(0)*0.5;
	size[1] = game->hero->getSize(1)*0.5;
	for(i = 0; i < game->hero->getLives(); i++)
	{
		drawQuad(size[0], size[1]);
		renderer_translate(0.0, -size[1]*2.0, 0.0);
	}
	renderer_pop_matrix();

	//-- draw usable items
	if(config->gfxLevel() > 1)
	{
		renderer_push_matrix();
		renderer_set_color(1.0, 1.0, 1.0, 1.0);
		renderer_translate(8.5, -7.7, 25.0);
		size[0] = 0.4;
		size[1] = 0.5;
		for(i = 0; i < NUM_HERO_ITEMS; i++)
		{
			if(i == game->hero->currentItem())
			{
				float a = game->hero->itemArmed()*0.8;
				renderer_set_color(0.4+a, 0.4, 0.4, 0.4+a);
				renderer_bind_texture( useFocus);
				drawQuad(size[1], size[1]);
				renderer_set_color(1.0, 1.0, 1.0, 1.0);
			}
			renderer_bind_texture( useItem[i]);
			drawQuad(size[0], size[0]);
			renderer_translate(-size[1]*2.0, 0.0, 0.0);
		}
		renderer_pop_matrix();
	}

	//-- draw 'enemy-got-past' Warning
	if(enemyWarn && game->hero->getLives() >= 0)
	{
		renderer_push_matrix();
		renderer_set_color(1.0, 0.0, 0.0, enemyWarn+0.15*sin(game->gameTime*35.0f));
		renderer_translate(0.0, -8.75, 25.0);
		renderer_bind_texture( heroAmmoFlash[0]);
		drawQuad(12.0, 3.0);
		renderer_pop_matrix();
		enemyWarn = 0.0;
	}

	//-- draw AMMO
	renderer_push_matrix();
	renderer_translate(statPosAmmo[0], statPosAmmo[1], statPosAmmo[2]);


	//--draw ammo reserves
	renderer_bind_texture( statTex);
	renderer_begin(DRAW_QUADS);
	for(i = 0; i < NUM_HERO_AMMO_TYPES; i++)
	{
 		renderer_set_color_v(statClrAmmo[i]);
		ammoStock = hero->getAmmoStock(i);
		if(ammoStock > 0.0)
		{
			x = i*0.3;
			y = ammoStock*0.02;
			y3= y*2.65;
			if( blink || ammoStock > 50.0 )
				renderer_set_color_v(statClrAmmo[i]);
			else
			{
				statClrWarnAmmo = true;
				renderer_set_color_v(statClrWarn);
			}

			renderer_set_texcoord(1.0, 0.00); renderer_vertex( x+w, -y3, 0.0 );
			renderer_set_texcoord(1.0,    y); renderer_vertex( x+w, 0.0, 0.0 );
			renderer_set_texcoord(0.0,    y); renderer_vertex( x-w, 0.0, 0.0 );
			renderer_set_texcoord(0.0, 0.00); renderer_vertex( x-w, -y3, 0.0 );
		}
	}
	renderer_end();

	renderer_bind_texture( topTex);
	if(statClrWarnAmmo)
		renderer_set_color(statClrWarn[0], statClrWarn[1], statClrWarn[2], 0.5+ammoAlpha);
	else
		renderer_set_color(0.5, 0.5, 0.5+ammoAlpha, 0.2+ammoAlpha);
	renderer_begin(DRAW_QUADS);
		renderer_set_texcoord(1.0,  1.0); renderer_vertex(  1.25, -1.85, 0.0 );
		renderer_set_texcoord(1.0,  0.0); renderer_vertex(  1.25,  0.47, 0.0 );
		renderer_set_texcoord(0.0,  0.0); renderer_vertex( -0.75,  0.47, 0.0 );
		renderer_set_texcoord(0.0,  1.0); renderer_vertex( -0.75, -2.85, 0.0 );
	renderer_end();
	x += w*1.5;

	renderer_pop_matrix();

	//--draw Shields
	damageAlpha *= (1.0 - 0.06*game->speedAdj);
	shieldAlpha *= (1.0 - 0.06*game->speedAdj);
	float	dc = damageAlpha*0.5;
	float   sc = shieldAlpha * 0.5;
	float	sl, sls, dl, dls;
	float	szx = 0.5;
	float	szy = 6.0;
	static	float rot = 0;
	rot+=2.0*game->speedAdj;
	float	rot2;
	rot2 = 2*((int)rot%180);

	sl  = sls = (shields/HERO_SHIELDS)-1.0;
	dl  = dls = ( damage/HERO_DAMAGE)-1.0;
	if(superShields)
		sls = dls = ((shields+superShields)/HERO_SHIELDS)-1.0;

	//------ draw Engine
	if(hero->isVisible() && config->gfxLevel() >= 1)
	{
		float c1f = 1.0+dl;
		float c2f = -dl;
		float c1[4] = { 0.85, 0.65, 1.00, 0.7 };
		float c2[4] = { 1.00, 0.20, 0.25, 0.7 };
//		renderer_set_color(0.9, 0.7, 1.0, 0.7);
		renderer_set_color(	c1[0]*c1f+c2[0]*c2f,
					c1[1]*c1f+c2[1]*c2f,
					c1[2]*c1f+c2[2]*c2f,
					c1[3]*c1f+c2[3]*c2f);
		renderer_bind_texture( heroAmmoFlash[0]);
		renderer_push_matrix();
		renderer_translate(hero->pos[0], hero->pos[1]-0.625, hero->pos[2]);
		float esz = 1.0+c2f;
		drawQuad(1.3, 0.5*esz);
		renderer_translate(0.0, -0.18, 0.0);
		renderer_rotate(IRAND, 0.0, 0.0, 1.0);
		drawQuad(0.85*esz, 0.6*esz);
		renderer_pop_matrix();
	}

//	if(shields > 0)
//	{
//		renderer_push_matrix();
//		float sz = hero->getSize(1)*1.5;
//		renderer_set_color(0.5, 0.5, 1.0, 0.2);
//		renderer_bind_texture( heroShieldTex);
//		renderer_translate(hero->pos[0], hero->pos[1]-0.05, hero->pos[2]);
//		renderer_rotate(IRAND, 0.0, 0.0, 1.0);
//		drawQuad(sz, sz);
//		renderer_pop_matrix();
//	}

	//------ draw Super Shields
	if(superShields)
	{
		renderer_push_matrix();
		float sz = hero->getSize(1)*1.3;
		renderer_set_color(1.0, 1.0, 1.0, 1.0-sls*sls);
		renderer_bind_texture( heroSuperTex);
		renderer_translate(hero->pos[0], hero->pos[1], hero->pos[2]);
		renderer_rotate(IRAND, 0.0, 0.0, 1.0);
		drawQuad(sz, sz);
		renderer_pop_matrix();

		//------ add a bit of Glitter...
		if(config->gfxLevel() > 1 && (!game->game_pause) )
		{
			float p[3] = { 0.0, 0.0, hero->pos[2] };
			float v0   = 0.01*SRAND;
			float v[3] = { v0, 0.0, 0.0 };
			float c3   = 1.0-sls*sls;
			float c[4] = { 1.0, 1.0, 0.7, c3 };
			switch((int)(game->gameTime/0.04f)%2)
			{
				case 0:
					v[1] = -0.3+FRAND*0.05;
					p[0] = hero->pos[0];
					p[1] = hero->pos[1]-0.8;
					game->explosions->addGlitter(p, v, c, 0, 0.4+0.4*FRAND);
					v[1] = -0.25+FRAND*0.05;
					p[0] = hero->pos[0]+0.95;
					p[1] = hero->pos[1]+0.1;
					game->explosions->addGlitter(p, v, c, 0, 0.4+0.4*FRAND);
					p[0] = hero->pos[0]-0.95;
					p[1] = hero->pos[1]+0.1;
					game->explosions->addGlitter(p, v, c, 0, 0.4+0.4*FRAND);
					break;
				case 1:
					v[1] = -0.25+FRAND*0.05;
					p[0] = hero->pos[0]+0.95;
					p[1] = hero->pos[1]+0.1;
					game->explosions->addGlitter(p, v, c, 0, 0.4+0.4*FRAND);
					p[0] = hero->pos[0]-0.95;
					p[1] = hero->pos[1]+0.1;
					game->explosions->addGlitter(p, v, c, 0, 0.4+0.4*FRAND);
					break;
			}
		}
	}

	//---------- Draw ammo flash
	if(config->gfxLevel() > 1)
	{
		renderer_push_matrix();
		renderer_translate(hero->pos[0], hero->pos[1], hero->pos[2]);
		if(hero->gunFlash0[0])
		{
			renderer_bind_texture( heroAmmoFlash[0]);
			szx = hero->gunFlash0[0];
			szy = 0.46f*szx;
			renderer_set_color(0.75f, 0.75f, 0.75f, szx);
			renderer_translate( 0.275,  0.25, 0.0);
			drawQuad(szy, szy);
			renderer_translate(-0.550,  0.00, 0.0);
			drawQuad(szy, szy);
			renderer_translate( 0.275, -0.25, 0.0);

			if(hero->gunFlash1[0])
			{
				renderer_translate( 0.45, -0.10, 0.0);
				drawQuad(szy, szy);
				renderer_translate(-0.90,  0.00, 0.0);
				drawQuad(szy, szy);
				renderer_translate( 0.45,  0.10, 0.0);
			}
		}
		if(hero->gunFlash0[1])
		{
			renderer_bind_texture( heroAmmoFlash[1]);
			szx = hero->gunFlash0[1];
			szy = 0.8f*szx;
			renderer_set_color(1.0f, 1.0f, 1.0f, szx);
			renderer_translate(0.0,  0.7, 0.0);
			drawQuad(szy, szy);
			renderer_translate(0.0, -0.7, 0.0);
		}
		renderer_bind_texture( heroAmmoFlash[2]);
		if(hero->gunFlash0[2])
		{
			szx = hero->gunFlash0[2];
			szy = 0.65*szx;
			renderer_set_color(1.0f, 1.0f, 1.0f, szx);
			renderer_translate(-0.65, -0.375, 0.0);
			drawQuad(szy, szy);
			renderer_translate( 0.65,  0.375, 0.0);
		}
		if(hero->gunFlash1[2])
		{
			szx = hero->gunFlash1[2];
			szy = 0.65f*szx;
			renderer_set_color(1.0f, 1.0f, 1.0f, szx);
			renderer_translate( 0.65, -0.375, 0.0);
			drawQuad(szy, szy);
			renderer_translate(-0.65,  0.375, 0.0);
		}
		renderer_pop_matrix();
	}

//	//-- shield indicator
	renderer_bind_texture( shldTex);
	renderer_set_color(0.2, 0.2, 0.2, 0.5);
	renderer_begin(DRAW_QUADS);
	szx = 0.6;
	szy = 6.0;
		renderer_set_texcoord( 1.0, 1.0); renderer_vertex(  statPosShld[0]+szx,  statPosShld[1]+szy, statPosShld[2] );
		renderer_set_texcoord(-2.5, 1.0); renderer_vertex(  statPosShld[0]-2.0,  statPosShld[1]+szy, statPosShld[2] );
		renderer_set_texcoord(-2.5, 0.0); renderer_vertex(  statPosShld[0]-2.0,  statPosShld[1]+0.0, statPosShld[2] );
		renderer_set_texcoord( 1.0, 0.0); renderer_vertex(  statPosShld[0]+szx,  statPosShld[1]+0.0, statPosShld[2] );

		renderer_set_texcoord( 3.5, 1.0); renderer_vertex( -statPosShld[0]+2.0,  statPosShld[1]+szy, statPosShld[2] );
		renderer_set_texcoord( 0.0, 1.0); renderer_vertex( -statPosShld[0]-szx,  statPosShld[1]+szy, statPosShld[2] );
		renderer_set_texcoord( 0.0, 0.0); renderer_vertex( -statPosShld[0]-szx,  statPosShld[1]+0.0, statPosShld[2] );
		renderer_set_texcoord( 3.5, 0.0); renderer_vertex( -statPosShld[0]+2.0,  statPosShld[1]+0.0, statPosShld[2] );
	renderer_end();

	if(config->gfxLevel() > 0)
	{
		//-- Shields
		if( (sl < -0.7 && blink && shields > 0.0) || superShields )
			renderer_set_color_v(statClrWarn);
		else
			renderer_set_color(0.7+dc, 0.6+dc, 0.8+dc, 0.5+damageAlpha);
//			renderer_set_color(0.0+sc, 0.35+sc, 1.0+sc, 0.5+shieldAlpha);
		renderer_push_matrix();
		renderer_translate(statPosShld[0], statPosShld[1], statPosShld[2]);
//		renderer_scale(1.0, 1.0, 1.5);
		renderer_rotate(-rot, 0.0, 1.0, 0.0);
		renderer_begin(DRAW_QUADS);
		szx = 0.5;
		renderer_set_texcoord( 1.0,     sls); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 0.0,     sls); renderer_vertex( -szx,  szy,  szx );
		renderer_set_texcoord( 0.0, 1.0+sls); renderer_vertex( -szx,  0.0,  szx );
		renderer_set_texcoord( 1.0, 1.0+sls); renderer_vertex(  szx,  0.0,  szx );

		renderer_set_texcoord( 0.0,     sls); renderer_vertex(  szx,  szy, -szx );
		renderer_set_texcoord( 0.0, 1.0+sls); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 1.0, 1.0+sls); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 1.0,     sls); renderer_vertex( -szx,  szy, -szx );

		renderer_set_texcoord( 1.0,     sls); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 1.0, 1.0+sls); renderer_vertex(  szx,  0.0,  szx );
		renderer_set_texcoord( 0.0, 1.0+sls); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 0.0,     sls); renderer_vertex(  szx,  szy, -szx );

		renderer_set_texcoord( 1.0,     sls); renderer_vertex( -szx,  szy, -szx );
		renderer_set_texcoord( 1.0, 1.0+sls); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 0.0, 1.0+sls); renderer_vertex( -szx,  0.0,  szx );
		renderer_set_texcoord( 0.0,     sls); renderer_vertex( -szx,  szy,  szx );

		if(shields)
		{
			renderer_set_texcoord( 1.0, 1.0);
			renderer_set_color(0.3+sc, 0.4+sc, 1.0+sc, 0.5);
			renderer_vertex(  szx,  0.0,  szx );
			renderer_vertex(  szx,  0.0, -szx );
			renderer_vertex( -szx,  0.0, -szx );
			renderer_vertex( -szx,  0.0,  szx );
		}
		renderer_end();

		renderer_rotate( rot2, 0.0, 1.0, 0.0);
//		renderer_set_color(0.4+sc, 0.5+sc, 1.0+sc, 0.6+shieldAlpha);
		renderer_set_color(0.1+sc, 0.15+sc, 0.9+sc, 0.6+shieldAlpha);
		renderer_begin(DRAW_QUADS);
		szx = 0.4;
		renderer_set_texcoord( 1.0,     sl); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 0.0,     sl); renderer_vertex( -szx,  szy,  szx );
		renderer_set_texcoord( 0.0, 1.0+sl); renderer_vertex( -szx,  0.0,  szx );
		renderer_set_texcoord( 1.0, 1.0+sl); renderer_vertex(  szx,  0.0,  szx );

		renderer_set_texcoord( 0.0,     sl); renderer_vertex(  szx,  szy, -szx );
		renderer_set_texcoord( 0.0, 1.0+sl); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 1.0, 1.0+sl); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 1.0,     sl); renderer_vertex( -szx,  szy, -szx );

		renderer_set_texcoord( 1.0,     sl); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 1.0, 1.0+sl); renderer_vertex(  szx,  0.0,  szx );
		renderer_set_texcoord( 0.0, 1.0+sl); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 0.0,     sl); renderer_vertex(  szx,  szy, -szx );

		renderer_set_texcoord( 0.0,     sl); renderer_vertex( -szx,  szy,  szx );
		renderer_set_texcoord( 1.0,     sl); renderer_vertex( -szx,  szy, -szx );
		renderer_set_texcoord( 1.0, 1.0+sl); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 0.0, 1.0+sl); renderer_vertex( -szx,  0.0,  szx );
		renderer_end();
		renderer_pop_matrix();



		if( (dl < -0.7 && blink) || superShields )
		{
			renderer_set_color_v(statClrWarn);
			if(config->texBorder())
				renderer_tex_parameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, statClrWarn );
		}
		else
		{
			renderer_set_color(0.9+dc, 0.6+dc, 0.7+dc, 0.5+damageAlpha);
			if(config->texBorder())
				renderer_tex_parameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, statClrZero );
		}
		//-- Life
		renderer_push_matrix();
		renderer_translate(-statPosShld[0], statPosShld[1], statPosShld[2]);
		renderer_rotate( rot, 0.0, 1.0, 0.0);

		renderer_begin(DRAW_QUADS);
		szx = 0.5;
		renderer_set_texcoord( 1.0,     dls); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 0.0,     dls); renderer_vertex( -szx,  szy,  szx );
		renderer_set_texcoord( 0.0, 1.0+dls); renderer_vertex( -szx,  0.0,  szx );
		renderer_set_texcoord( 1.0, 1.0+dls); renderer_vertex(  szx,  0.0,  szx );

		renderer_set_texcoord( 0.0,     dls); renderer_vertex(  szx,  szy, -szx );
		renderer_set_texcoord( 0.0, 1.0+dls); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 1.0, 1.0+dls); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 1.0,     dls); renderer_vertex( -szx,  szy, -szx );

		renderer_set_texcoord( 1.0,     dls); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 1.0, 1.0+dls); renderer_vertex(  szx,  0.0,  szx );
		renderer_set_texcoord( 0.0, 1.0+dls); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 0.0,     dls); renderer_vertex(  szx,  szy, -szx );

		renderer_set_texcoord( 0.0,     dls); renderer_vertex( -szx,  szy,  szx );
		renderer_set_texcoord( 1.0,     dls); renderer_vertex( -szx,  szy, -szx );
		renderer_set_texcoord( 1.0, 1.0+dls); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 0.0, 1.0+dls); renderer_vertex( -szx,  0.0,  szx );

		if(damage)
		{
			renderer_set_texcoord( 1.0, 1.0);
			renderer_set_color(1.0+dc, 0.0+dc, 0.0+dc, 0.5);
			renderer_vertex(  szx,  0.0,  szx );
			renderer_vertex(  szx,  0.0, -szx );
			renderer_vertex( -szx,  0.0, -szx );
			renderer_vertex( -szx,  0.0,  szx );
		}
		renderer_end();

		renderer_rotate(-rot2, 0.0, 1.0, 0.0);
		renderer_set_color(1.0+dc, 0.0+dc, 0.0+dc, 0.6+damageAlpha);
		renderer_begin(DRAW_QUADS);
		szx = 0.4;
		renderer_set_texcoord( 1.0,     dl); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 0.0,     dl); renderer_vertex( -szx,  szy,  szx );
		renderer_set_texcoord( 0.0, 1.0+dl); renderer_vertex( -szx,  0.0,  szx );
		renderer_set_texcoord( 1.0, 1.0+dl); renderer_vertex(  szx,  0.0,  szx );

		renderer_set_texcoord( 0.0,     dl); renderer_vertex(  szx,  szy, -szx );
		renderer_set_texcoord( 0.0, 1.0+dl); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 1.0, 1.0+dl); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 1.0,     dl); renderer_vertex( -szx,  szy, -szx );

		renderer_set_texcoord( 1.0,     dl); renderer_vertex(  szx,  szy,  szx );
		renderer_set_texcoord( 1.0, 1.0+dl); renderer_vertex(  szx,  0.0,  szx );
		renderer_set_texcoord( 0.0, 1.0+dl); renderer_vertex(  szx,  0.0, -szx );
		renderer_set_texcoord( 0.0,     dl); renderer_vertex(  szx,  szy, -szx );

		renderer_set_texcoord( 0.0,     dl); renderer_vertex( -szx,  szy,  szx );
		renderer_set_texcoord( 1.0,     dl); renderer_vertex( -szx,  szy, -szx );
		renderer_set_texcoord( 1.0, 1.0+dl); renderer_vertex( -szx,  0.0, -szx );
		renderer_set_texcoord( 0.0, 1.0+dl); renderer_vertex( -szx,  0.0,  szx );

		renderer_end();
		renderer_pop_matrix();
	}
	else
	{
		szx = 0.8;
		if( (sl < -0.7 && blink && shields > 0.0) || superShields )
			renderer_set_color_v(statClrWarn);
		else
			renderer_set_color(0.0+sc, 0.35+sc, 1.0+sc, 0.7+shieldAlpha);
		//-- Shields
		renderer_begin(DRAW_QUADS);
		renderer_set_texcoord( 1.0,     sl); renderer_vertex( statPosShld[0]    , statPosShld[1]+szy, statPosShld[2] );
		renderer_set_texcoord( 0.0,     sl); renderer_vertex( statPosShld[0]-szx, statPosShld[1]+szy, statPosShld[2] );
		renderer_set_texcoord( 0.0, 1.0+sl); renderer_vertex( statPosShld[0]-szx, statPosShld[1]    , statPosShld[2] );
		renderer_set_texcoord( 1.0, 1.0+sl); renderer_vertex( statPosShld[0]    , statPosShld[1]    , statPosShld[2] );
		//-- Life

		if( (dl < -0.7 && blink) )
			renderer_set_color_v(statClrWarn);
		else
			renderer_set_color(1.0+dc, 0.0+dc, 0.0+dc, 0.7+damageAlpha);
		renderer_set_texcoord( 1.0,     dl); renderer_vertex( -statPosShld[0]    , statPosShld[1]+szy, statPosShld[2] );
		renderer_set_texcoord( 1.0, 1.0+dl); renderer_vertex( -statPosShld[0]    , statPosShld[1]    , statPosShld[2] );
		renderer_set_texcoord( 0.0, 1.0+dl); renderer_vertex( -statPosShld[0]+szx, statPosShld[1]    , statPosShld[2] );
		renderer_set_texcoord( 0.0,     dl); renderer_vertex( -statPosShld[0]+szx, statPosShld[1]+szy, statPosShld[2] );
		renderer_end();
	}

	//-- print message if we're paused...
	if(game->game_pause)
	{
		float off[2];
		off[0] = 2.0 * sin(game->gameTime*0.5f);
		off[1] = 1.0 * cos(game->gameTime*0.55f);
		renderer_push_matrix();
		renderer_translate(-14.5, -3.0, 0.0);
		renderer_scale(0.21, 0.21, 1.0);
		renderer_push_matrix();
		renderer_set_color(1.0, 1.0, 1.0, 0.10*fabs(sin(game->gameTime*2.5f)) );
		game->text->Render(_("p a u s e d"));
		renderer_pop_matrix();
		renderer_set_color(1.0, 1.0, 1.0, 0.10*fabs(sin(game->gameTime*1.5f)) );
		renderer_translate(off[0], off[1], 0.0);
		game->text->Render(_("p a u s e d"));
		renderer_pop_matrix();
	}
	if( game->tipShipPast == 1 && game->gameLevel == 1)
	{
		game->tipShipPast++;
		tipShipShow = 200;
	}
	if( game->tipSuperShield == 1 && game->gameLevel == 1)
	{
		game->tipSuperShield++;
		tipSuperShow = 200;
	}
	if(	tipShipShow > 0 )
	{
		tipShipShow -= game->speedAdj;
		renderer_push_matrix();
		renderer_translate(-16, 13.0, 0.0);
		renderer_scale(0.035, 0.035, 1.0);
		renderer_set_color(1.0, 1.0, 1.0, tipShipShow/300.0 );
		const char *str = _("do not let -any- ships past you! each one costs you a life!");
		game->text->Render(str);
		renderer_pop_matrix();
	}
	if(	tipSuperShow > 0 )
	{
		tipSuperShow -= game->speedAdj;
		renderer_push_matrix();
		renderer_translate(-16, 13.0, 0.0);
		renderer_scale(0.035, 0.035, 1.0);
		renderer_set_color(1.0, 1.0, 1.0, tipSuperShow/300.0 );
		const char *str = _("let super shields pass by for an extra life!");
		game->text->Render(str);
		renderer_pop_matrix();
	}
}
