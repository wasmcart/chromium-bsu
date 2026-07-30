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

#include "MenuGL.h"

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

#include "extern.h"
#include "define.h"
#include "Global.h"
#include "HiScore.h"
#include "Ground.h"
#include "Audio.h"
#include "Image.h"

#include "textGeometry.h"

static const int NumMssg = 7;
static char mssgHelpText[NumMssg][128] = {
N_("  d o   n o t   a l l o w  -a n y-   e n e m i e s   g e t   p a s t   y o u !"),
N_("  e v e r y   e n e m y   t h a t   g e t s   b y   c o s t s   y o u   a   l i f e !"),
N_("  a l l o w   p o w e r - u p s   t o   p a s s   b y   f o r   b i g   p o i n t s !"),
N_("  c r a s h   i n t o   e n e m i e s   t o   d e s t r o y   t h e m !"),
N_("  r i g h t   c l i c k   t w i c e   t o   s e l f - d e s t r u c t !"),
N_("  s e l f - d e s t r u c t   t o   p r e s e r v e   y o u r   a m m u n i t i o n !"),
N_("  d o w n l o a d   C h r o m i u m   B. S. U.   a t   http://chromium-bsu.sf.net/"),
};

//====================================================================
MenuGL::MenuGL()
{
	game = Global::getInstance();

	curSel = NewGame;

	elecStretch = 10.0;
	elecOffX = 0.0;
	elecOffY = 0.0;
	textAngle = 0.0;
	txtHeight = 0.5;
	titleTilt = -10.0;

	butHeight	= 0.5;
	butWidth	= butHeight*4.0;
	butOffset	= 3.05;

	menuText[NewGame]	= _("n e w    g a m e");
	menuText[GameLevel]	= _("l e v e l");
	menuText[SkillLevel]= _("s k i l l");
	menuText[Graphics]	= _("g f x    d e t a i l");
	menuText[FullScreen]= _("f u l l s c r e e n");
	menuText[ScreenSize]= _("s c r e e n    s i z e");
	menuText[Sound]		= _("s o u n d    f x    v o l u m e");
	menuText[Music]		= _("m u s i c    v o l u m e");
	menuText[MovementSpeed]= _("m o v e m e n t   s p e e d");
	menuText[Quit]		= _("q u i t");

	loadTextures();

	thickText = true;

	mssgAlpha = 0.0;
	sprintf(mssgText, " ");
	mssgIndex = 0;
	mssgCount = 0;
	mssgHelpOverride = false;

	startMenu();
}

//----------------------------------------------------------
MenuGL::~MenuGL()
{
	deleteTextures();
}

//----------------------------------------------------------
void MenuGL::loadTextures()
{
	elecTex   = Image::load(dataLoc("png/electric.png"), IMG_NOMIPMAPS, IMG_BLEND3, GL_CLAMP, GL_LINEAR, GL_LINEAR);
	backTex   = Image::load(dataLoc("png/menu_back.png"), IMG_NOMIPMAPS, IMG_SOLID, GL_REPEAT, GL_LINEAR, GL_LINEAR);
//	csrTex    = Image::load(dataLoc("png/cursor.png"));
	csrTex    = Image::load(dataLoc("png/heroAmmoFlash00.png"));
	updwnTex  = Image::load(dataLoc("png/menu_updown.png"));
	//-- Environment map
	renderer_tex_geni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	renderer_tex_geni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	envTex = Image::load(dataLoc("png/reflect.png"), IMG_BUILDMIPMAPS, IMG_SOLID, GL_CLAMP, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR);

	thickText = true;
}

//----------------------------------------------------------
void MenuGL::deleteTextures()
{
	renderer_delete_textures(1, &elecTex);
	renderer_delete_textures(1, &backTex);
	renderer_delete_textures(1, &envTex);
	renderer_delete_textures(1, &csrTex);
	renderer_delete_textures(1, &updwnTex);
	elecTex	= 0;
	backTex	= 0;
	envTex	= 0;
	csrTex	= 0;
	updwnTex = 0;

	/* display lists removed — title text rendered directly */
}

//----------------------------------------------------------
void MenuGL::createLists(bool thick)
{
	thickText = thick;
	titleTilt = -10.0;
}

