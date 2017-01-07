/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "GfxOpenGL"
#include <imagine/gfx/Gfx.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/GLContext.hh>
#include "GLStateCache.hh"
#include <imagine/util/Interpolator.hh>
#include "private.hh"
#include "utils.h"

#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT				0x0402
#endif

#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT				0x0403
#endif

namespace Gfx
{

Base::GLDisplay glDpy{};
Base::GLContext gfxContext{};
Base::GLDrawable currWin{};
GLStateCache glState;
TimedInterpolator<Gfx::GC> projAngleM;
bool checkGLErrors = Config::DEBUG_BUILD;
bool checkGLErrorsVerbose = false;
static constexpr bool multipleContextsPerThread = Config::envIsAndroid;

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
bool useFixedFunctionPipeline = true;
#endif
static ColorComp vColor[4]{}; // color when using shader pipeline
static ColorComp texEnvColor[4]{}; // color when using shader pipeline
static Base::Timer releaseShaderCompilerTimer;

TextureRef newTex()
{
	GLuint ref;
	glGenTextures(1, &ref);
	//logMsg("created texture:0x%X", ref);
	return ref;
}

void deleteTex(TextureRef texRef)
{
	//logMsg("deleting texture:0x%X", texRef);
	glcDeleteTextures(1, &texRef);
}

void setZTest(bool on)
{
	if(on)
	{
		glcEnable(GL_DEPTH_TEST);
	}
	else
	{
		glcDisable(GL_DEPTH_TEST);
	}
}

void setBlend(bool on)
{
	if(on)
		glcEnable(GL_BLEND);
	else
		glcDisable(GL_BLEND);
}

void setBlendFunc(BlendFunc s, BlendFunc d)
{
	glcBlendFunc((GLenum)s, (GLenum)d);
}

void setBlendMode(uint mode)
{
	switch(mode)
	{
		bcase BLEND_MODE_OFF:
			setBlend(false);
		bcase BLEND_MODE_ALPHA:
			setBlendFunc(BlendFunc::SRC_ALPHA, BlendFunc::ONE_MINUS_SRC_ALPHA); // general blending
			//setBlendFunc(BlendFunc::ONE, BlendFunc::ONE_MINUS_SRC_ALPHA); // for premultiplied alpha
			setBlend(true);
		bcase BLEND_MODE_INTENSITY:
			setBlendFunc(BlendFunc::SRC_ALPHA, BlendFunc::ONE);
			setBlend(true);
	}
}

void setBlendEquation(uint mode)
{
#if !defined CONFIG_GFX_OPENGL_ES \
	|| (defined CONFIG_BASE_IOS || defined __ANDROID__)
	glcBlendEquation(mode == BLEND_EQ_ADD ? GL_FUNC_ADD :
			mode == BLEND_EQ_SUB ? GL_FUNC_SUBTRACT :
			mode == BLEND_EQ_RSUB ? GL_FUNC_REVERSE_SUBTRACT :
			GL_FUNC_ADD);
#endif
}

void setImgMode(uint mode)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		switch(mode)
		{
			bcase IMG_MODE_REPLACE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			bcase IMG_MODE_MODULATE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			bcase IMG_MODE_ADD: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			bcase IMG_MODE_BLEND: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		}
		return;
	}
	#endif
	// TODO
}

void setImgBlendColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		GLfloat col[4] {r, g, b, a};
		glcTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
		return;
	}
	#endif
	texEnvColor[0] = r;
	texEnvColor[1] = g;
	texEnvColor[2] = b;
	texEnvColor[3] = a;
}

void setZBlend(uchar on)
{
//	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
//	if(useFixedFunctionPipeline)
//	{
//		if(on)
//		{
//			#ifndef CONFIG_GFX_OPENGL_ES
//			glFogi(GL_FOG_MODE, GL_LINEAR);
//			#else
//			glFogf(GL_FOG_MODE, GL_LINEAR);
//			#endif
//			glFogf(GL_FOG_DENSITY, 0.1f);
//			glHint(GL_FOG_HINT, GL_DONT_CARE);
//			glFogf(GL_FOG_START, proj.zRange/2.0);
//			glFogf(GL_FOG_END, proj.zRange);
//			glcEnable(GL_FOG);
//		}
//		else
//		{
//			glcDisable(GL_FOG);
//		}
//	}
//	#endif
	if(!useFixedFunctionPipeline)
	{
		bug_exit("TODO");
	}
}

void setZBlendColor(ColorComp r, ColorComp g, ColorComp b)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		GLfloat c[4] = {r, g, b, 1.0f};
		glFogfv(GL_FOG_COLOR, c);
	}
	#endif
	if(!useFixedFunctionPipeline)
	{
		bug_exit("TODO");
	}
}

void setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		glcColor4f(r, g, b, a);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	// !useFixedFunctionPipeline
	if(vColor[0] == r && vColor[1] == g && vColor[2] == b && vColor[3] == a)
		return;
	vColor[0] = r;
	vColor[1] = g;
	vColor[2] = b;
	vColor[3] = a;
	glVertexAttrib4f(VATTR_COLOR, r, g, b, a);
	//logMsg("set color: %f:%f:%f:%f", (double)r, (double)g, (double)b, (double)a);
	#endif
}

uint color()
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		return ColorFormat.build(glState.colorState[0], glState.colorState[1], glState.colorState[2], glState.colorState[3]);
	}
	#endif
	// !useFixedFunctionPipeline
	return ColorFormat.build((float)vColor[0], (float)vColor[1], (float)vColor[2], (float)vColor[3]);
}

