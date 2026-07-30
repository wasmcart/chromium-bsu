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

#include "GroundMetalSegment.h"

#include <cmath>

#include "compatibility.h"

#if defined(HAVE_APPLE_OPENGL_FRAMEWORK) || defined(HAVE_OPENGL_GL_H)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
//#define GL_EXT_
//#include <GL/glext.h>

#include "Config.h"
#include "Global.h"
#include "Ground.h"

//====================================================================
GroundMetalSegment::GroundMetalSegment(float p[3], float s[2], Ground *prnt)
	: GroundSegment(p, s, prnt)
{
}

GroundMetalSegment::~GroundMetalSegment()
{
}

//----------------------------------------------------------
void GroundMetalSegment::drawGL()
{
	Config *config = Config::instance();
	static float c0_clr[4] = { 0.65, 0.62, 0.53, 1.0 };
	static float c1_clr[4] = { 0.79, 0.82, 0.69, 1.0 };
	static float r0_clr[4]  = { 0.07, 0.07, 0.13, 1.0 };
	static float r1_clr[4]  = { 0.31, 0.30, 0.30, 1.0 };
	static float r2_clr[4]  = { 0.31, 0.30, 0.30, 1.0 };
	float S;
	float tmp;
	float rep;
	float tilt;
	float clr_sin;
	bool blipMirrorT = false;

	age += parent->game->speedAdj;

	clr_sin = 0.5*sin(parent->game->gameTime*0.05f);
	r1_clr[0] = 0.15+clr_sin;
	r2_clr[0] = 0.15+clr_sin;

	clr_sin = 0.2*sin(parent->game->gameTime*0.025f);
	c0_clr[0] = 0.28+clr_sin;
	c0_clr[1] = 0.25+clr_sin;
	c0_clr[2] = 0.16+clr_sin;
	c1_clr[0] = 0.42+clr_sin;
	c1_clr[1] = 0.45+clr_sin;
	c1_clr[2] = 0.34+clr_sin;

	switch(parent->variation)
	{
		default:
		case 0:
			rep = 0.26;
			tilt = 0.1;
			S = -parent->game->gameTime*0.05f;
			blipMirrorT = false;
			break;
		case 1:
			rep = 0.4;
			tilt = 0.2;
			S = -parent->game->gameTime*0.05f;
			blipMirrorT = true;
			break;
		case 2:
			S = -parent->game->gameTime*0.25f;
			tmp = sin(S);
			rep = 0.7+tmp;
			tilt = 0.5+tmp;
			blipMirrorT = true;
			break;
	}

#ifdef EXPERIMENTAL
	drawMultiTex(rep, S, tilt, blipMirrorT,
					c0_clr,
					c1_clr,
					r0_clr,
					r1_clr,
					r2_clr);
#else // EXPERIMENTAL
	if(config->gfxLevel() > 1)
	{
		drawBlip(rep, S, tilt, blipMirrorT);
	}
	if(true)
	{
		drawSurface(c0_clr,
					c1_clr,
					r0_clr,
					r1_clr,
					r2_clr);
	}
#endif // EXPERIMENTAL

}