static const char *skillString(int i)
{
	switch(i)
	{
		case 2: return _("fish in a barrel");
		case 3: return _("wimp");
		case 4: return _("easy");
		case 5: return _("normal");
		case 6: return _("experienced");
		case 7: return _("fun");
		case 8: return _("insane");
		case 9: return _("impossible");
		default:return _("-");
	}
}

//----------------------------------------------------------
void MenuGL::startMenu()
{
//	elecOffX	= elecStretch*0.4;
	elecOffX	= 0.0;
	textAngle	= 90.0;
	textCount	= 500;
	createLists( (thickText = true) );
	Global::cursorPos[0] = 0.0;
	Global::cursorPos[1] = 0.0;

}

//----------------------------------------------------------
void MenuGL::drawGL()
{
	Config	*config = Config::instance();
	Global	*game = Global::getInstance();
	HiScore	*hiScore = HiScore::getInstance();
	int		i;

	textCount -= game->speedAdj; if(textCount < 0)
	{
		textCount = 500;
//		textAngle = 360.0;
	}
	if(textAngle > 0.0)
		textAngle -= 5.0*game->speedAdj;
	else
		textAngle  =  0.0;

//	renderer_set_clear_color(0.27451, 0.235294, 0.392157, 1.0);
	//-- Clear buffers
	renderer_clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//-- Place camera
	renderer_load_identity();
	renderer_translate(0.0, 0.0, config->zTrans());

	renderer_set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderer_set_color(1.0, 1.0, 1.0, 1.0);

	//-- Draw background
	game->ground->drawGL();

	//-- Update audio
	game->audio->update();

	float	szx		=   9.0;
	float	szy		=   4.5;
	float	top		=   1.0;
	float	left	=  -8.0;
	float	inc		=  -txtHeight*2.5;

	//----- Draw credits texture --------------------------------
	renderer_push_matrix();
		// NOTE: corners of back tex is white, alpha 1 and
		// we are in modulate blend...
		renderer_bind_texture( backTex);
		renderer_set_texcoord(1.0, 0.0);

		//-- darken
		renderer_begin(DRAW_QUADS);
		renderer_set_color(0.0, 0.0, 0.0, 0.8);
			renderer_vertex( szx,  szy+0.25-3.0, 10.0);
			renderer_vertex(-szx,  szy+0.25-3.0, 10.0);
		renderer_set_color(0.0, 0.0, 0.0, 0.4);
			renderer_vertex(-szx, -13.0, 10.0);
			renderer_vertex( szx, -13.0, 10.0);
		renderer_end();

		renderer_begin(DRAW_QUADS);
		renderer_set_color(0.0, 0.0, 0.0, mssgAlpha);
			renderer_vertex( 16.0,  -10.7, 10.0);
			renderer_vertex(-16.0,  -10.7, 10.0);
		renderer_set_color(0.0, 0.0, 0.0, mssgAlpha);
			renderer_vertex(-16.0, -11.9, 10.0);
			renderer_vertex( 16.0, -11.9, 10.0);
		renderer_end();

		szx = 12.0;
		szy =  txtHeight*0.5;
		renderer_push_matrix();
		renderer_translate(left, top+(inc*curSel), 10.0);
		renderer_begin(DRAW_QUADS);
			renderer_set_color(0.5, 0.5, 1.0, 1.0);
			renderer_vertex(-szx,  szy*3.0, 0.0);
			renderer_vertex(-szx, -szy, 0.0);
			renderer_set_color(0.2, 0.2, 1.0, 0.0);
			renderer_vertex( szx, -szy, 0.0);
			renderer_vertex( szx,  szy*3.0, 0.0);
		renderer_end();
		drawIndicator();

		renderer_set_blend_func(GL_SRC_ALPHA, GL_ONE);
		drawElectric();
		renderer_pop_matrix();
		renderer_set_color(1.0, 1.0, 1.0, 0.9);
		float sc = 0.035;
		for(i = 0; i < NumSelections; i++)
		{
			renderer_push_matrix();
			renderer_translate(left, top+(inc*i), 10.0);
			renderer_rotate(textAngle, 1.0, 0.0, 0.0);
			renderer_scale(sc, sc*0.75, 1.0);
			game->text->Render(menuText[i]);
			renderer_pop_matrix();
		}

		{
			float f = game->gameTime * -50.0f;
			float r = cos(f*0.02);
			renderer_push_matrix();
//			renderer_set_color(1.0+r, r, r, 0.6+0.2*r);
			renderer_set_color(1.0, 1.0, 1.0, 0.6+0.2*r);
			renderer_translate(-18.75, -8.5, 0.0);
			renderer_scale(sc, sc*0.75, 1.0);
			game->text->Render(_("high scores"));
			renderer_translate(-100.0, -30.0, 0.0);
			char buf[16];
			int i;
			float trans;
			time_t nowTime = time(NULL);
			int l = config->intSkill();
			int recentHiScore = -1;
			time_t mostRecent = 0;
			for(i = 0; i < HI_SCORE_HIST; i++)
			{
				if(	hiScore->getDate(l, i) > nowTime-300 && // highlight score for 5 minutes (300)
					hiScore->getDate(l, i) > mostRecent )
				{
					recentHiScore = i;
					mostRecent = hiScore->getDate(l, i);
				}
			}
			for(i = 0; i < HI_SCORE_HIST; i++)
			{
				f += 30.0;
				r = cos(f*0.02);
				if(i == recentHiScore)
					renderer_set_color(1.5, 0.5, 0.5, 0.6+0.1*r);
				else
					renderer_set_color(1.0, 1.0, 1.0, 0.2+0.2*r);
//				renderer_set_color(0.5+r*0.5, 0.5, 0.25-r*0.25, 0.2+0.2*r);
				sprintf(buf, "%d", (int)hiScore->getScore(config->intSkill(), i) );
				trans = 80.0 + game->text->Advance(_("high scores")) - game->text->Advance(buf);
				renderer_translate( trans, 0.0, 0.0 );
				game->text->Render(buf);
				renderer_translate( -trans, -30.0, 0.0) ;
			}
			renderer_pop_matrix();
		}

		//---- credits
		if(true)
		{
			int		 n = 0;
			float	alpha;
			static double c = 0.0;
			if(c > 75.0)
			{
				c = 0.0;
			}
			c += 0.2;
			renderer_push_matrix();
			if(c > 25)	alpha = 0.4*(75.0-c)/50.0;
			else		alpha = 0.4;
			renderer_set_color(1.0, 1.0, 1.0, alpha);
			sc = 0.03;
			renderer_translate(14.0, -11.5, 0.0);
			renderer_scale(sc, sc, 1.0);
			renderer_translate(-c*1.5, c, 0.0);
			if(c < 3)	n = (int)c;
			else		n = 3;
			if(n>0) game->text->Render(_("the"), n);
			renderer_translate(c, -38+c, 0.0);
			if(c < 10)	n = (int)(c-3);
			else		n = 7;
			if(n>0) game->text->Render(_("reptile"), n);
			renderer_translate(c, -38+c, 0.0);
			if(c < 16)	n = (int)c-10;
			else		n = 6;
			if(n>0) game->text->Render(_("labour"), n);
			renderer_translate(c, -38+c, 0.0);
			if(c < 23)	n = (int)(c-16);
			else		n = 7;
			if(n>0) game->text->Render(_("project"), n);
			// font height is 23
			renderer_pop_matrix();
		}

		//-------- draw help message
		if(mssgAlpha > 0.0)
		{
			if(mssgHelpOverride)
				renderer_set_color(1.3, mssgAlpha, mssgAlpha, mssgAlpha);
			else
				renderer_set_color(0.5, 0.5, 0.9, (0.2+mssgAlpha));
			sc = 0.042;
			renderer_translate(-19.5, -14.0, 0.0);
			renderer_scale(sc, sc*0.75, 1.0);
			size_t len = mbstowcs(NULL,mssgText,0);
			unsigned int	ti = (unsigned int)(112.0*mssgAlpha);
			if(ti > len) ti = len;
			if(ti) game->text->Render(mssgText, ti);
			mssgAlpha -= 0.004*game->speedAdj;
			renderer_set_color(1.0, 1.0, 1.0, 1.0);
		}



	renderer_pop_matrix();


	renderer_set_blend_func(GL_SRC_ALPHA, GL_ONE);
	renderer_push_matrix();
	renderer_translate(0.0, 4.75, 25.0);
	renderer_set_color(1.0, 1.0, 1.0, 1.0);
	renderer_set_depth_mask(GL_FALSE);	//XXX Hack to make Voodoo3 XF4 work
	drawTitleBack();
	renderer_set_depth_mask(GL_TRUE);	//XXX Hack to make Voodoo3 XF4 work
	drawTitle();
	renderer_pop_matrix();

//	//-- draw cursor...
//	{
//		float x = Global::cursorPos[0]*16.60;
//		float y = Global::cursorPos[1]*12.45;
//		float z = 10.0;
//		float sz;
//		renderer_bind_texture( csrTex);
//		sz = 0.2;
//		renderer_begin(DRAW_QUADS);
//		renderer_set_texcoord(1.0, 1.0); renderer_vertex( x+sz, y+sz, z);
//		renderer_set_texcoord(0.0, 1.0); renderer_vertex( x-sz, y+sz, z);
//		renderer_set_texcoord(0.0, 0.0); renderer_vertex( x-sz, y-sz, z);
//		renderer_set_texcoord(1.0, 0.0); renderer_vertex( x+sz, y-sz, z);
//		renderer_end();
//	}

	if(thickText && game->fps < 30)
	{
		if( config->debug() ) fprintf(stderr, _("ATTENTION: Using 'thin' text to improve framerate...\n"));
		createLists( (thickText = false) );
	}
	if(!thickText && game->fps > 40)
	{
		if( config->debug() ) fprintf(stderr, _("ATTENTION: Reverting to 'thick' text...\n"));
		createLists( (thickText = true) );
	}

	//---------- Help messages
	if(mssgHelpOverride && mssgAlpha < 0.05)
	{
		mssgHelpOverride = false;
		mssgCount = 0;
	}
	if(!mssgHelpOverride)
	{
		int interval = (mssgCount++)%500;
		if(!interval)
		{
			strcpy(mssgText, gettext(mssgHelpText[mssgIndex%NumMssg]));
			mssgIndex++;
		}
		if(interval < 150)
		{
			mssgAlpha = interval/150.0;
		}
	}
}

