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

#include "GroundSeaSegment.h"

#include <cmath>

#include "compatibility.h"

#if defined(HAVE_APPLE_OPENGL_FRAMEWORK) || defined(HAVE_OPENGL_GL_H)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
//#define GL_EXT_
//#include <GL/glext.h>

#include "define.h"
#include "Global.h"
#include "Ground.h"

//#undef FRAND
//#define FRAND 1.0
//====================================================================
GroundSeaSegment::GroundSeaSegment(float p[3], float s[2], Ground *prnt)
	: GroundSegment(p, s, prnt)
{
	float a = 1.0+(FRAND*0.5);
	vert[0][0] =   a * s[0];	a = 1.0+(FRAND*0.5);
	vert[1][0] = - a * s[0];	a = 1.0+(FRAND*0.5);
	vert[2][0] =   a * s[0];	a = 1.0+(FRAND*0.5);
	vert[3][0] = - a * s[0];	a = 1.0+(FRAND*0.5);
	vert[0][1] =   a * s[1];	a = 1.0+(FRAND*0.5);
	vert[1][1] =   a * s[1];	a = 1.0+(FRAND*0.5);
	vert[2][1] = - a * s[1];	a = 1.0+(FRAND*0.5);
	vert[3][1] = - a * s[1];	a = 1.0+(FRAND*0.5);
	vert[0][2] = 0.0;
	vert[1][2] = 0.0;
	vert[2][2] = 0.0;
	vert[3][2] = 0.0;
}

GroundSeaSegment::~GroundSeaSegment()
{
}

//----------------------------------------------------------
void GroundSeaSegment::drawGL()
{
	renderer_push_matrix();
	renderer_translate(pos[0], pos[1], pos[2]);
		renderer_bind_texture( parent->tex[Ground::Base]);
		renderer_begin(DRAW_TRIANGLE_STRIP);
			renderer_set_texcoord( 1.0, 0.0); renderer_vertex_v(vert[0]);
			renderer_set_texcoord( 0.0, 0.0); renderer_vertex_v(vert[1]);
			renderer_set_texcoord( 1.0, 1.0); renderer_vertex_v(vert[2]);
			renderer_set_texcoord( 0.0, 1.0); renderer_vertex_v(vert[3]);
		renderer_end();
	renderer_pop_matrix();
}