//----------------------------------------------------------
void GroundMetalSegment::drawBlip(float rep, float S, float tilt, bool blipMirrorT)
{
	float repA = 0.0;
	float repB = rep;
	renderer_set_color(1.0, 1.0, 1.0, 1.0);
	renderer_bind_texture( parent->tex[Ground::Blip]);
//	renderer_begin(DRAW_QUADS);
	renderer_begin(DRAW_TRIANGLES);
		renderer_set_texcoord( rep,  repA+S+tilt); renderer_vertex(         pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repA+S);      renderer_vertex(-size[0]+pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repB+S);      renderer_vertex(-size[0]+pos[0], -size[1]+pos[1], pos[2]);

		renderer_set_texcoord( rep,  repA+S+tilt); renderer_vertex(         pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repB+S);      renderer_vertex(-size[0]+pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  repB+S+tilt); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);
		//--
		renderer_set_texcoord( 0.0,  repA+S);      renderer_vertex( size[0]+pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  repA+S+tilt); renderer_vertex(         pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  repB+S+tilt); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);

		renderer_set_texcoord( 0.0,  repA+S);      renderer_vertex( size[0]+pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  repB+S+tilt); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repB+S);      renderer_vertex( size[0]+pos[0], -size[1]+pos[1], pos[2]);

		if(blipMirrorT) { repA = rep; repB = 0.0; }

		renderer_set_texcoord( 0.0,  repA+S);      renderer_vertex( size[0]+pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  repA+S+tilt); renderer_vertex(	      pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  repB+S+tilt); renderer_vertex(	      pos[0],	       pos[1], pos[2]);

		renderer_set_texcoord( 0.0,  repA+S);      renderer_vertex( size[0]+pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  repB+S+tilt); renderer_vertex(	      pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repB+S);      renderer_vertex( size[0]+pos[0],	       pos[1], pos[2]);
		//--
		renderer_set_texcoord( rep,  repA+S+tilt); renderer_vertex(	      pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repA+S);      renderer_vertex(-size[0]+pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repB+S);      renderer_vertex(-size[0]+pos[0],	       pos[1], pos[2]);

		renderer_set_texcoord( rep,  repA+S+tilt); renderer_vertex(	      pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  repB+S);      renderer_vertex(-size[0]+pos[0],	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  repB+S+tilt); renderer_vertex(         pos[0],	       pos[1], pos[2]);
	renderer_end();
}

//----------------------------------------------------------
void GroundMetalSegment::drawSurface(float *c0_clr, float *c1_clr,
									 float *r0_clr, float *r1_clr, float *r2_clr)
{
	Config *config = Config::instance();
	float rep = 1.0;
	renderer_bind_texture( parent->tex[Ground::Base]);
	renderer_begin(DRAW_TRIANGLES); //-- use triangles to prevent color popping on Utah
	if(config->gfxLevel() > 0)
	{
		renderer_set_color_v(c0_clr); renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_color_v(c1_clr); renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],          pos[1], pos[2]);
		renderer_set_color_v(r2_clr); renderer_set_texcoord( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],          pos[1], pos[2]);

		renderer_set_color_v(c0_clr); renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_color_v(r2_clr); renderer_set_texcoord( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],          pos[1], pos[2]);
		renderer_set_color_v(r0_clr); renderer_set_texcoord( 0.0,  rep); renderer_vertex(-size[0]+pos[0], -size[1]+pos[1], pos[2]);

		renderer_set_color_v(r1_clr); renderer_set_texcoord( 0.0,  rep); renderer_vertex( size[0]+pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_color_v(r0_clr); renderer_set_texcoord( 0.0,  0.0); renderer_vertex( size[0]+pos[0],          pos[1], pos[2]);
		renderer_set_color_v(c1_clr); renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],          pos[1], pos[2]);

		renderer_set_color_v(r1_clr); renderer_set_texcoord( 0.0,  rep); renderer_vertex( size[0]+pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_color_v(c1_clr); renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],          pos[1], pos[2]);
		renderer_set_color_v(c0_clr); renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);

		renderer_set_color_v(r0_clr); renderer_set_texcoord( 0.0,  0.0); renderer_vertex( size[0]+pos[0],          pos[1], pos[2]);
		renderer_set_color_v(r1_clr); renderer_set_texcoord( 0.0,  rep); renderer_vertex( size[0]+pos[0],  size[1]+pos[1]+0.1, pos[2]);
		renderer_set_color_v(c0_clr); renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0],  size[1]+pos[1]+0.1, pos[2]);

		renderer_set_color_v(r0_clr); renderer_set_texcoord( 0.0,  0.0); renderer_vertex( size[0]+pos[0],          pos[1], pos[2]);
		renderer_set_color_v(c0_clr); renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0],  size[1]+pos[1]+0.1, pos[2]);
		renderer_set_color_v(c1_clr); renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],          pos[1], pos[2]);

		renderer_set_color_v(c1_clr); renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],          pos[1], pos[2]);
		renderer_set_color_v(c0_clr); renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0],  size[1]+pos[1]+0.1, pos[2]);
		renderer_set_color_v(r0_clr); renderer_set_texcoord( 0.0,  rep); renderer_vertex(-size[0]+pos[0],  size[1]+pos[1]+0.1, pos[2]);

		renderer_set_color_v(c1_clr); renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],          pos[1], pos[2]);
		renderer_set_color_v(r0_clr); renderer_set_texcoord( 0.0,  rep); renderer_vertex(-size[0]+pos[0],  size[1]+pos[1]+0.1, pos[2]);
		renderer_set_color_v(r2_clr); renderer_set_texcoord( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],          pos[1], pos[2]);
	}
	else
	{
		float b = -0.1;
		renderer_set_color(c0_clr[0]-b, c0_clr[1]-b, c0_clr[2]-b, c0_clr[3] );
		renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0], 		   pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],  	       pos[1], pos[2]);

		renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],          pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  rep); renderer_vertex(-size[0]+pos[0], -size[1]+pos[1], pos[2]);

		renderer_set_texcoord( 0.0,  rep); renderer_vertex( size[0]+pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  0.0); renderer_vertex( size[0]+pos[0],  	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],  	       pos[1], pos[2]);

		renderer_set_texcoord( 0.0,  rep); renderer_vertex( size[0]+pos[0], -size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],  	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0], -size[1]+pos[1], pos[2]);

		renderer_set_texcoord( 0.0,  0.0); renderer_vertex( size[0]+pos[0],          pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  rep); renderer_vertex( size[0]+pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0],  size[1]+pos[1], pos[2]);

		renderer_set_texcoord( 0.0,  0.0); renderer_vertex( size[0]+pos[0],  	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],  	       pos[1], pos[2]);

		renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],  	       pos[1], pos[2]);
		renderer_set_texcoord( rep,  rep); renderer_vertex(         pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  rep); renderer_vertex(-size[0]+pos[0],  size[1]+pos[1], pos[2]);

		renderer_set_texcoord( rep,  0.0); renderer_vertex(         pos[0],  	       pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  rep); renderer_vertex(-size[0]+pos[0],  size[1]+pos[1], pos[2]);
		renderer_set_texcoord( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],  	       pos[1], pos[2]);
	}
	renderer_end();
}