//----------------------------------------------------------
void MenuGL::drawIndicator()
{
	Config	*config = Config::instance();
	Global	*game = Global::getInstance();
	char	buf[64];
	float	szx = 10.0;
	float	szy = txtHeight;
	float	level = 0.0;
	float	sc = 0.025;
	int		tmp;
	switch(curSel)
	{
		case GameLevel:
			level = game->gameLevel/9.0;
			sprintf(buf, "%d", game->gameLevel);
			break;
		case SkillLevel:
			level = config->gameSkillBase();
			tmp = (int)((level+0.05)*10.0);
			sprintf(buf, "%s", skillString(tmp));
			break;
		case Graphics:
			level = config->gfxLevel()/2.0;
			switch(config->gfxLevel())
			{
				case 0: sprintf(buf, _("low")); break;
				case 1: sprintf(buf, _("med")); break;
				case 2: sprintf(buf, _("high")); break;
			}
			break;
		case ScreenSize:
			level = (float)config->approxScreenSize()/(float)MAX_SCREEN_SIZE;
			sprintf(buf, _("%dx%d"), config->screenW(), config->screenH());
			break;
		case FullScreen:
			level = (float)config->fullScreen();
			if(config->fullScreen()) sprintf(buf, _("true"));
			else sprintf(buf, _("false"));
			break;
		case Sound:
			level = config->volSound();
			sprintf(buf, "%d", (int)((level+0.05)*10.0));
			break;
		case Music:
			level = config->volMusic();
			sprintf(buf, "%d", (int)((level+0.05)*10.0));
			break;
		case MovementSpeed:
			level = config->movementSpeed()*10.0;
			sprintf(buf, "%d", (int)((level+0.005)*100.0));
			break;
		default:
			level = -5.0;
			break;
	}
	renderer_push_matrix();
		renderer_translate(0.0, -txtHeight, 0.0);
		renderer_begin(DRAW_QUADS);
			renderer_set_color(1.0, 1.0, 1.0, 0.3);
			renderer_vertex(szx+szy, szy, 0.0);
			renderer_vertex(   -szx, szy, 0.0);
			renderer_set_color(1.0, 1.0, 1.0, 0.15);
			renderer_vertex(   -szx, 0.0, 0.0);
			renderer_vertex(    szx, 0.0, 0.0);
		renderer_end();

		//-- draw level indicator and/or text
		renderer_push_matrix();
		if(level > -1.0)
		{
			renderer_begin(DRAW_QUADS);
				renderer_set_color(1.0, 0.0, 0.0, 1.0);
				renderer_vertex(szy+szx*level, szy, 0.0);
				renderer_vertex(          0.0, szy, 0.0);
				renderer_set_color(0.0, 0.0, 0.0, 0.0);
				renderer_vertex(          0.0, 0.0, 0.0);
				renderer_vertex(    szx*level, 0.0, 0.0);
			renderer_end();

			//-- draw +/- buttons ---
			float	bx = butWidth;
			float	by = butHeight;
			float	bo = butOffset;
			renderer_bind_texture( updwnTex);
			renderer_begin(DRAW_QUADS);
				renderer_set_color(1.0, 1.0, 1.0, 0.6);
				renderer_set_texcoord(1.0, 0.0);	renderer_vertex(	 bx-bo,  by, 0.0);
				renderer_set_texcoord(0.0, 0.0);	renderer_vertex(	0.0-bo,  by, 0.0);
				renderer_set_texcoord(0.0, 1.0);	renderer_vertex(	0.0-bo, 0.0, 0.0);
				renderer_set_texcoord(1.0, 1.0);	renderer_vertex(  bx-bo, 0.0, 0.0);
			renderer_end();

			renderer_set_color(1.0, 1.0, 1.0, 0.5);
			renderer_translate(11.0, 0.0, 0.0);
			renderer_scale(sc, sc, 1.0);
			game->text->Render(buf);
		}
		renderer_pop_matrix();

	renderer_pop_matrix();

}