void setVisibleGeomFace(uint faces)
{
	if(faces == BOTH_FACES)
	{
		glcDisable(GL_CULL_FACE);
	}
	else if(faces == FRONT_FACES)
	{
		glcEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT); // our order is reversed from OpenGL
	}
	else
	{
		glcEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
}

void setClipRect(bool on)
{
	if(on)
		glcEnable(GL_SCISSOR_TEST);
	else
		glcDisable(GL_SCISSOR_TEST);
}

void setClipRectBounds(const Base::Window &win, int x, int y, int w, int h)
{
	//logMsg("scissor before transform %d,%d size %d,%d", x, y, w, h);
	// translate from view to window coordinates
	if(!Config::SYSTEM_ROTATES_WINDOWS)
	{
		using namespace Base;
		switch(win.softOrientation())
		{
			bcase VIEW_ROTATE_0:
				//x += win.viewport.rect.x;
				y = win.height() - (y + h);
			bcase VIEW_ROTATE_90:
				//x += win.viewport.rect.y;
				//y = win.width() - (y + h /*+ (win.w - win.viewport.rect.x2)*/);
				std::swap(x, y);
				std::swap(w, h);
				x = (win.realWidth() - x) - w;
				y = (win.realHeight() - y) - h;
			bcase VIEW_ROTATE_270:
				//x += win.viewport.rect.y;
				//y += win.viewport.rect.x;
				std::swap(x, y);
				std::swap(w, h);
			bcase VIEW_ROTATE_180:
				x = (win.realWidth() - x) - w;
				//y = win.height() - (y + h);
				//std::swap(x, y);
				//std::swap(w, h);
				//x += win.viewport.rect.x;
				//y += win.height() - win.viewport.bounds().y2;
		}
	}
	else
	{
		//x += win.viewport.rect.x;
		y = win.height() - (y + h /*+ win.viewport.rect.y*/);
	}
	//logMsg("setting Scissor %d,%d size %d,%d", x, y, w, h);
	glScissor(x, y, w, h);
}

void setClearColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	//GLfloat c[4] = {r, g, b, a};
	//logMsg("setting clear color %f %f %f %f", (float)r, (float)g, (float)b, (float)a);
	// TODO: add glClearColor to the state cache
	glClearColor((float)r, (float)g, (float)b, (float)a);
}

void setDither(bool on)
{
	if(on)
		glcEnable(GL_DITHER);
	else
	{
		logMsg("disabling dithering");
		glcDisable(GL_DITHER);
	}
}

bool dither()
{
	return glcIsEnabled(GL_DITHER);
}

void releaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	glReleaseShaderCompiler();
	#endif
}

void autoReleaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!releaseShaderCompilerTimer)
	{
		releaseShaderCompilerTimer.callbackAfterMSec(
			[]()
			{
				logMsg("automatically releasing shader compiler");
				glReleaseShaderCompiler();
			}, 1, {});
	}
	#endif
}

static void discardTemporaryData()
{
	discardTexturePBO();
}

void clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

bool bind()
{
	if(multipleContextsPerThread && gfxContext != Base::GLContext::current(glDpy))
	{
		logMsg("restoring context");
		gfxContext.setCurrent(glDpy, gfxContext, currWin);
		return true;
	}
	return false;
}

void updateDrawableForSurfaceChange(Drawable &drawable, Base::Window::SurfaceChange change)
{
	if(change.destroyed())
	{
		Gfx::deinitDrawable(drawable);
	}
	else if(change.reset())
	{
		if(currWin == drawable)
			currWin = {};
	}
}

bool setCurrentDrawable(Drawable win)
{
	if(multipleContextsPerThread && gfxContext != Base::GLContext::current(glDpy))
	{
		logMsg("restoring context");
		currWin = win;
		gfxContext.setCurrent(glDpy, gfxContext, win);
		return true;
	}
	else
	{
		if(win == currWin)
			return false;
		gfxContext.setDrawable(glDpy, win, gfxContext);
		if(shouldSpecifyDrawReadBuffers && win)
		{
			//logMsg("specifying draw/read buffers");
			const GLenum back = Config::Gfx::OPENGL_ES_MAJOR_VERSION ? GL_BACK : GL_BACK_LEFT;
			glDrawBuffers(1, &back);
			glReadBuffer(GL_BACK);
		}
		currWin = win;
		return true;
	}
}

bool updateCurrentDrawable(Drawable &drawable, Base::Window &win, Base::Window::DrawParams params, Viewport viewport, Mat4 projMat)
{
	if(!drawable)
	{
		std::error_code ec{};
		drawable = glDpy.makeDrawable(win, gfxBufferConfig, ec);
		if(ec)
		{
			logErr("Error creating GL drawable");
		}
	}
	if(setCurrentDrawable(drawable) || params.wasResized())
	{
		setViewport(viewport);
		setProjectionMatrix(projMat);
		return true;
	}
	return false;
}

void deinitDrawable(Drawable &drawable)
{
	if(currWin == drawable)
		currWin = {};
	glDpy.deleteDrawable(drawable);
}

void presentDrawable(Drawable win)
{
	discardTemporaryData();
	gfxContext.present(glDpy, win, gfxContext);
}

void finishPresentDrawable(Drawable win)
{
	gfxContext.finishPresent(glDpy, win);
}

void finish()
{
	setCurrentDrawable({});
	if(Config::envIsIOS)
	{
		glFinish();
	}
}

void setCorrectnessChecks(bool on)
{
	if(on)
	{
		logWarn("enabling verification of OpenGL state");
	}
	GLStateCache::verifyState = on;
	checkGLErrors = on ? true : Config::DEBUG_BUILD;
	checkGLErrorsVerbose = on;
}

}
