#include "Renderer.h"
/*
 * Copyright (c) 2000 Mark B. Allan. All rights reserved.
 *
 * "Chromium B.S.U." is free software; you can redistribute
 * it and/or use it and/or modify it under the terms of the
 * "Clarified Artistic License"
 */
#ifndef StatusDisplay_h
#define StatusDisplay_h

#ifdef HAVE_CONFIG_H
#include <chromium-bsu-config.h>
#endif

#include "compatibility.h"

#if defined(HAVE_APPLE_OPENGL_FRAMEWORK) || defined(HAVE_OPENGL_GL_H)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "HeroAircraft.h"

class Global;

//====================================================================
class StatusDisplay
{
public:
	StatusDisplay();
	~StatusDisplay();

	void	darkenGL();
	void	drawGL(HeroAircraft	*hero);

	void	setAmmoAlpha(float in)	{ ammoAlpha = in; };
	void	setDamageAlpha(float in) { damageAlpha = in; };
	void    setShieldAlpha(float in) { shieldAlpha = in; };

	void	enemyWarning(float v) { if(v > enemyWarn) enemyWarn = v; }

	void	loadTextures();
	void	deleteTextures();

private:
	GLuint		statTex;
	GLuint		shldTex;
	GLuint		topTex;
	GLuint		heroSuperTex;
	GLuint		heroShieldTex;
	GLuint		heroAmmoFlash[NUM_HERO_AMMO_TYPES];
	GLuint		useFocus;
	GLuint		useItem[NUM_HERO_ITEMS];

	float		ammoAlpha;
	float		damageAlpha;
        float           shieldAlpha;
	bool		blink;

	inline void drawQuad(float szx, float szy)
	{
		renderer_begin(DRAW_TRIANGLE_STRIP);
			renderer_set_texcoord(1.0, 0.0); renderer_vertex( szx,  szy, 0.0);
			renderer_set_texcoord(0.0, 0.0); renderer_vertex(-szx,  szy, 0.0);
			renderer_set_texcoord(1.0, 1.0); renderer_vertex( szx, -szy, 0.0);
			renderer_set_texcoord(0.0, 1.0); renderer_vertex(-szx, -szy, 0.0);
		renderer_end();
	}

	float tipShipShow;
	float tipSuperShow;

	float enemyWarn;

private:
	Global	*game;
};

#endif // StatusDisplay_h