//----------------------------------------------------------
void MenuGL::drawElectric()
{
	float es = elecStretch;
	elecOffY = FRAND*0.7;
	if( (elecOffX+=0.175) > es)
		elecOffX = -5.0;
	float szx = 30.0;
	float szy = txtHeight;
	renderer_bind_texture( elecTex);
	renderer_push_matrix();
	renderer_translate(0.0, txtHeight*0.5, 0.0);
	renderer_begin(DRAW_QUADS);
		renderer_set_color(1.0, 1.0, 1.0, 1.0);
		renderer_set_texcoord(   elecOffX, elecOffY+0.3); renderer_vertex( -8.0,  szy, 0.0);
		renderer_set_texcoord(   elecOffX,     elecOffY); renderer_vertex( -8.0, -szy, 0.0);
		renderer_set_color(0.0, 0.0, 0.1, 1.0);
		renderer_set_texcoord(elecOffX-es,     elecOffY); renderer_vertex(  szx, -szy, 0.0);
		renderer_set_texcoord(elecOffX-es, elecOffY+0.3); renderer_vertex(  szx,  szy, 0.0);
	renderer_end();
	renderer_pop_matrix();
}

//----------------------------------------------------------
void MenuGL::drawTitleBack()
{
	float	clr_c[4] = { 1.0, 1.0, 1.0, 0.0 };
	float	clr_w[4] = { 1.0, 1.0, 1.0, 1.0 };
	float	w = 9.0;
	float	a = 2.0;
	float	h = 1.4;
	float	z = 0.5;

	float	as = a/(w+a);
	float	at = a/(h+a);

	renderer_bind_texture( backTex);
	renderer_matrix_mode(GL_TEXTURE);
	renderer_push_matrix();
	static float amt = 0.0;
	renderer_translate(amt*0.1, amt*0.5, 0.0);
	renderer_rotate(-amt*100.0, 0.0, 1.0, 1.0);
	amt -= 0.01*game->speedAdj;
	//-- Top right
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex(  w+a,  h  , 0.0);
		renderer_set_texcoord(1.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  w+a,  h+a, 0.0);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  w  ,  h  , 0.0);
		renderer_set_texcoord(1.0-as, 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  w  ,  h+a, 0.0);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0,  h  , 0.0);
		renderer_set_texcoord(0.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  0.0,  h+a, 0.0);
	renderer_end();
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(1.0   , 0.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  w+a, 0.0, z);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex(  w+a,	h, 0.0);
		renderer_set_texcoord(1.0-as, 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex(  w  , 0.0, z);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  w  ,	h, 0.0);
		renderer_set_texcoord(0.0   , 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex(  0.0, 0.0, z);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0,	h, 0.0);
	renderer_end();
	//-- Top left
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0,  h  , 0.0);
		renderer_set_texcoord(0.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  0.0,  h+a, 0.0);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex( -w  ,  h  , 0.0);
		renderer_set_texcoord(1.0-as, 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex( -w  ,  h+a, 0.0);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex( -w-a,  h  , 0.0);
		renderer_set_texcoord(1.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex( -w-a,  h+a, 0.0);
	renderer_end();
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(0.0   , 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex(  0.0, 0.0, z);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0,	h, 0.0);
		renderer_set_texcoord(1.0-as, 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex( -w  , 0.0, z);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex( -w  ,	h, 0.0);
		renderer_set_texcoord(1.0   , 0.0   ); renderer_set_color_v(clr_c);	renderer_vertex( -w-a, 0.0, z);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex( -w-a,	h, 0.0);
	renderer_end();
	//-- bottom right
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex(  w+a,  -h, 0.0);
		renderer_set_texcoord(1.0   , 0.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  w+a, 0.0, z);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  w  ,  -h, 0.0);
		renderer_set_texcoord(1.0-as, 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex(  w  , 0.0, z);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0,  -h, 0.0);
		renderer_set_texcoord(0.0   , 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex(  0.0, 0.0, z);
	renderer_end();
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(1.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  w+a, -h-a, 0.0);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex(  w+a, -h  , 0.0);
		renderer_set_texcoord(1.0-as, 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  w  , -h-a, 0.0);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  w  , -h  , 0.0);
		renderer_set_texcoord(0.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  0.0, -h-a, 0.0);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0, -h  , 0.0);
	renderer_end();
	//bottom left
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0,  -h, 0.0);
		renderer_set_texcoord(0.0   , 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex(  0.0, 0.0, z);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex( -w  ,  -h, 0.0);
		renderer_set_texcoord(1.0-as, 0.0   ); renderer_set_color_v(clr_w);	renderer_vertex( -w  , 0.0, z);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex( -w-a,  -h, 0.0);
		renderer_set_texcoord(1.0   , 0.0   ); renderer_set_color_v(clr_c);	renderer_vertex( -w-a, 0.0, z);
	renderer_end();
	renderer_begin(DRAW_TRIANGLE_STRIP);
		renderer_set_texcoord(0.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex(  0.0, -h-a, 0.0);
		renderer_set_texcoord(0.0   , 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex(  0.0, -h  , 0.0);
		renderer_set_texcoord(1.0-as, 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex( -w  , -h-a, 0.0);
		renderer_set_texcoord(1.0-as, 1.0-at); renderer_set_color_v(clr_w);	renderer_vertex( -w  , -h  , 0.0);
		renderer_set_texcoord(1.0   , 1.0   ); renderer_set_color_v(clr_c);	renderer_vertex( -w-a, -h-a, 0.0);
		renderer_set_texcoord(1.0   , 1.0-at); renderer_set_color_v(clr_c);	renderer_vertex( -w-a, -h  , 0.0);
	renderer_end();

	renderer_pop_matrix();
	renderer_matrix_mode(MATRIX_MODELVIEW);

}

//----------------------------------------------------------
void MenuGL::drawTitle()
{
	static int tiltCount = 600;
	static float ta0  = -60.0;
	static float ta1  = -90.0;
	float &tilt = titleTilt;
	if(ta0 < 90.0)	ta0 += 0.7*game->speedAdj;
	else if(!thickText) ta0 += 180.0*game->speedAdj;
	else ta0 += 5.0*game->speedAdj;
	if(ta0 > 270.0)	ta0 = ta0-360.0;

	if(ta1 < 90.0)	ta1 += 0.55*game->speedAdj;
	else if(!thickText) ta1 += 180.0*game->speedAdj;
	else ta1 += 8.0*game->speedAdj;
	if(ta1 > 270.0)	ta1 = ta1-360.0;

	if(thickText)
	{
		tiltCount -= game->speedAdj;
		if(tiltCount == 0)
			tilt = 360.0+tilt;
		else if(tiltCount < 0)
		{
			tilt -= 1.0*game->speedAdj;
			if(tilt < -10.0)
			{
				tiltCount = 1500;
				tilt = -10.0;
			}
		}
		else
			tilt -= 0.01*game->speedAdj;
	}

	renderer_enable(GL_DEPTH_TEST);
	renderer_set_depth_func(GL_LEQUAL);
	renderer_set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	renderer_set_color(1.0, 1.0, 1.0, 1.0);
	renderer_push_matrix();
		renderer_enable(GL_TEXTURE_GEN_S);
		renderer_enable(GL_TEXTURE_GEN_T);
		renderer_bind_texture( envTex);
		renderer_push_matrix();
			renderer_translate(0.0,  1.0, 0.0);
			renderer_rotate( tilt, 1.0, 0.0, 0.0);
			renderer_rotate(   ta0, 0.0, 1.0, 0.0);
			textGeometryChromium(thickText);
		renderer_pop_matrix();
		renderer_push_matrix();
			renderer_translate(0.0, -1.0, 0.0);
			renderer_rotate( tilt, 1.0, 0.0, 0.0);
			renderer_rotate(   ta1, 0.0, 1.0, 0.0);
			textGeometryBSU(thickText);
		renderer_pop_matrix();
		renderer_disable(GL_TEXTURE_GEN_S);
		renderer_disable(GL_TEXTURE_GEN_T);
	renderer_pop_matrix();
	renderer_disable(GL_DEPTH_TEST);
}

//----------------------------------------------------------
void MenuGL::keyHit(MainToolkit::Key key)
{
	switch(key)
	{
		case MainToolkit::KeyUp:
			if(curSel == (MenuSelection)0)
				curSel = (MenuSelection)(NumSelections-1);
			else
				curSel = (MenuSelection)(curSel - 1);
//			curSel = (MenuSelection)(curSel - 1);
//			if(curSel < (MenuSelection)0)
//				curSel = (MenuSelection)(NumSelections-1);
			elecOffX = 0.0;
			break;
		case MainToolkit::KeyDown:
			curSel = (MenuSelection)((curSel+1)%NumSelections);
			elecOffX = 0.0;
			break;
		case MainToolkit::KeyLeft:
			decItem();
			break;
		case MainToolkit::KeyRight:
			incItem();
			break;
		case MainToolkit::KeyEnter:
			activateItem();
			break;
		default:
			break;
	}
}

//----------------------------------------------------------
void MenuGL::activateItem()
{
	Config	*config = Config::instance();
	Global *game = Global::getInstance();
	switch(curSel)
	{
		case NewGame:
			game->gameMode = Global::Game;
			game->newGame();
			game->toolkit->grabMouse(true);
			game->audio->setMusicMode(Audio::MusicGame);
			break;
		case SkillLevel:
			break;
		case GameLevel:
			break;
		case Graphics:
			break;
		case ScreenSize:
			break;
		case FullScreen:
			config->setFullScreen(!config->fullScreen());
			game->deleteTextures();
			if( !game->toolkit->setVideoMode() )
			{
				mssgHelpOverride = true;
				mssgAlpha = 1.1;
				if( config->fullScreen() )
					sprintf(mssgText, _("---- error setting full screen mode ----"));
				else
					sprintf(mssgText, _("---- error setting window mode ----"));
				config->setFullScreen(!config->fullScreen());
			}
			game->loadTextures();
			break;
		case Sound:
			break;
		case Music:
			break;
		case MovementSpeed:
			break;
		case Quit:
			game->game_quit = true;
			break;
		case NumSelections:
			break;
	}
}

//----------------------------------------------------------
void MenuGL::incItem()
{
	Config	*config = Config::instance();
	HiScore *hiScore = HiScore::getInstance();
	float	pos[3] = { 0.0, 0.0, 25.0 };
	switch(curSel)
	{
		case NewGame:
			activateItem();
			break;
		case SkillLevel:
			config->setGameSkillBase(config->gameSkillBase()+0.1);
			if( config->debug() ) hiScore->print(config->intSkill());
			game->newGame();
			break;
		case GameLevel:
			game->gameLevel++;
			if(game->gameLevel > config->maxLevel())
			{
				mssgHelpOverride = true;
				mssgAlpha = 1.1;
				sprintf(mssgText, _("---- you must complete level %d before you can select level %d ----"), config->maxLevel(), game->gameLevel);
				game->gameLevel = config->maxLevel();
			}
			else
				game->newGame();
			break;
		case Graphics:
			config->setGfxLevel(config->gfxLevel()+1);
			break;
		case ScreenSize:
			config->setScreenSize(config->approxScreenSize()+1);
			game->deleteTextures();
			if( !game->toolkit->setVideoMode() )
			{
				mssgHelpOverride = true;
				mssgAlpha = 1.1;
				sprintf(mssgText, _("---- error setting screen size ----"));
				config->setScreenSize(config->approxScreenSize()-1);
			}
			game->loadTextures();
			break;
		case FullScreen:
			if(!config->fullScreen())
			{
				config->setFullScreen(true);
				game->deleteTextures();
				if( !game->toolkit->setVideoMode() )
				{
					mssgHelpOverride = true;
					mssgAlpha = 1.1;
					sprintf(mssgText, _("---- error setting full screen mode ----"));
					config->setFullScreen(false);
				}
				game->loadTextures();
			}
			break;
		case Sound:
			config->setVolSound(config->volSound()+0.05);
			game->audio->setSoundVolume(config->volSound());
			game->audio->playSound(Audio::Explosion, pos);
			break;
		case Music:
			config->setVolMusic(config->volMusic()+0.05);
			game->audio->setMusicVolume(config->volMusic());
			break;
		case MovementSpeed:
			config->setMovementSpeed(config->movementSpeed()+0.005);
			break;
		case Quit:
			activateItem();
			break;
		case NumSelections:
			break;
	}
}

//----------------------------------------------------------
void MenuGL::decItem()
{
	Config	*config = Config::instance();
	HiScore *hiScore = HiScore::getInstance();
	float	pos[3] = { 0.0, 0.0, 25.0 };
	switch(curSel)
	{
		case NewGame:
			break;
		case SkillLevel:
			config->setGameSkillBase(config->gameSkillBase()-0.1);
			if( config->debug() ) hiScore->print(config->intSkill());
			game->newGame();
			break;
		case GameLevel:
			game->gameLevel -= 1;
			if(game->gameLevel < 1)
				game->gameLevel = 1;
			game->newGame();
			break;
		case Graphics:
			config->setGfxLevel(config->gfxLevel()-1);
			break;
		case ScreenSize:
			config->setScreenSize(config->approxScreenSize()-1);
			game->deleteTextures();
			if( !game->toolkit->setVideoMode() )
			{
				mssgHelpOverride = true;
				mssgAlpha = 1.1;
				sprintf(mssgText, _("---- error setting screen size ----"));
				config->setScreenSize(config->approxScreenSize()+1);
			}
			game->loadTextures();
			break;
		case FullScreen:
			if(config->fullScreen())
			{
				config->setFullScreen(false);
				game->deleteTextures();
				if( !game->toolkit->setVideoMode() )
				{
					mssgHelpOverride = true;
					mssgAlpha = 1.1;
					sprintf(mssgText, _("---- error setting full screen mode ----"));
					config->setFullScreen(true);
				}
				game->loadTextures();
			}
			break;
		case Sound:
			config->setVolSound(config->volSound()-0.05);
			game->audio->setSoundVolume(config->volSound());
			game->audio->playSound(Audio::Explosion, pos);
			break;
		case Music:
			config->setVolMusic(config->volMusic()-0.05);
			game->audio->setMusicVolume(config->volMusic());
			break;
		case MovementSpeed:
			config->setMovementSpeed(config->movementSpeed()-0.005);
			break;
		case Quit:
			break;
		case NumSelections:
			break;
	}
}

/**
 * horrible, hacky, but quick to implement....
 */
//----------------------------------------------------------
void MenuGL::mousePress(MainToolkit::Button but, int xi, int yi)
{
	if(but == MainToolkit::Left)
	{
		float x,y;
		Config *config = Config::instance();
		x = -2.0*(0.5-(((float)xi)/config->screenW()))* 16.60;
		y =  2.0*(0.5-(((float)yi)/config->screenH()))* 12.45;

		float	p = -y+(1.0+txtHeight*1.5);
		float	s = txtHeight*2.5;
		int		mSel = -1;
		if(p > 0.0)
		{
			// reset electric
			elecOffX = 0.0;

			p = p/s;
			mSel = (int)floor(p);
			if( mSel >= 0 && mSel < (int)NumSelections)
			{
				if(mSel != (int)curSel)
				{
					curSel = (MenuSelection)mSel;
					elecOffX = 0.0;
					mSel = -1;
				}
			}
		}
		float l  = -8.0 - butOffset;
		float hw = butWidth*0.5;
		if(mSel >= 0)
		{
			if(x > l && x < l+hw )
				decItem();
			else if (x > l+hw && x < l+butWidth)
				incItem();
			else
				activateItem();
		}
	}
}