#ifdef EXPERIMENTAL

#define glTexCoord2f_M0(a, b) glMultiTexCoord2fARB(GL_TEXTURE0_ARB, a, b)
#define glTexCoord2f_M1(a, b) glMultiTexCoord2fARB(GL_TEXTURE1_ARB, a, b)

//--------------------------------------------------------------------
void GroundMetalSegment::drawMultiTex(float rep, float S, float tilt, bool blipMirrorT,
						float *c0_clr,
						float *c1_clr,
						float *r0_clr,
						float *r1_clr,
						float *r2_clr)
{
	float repA = 0.0;
	float repB = rep;

	glActiveTextureARB(GL_TEXTURE0_ARB);
	renderer_bind_texture( parent->tex[Ground::Blip]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);


	glActiveTextureARB(GL_TEXTURE1_ARB);
	renderer_bind_texture( parent->tex[Ground::Base]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	renderer_enable(GL_TEXTURE_2D);

//	renderer_set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderer_begin(DRAW_TRIANGLES); //-- use triangles to prevent color popping on Utah

		glTexCoord2f_M0( rep,  repA+S+tilt); renderer_set_color_v(c1_clr); glTexCoord2f_M1( 1.0,  0.0); renderer_vertex(		  pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repA+S);      renderer_set_color_v(r2_clr); glTexCoord2f_M1( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repB+S);      renderer_set_color_v(r0_clr); glTexCoord2f_M1( 0.0,  1.0); renderer_vertex(-size[0]+pos[0], -size[1]+pos[1], pos[2]);

		glTexCoord2f_M0( rep,  repA+S+tilt); renderer_set_color_v(c1_clr); glTexCoord2f_M1( 1.0,  0.0); renderer_vertex(		  pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repB+S);      renderer_set_color_v(r0_clr); glTexCoord2f_M1( 0.0,  1.0); renderer_vertex(-size[0]+pos[0], -size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repB+S+tilt); renderer_set_color_v(c0_clr); glTexCoord2f_M1( 1.0,  1.0); renderer_vertex(		  pos[0], -size[1]+pos[1], pos[2]);

		glTexCoord2f_M0( 0.0,  repA+S);      renderer_set_color_v(r0_clr); glTexCoord2f_M1( 0.0,  0.0); renderer_vertex( size[0]+pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repA+S+tilt); renderer_set_color_v(c1_clr); glTexCoord2f_M1( 1.0,  0.0); renderer_vertex(		  pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repB+S+tilt); renderer_set_color_v(c0_clr); glTexCoord2f_M1( 1.0,  1.0); renderer_vertex(		  pos[0], -size[1]+pos[1], pos[2]);

		glTexCoord2f_M0( 0.0,  repA+S);      renderer_set_color_v(r0_clr); glTexCoord2f_M1( 0.0,  0.0); renderer_vertex( size[0]+pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repB+S+tilt); renderer_set_color_v(c0_clr); glTexCoord2f_M1( 1.0,  1.0); renderer_vertex(		  pos[0], -size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repB+S);      renderer_set_color_v(r1_clr); glTexCoord2f_M1( 0.0,  1.0); renderer_vertex( size[0]+pos[0], -size[1]+pos[1], pos[2]);

		if(blipMirrorT) { repA = rep; repB = 0.0; }

		glTexCoord2f_M0( 0.0,  repA+S);      renderer_set_color_v(r1_clr); glTexCoord2f_M1( 0.0,  1.0); renderer_vertex( size[0]+pos[0],  size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repA+S+tilt); renderer_set_color_v(c0_clr); glTexCoord2f_M1( 1.0,  1.0); renderer_vertex(		  pos[0],  size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repB+S+tilt); renderer_set_color_v(c1_clr); glTexCoord2f_M1( 1.0,  0.0); renderer_vertex(		  pos[0],		   pos[1], pos[2]);

		glTexCoord2f_M0( 0.0,  repA+S);      renderer_set_color_v(r1_clr); glTexCoord2f_M1( 0.0,  1.0); renderer_vertex( size[0]+pos[0],  size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repB+S+tilt); renderer_set_color_v(c1_clr); glTexCoord2f_M1( 1.0,  0.0); renderer_vertex(		  pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repB+S);      renderer_set_color_v(r0_clr); glTexCoord2f_M1( 0.0,  0.0); renderer_vertex( size[0]+pos[0],		   pos[1], pos[2]);
		//--
		glTexCoord2f_M0( rep,  repA+S+tilt); renderer_set_color_v(c0_clr); glTexCoord2f_M1( 1.0,  1.0); renderer_vertex(		  pos[0],  size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repA+S);      renderer_set_color_v(r0_clr); glTexCoord2f_M1( 0.0,  1.0); renderer_vertex(-size[0]+pos[0],  size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repB+S);      renderer_set_color_v(r2_clr); glTexCoord2f_M1( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],		   pos[1], pos[2]);

		glTexCoord2f_M0( rep,  repA+S+tilt); renderer_set_color_v(c0_clr); glTexCoord2f_M1( 1.0,  1.0); renderer_vertex(		  pos[0],  size[1]+pos[1], pos[2]);
		glTexCoord2f_M0( 0.0,  repB+S);      renderer_set_color_v(r2_clr); glTexCoord2f_M1( 0.0,  0.0); renderer_vertex(-size[0]+pos[0],		   pos[1], pos[2]);
		glTexCoord2f_M0( rep,  repB+S+tilt); renderer_set_color_v(c1_clr); glTexCoord2f_M1( 1.0,  0.0); renderer_vertex(		  pos[0],		   pos[1], pos[2]);

	renderer_end();

	renderer_disable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

}

#endif // EXPERIMENTAL
