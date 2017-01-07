/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "video"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include "internal.hh"

extern "C"
{
	#include "palette.h"
	#include "video.h"
	#include "videoarch.h"
	#include "kbdbuf.h"
	#include "sound.h"
	#include "vsync.h"
	#include "vsyncapi.h"
}

struct video_canvas_s *activeCanvas{};
IG::Pixmap canvasSrcPix{};
double systemFrameRate = 60.0;

void setCanvasSkipFrame(bool on)
{
	activeCanvas->skipFrame = on;
}

CLINK LVISIBLE int vsync_do_vsync2(struct video_canvas_s *c, int been_skipped);
int vsync_do_vsync2(struct video_canvas_s *c, int been_skipped)
{
	//assert(EmuSystem::gameIsRunning());
	if(likely(runningFrame))
	{
		//logMsg("vsync_do_vsync signaling main thread");
		execDoneSem.notify();
		execSem.wait();
	}
	else
	{
		logMsg("spurious vsync_do_vsync()");
	}
	return c->skipFrame;
}

void vsyncarch_refresh_frequency_changed(double rate)
{
	logMsg("system frame rate:%.4f", rate);
	systemFrameRate = rate;
	EmuSystem::configAudioPlayback();
}

void video_arch_canvas_init(struct video_canvas_s *c)
{
	logMsg("created canvas with size %d,%d", c->draw_buffer->canvas_width, c->draw_buffer->canvas_height);
	c->video_draw_buffer_callback = nullptr;
	activeCanvas = c;
}

int video_canvas_set_palette(video_canvas_t *c, struct palette_s *palette)
{
	iterateTimes(256, i)
	{
		plugin.video_render_setrawrgb(i, pixFmt.desc().build(i/255., 0., 0., 0.), pixFmt.desc().build(0., i/255., 0., 0.), pixFmt.desc().build(0., 0., i/255., 0.));
	}
	plugin.video_render_initraw(c->videoconfig);

	if(palette)
	{
		c->palette = palette;
		iterateTimes(palette->num_entries, i)
		{
			auto col = pixFmt.desc().build(palette->entries[i].red/255., palette->entries[i].green/255., palette->entries[i].blue/255., 0.);
			logMsg("set color %d to %X", i, col);
			plugin.video_render_setphysicalcolor(c->videoconfig, i, col, pixFmt.bitsPerPixel());
		}
	}

	return 0;
}

void video_canvas_refresh(struct video_canvas_s *c, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
	xi *= c->videoconfig->scalex;
	w *= c->videoconfig->scalex;
	yi *= c->videoconfig->scaley;
	h *= c->videoconfig->scaley;

	w = std::min(w, c->pixmap->w());
	h = std::min(h, c->pixmap->h());

	plugin.video_canvas_render(c, (BYTE*)c->pixmap->pixel({}), w, h, xs, ys, xi, yi, c->pixmap->pitchBytes(), pixFmt.bitsPerPixel());
}

void resetCanvasSourcePixmap(struct video_canvas_s *c)
{
	uint canvasW = c->pixmap->w();
	uint canvasH = c->pixmap->h();
	if(optionCropNormalBorders && (canvasH == 247 || canvasH == 272))
	{
		logMsg("cropping borders");
		// Crop all vertical borders on NTSC, leaving leftover side borders
		int xBorderSize = 32, yBorderSize = 23;
		int height = 200;
		int startX = yBorderSize, startY = yBorderSize;
		if(canvasH == 272) // PAL
		{
			// Crop all horizontal borders on PAL, leaving leftover top/bottom borders
			yBorderSize = 32;
			height = 206;
			startX = xBorderSize; startY = xBorderSize;
		}
		int width = 320+(xBorderSize*2 - startX*2);
		int widthPadding = startX*2;
		canvasSrcPix = c->pixmap->subPixmap({startX, startY}, {width, height});
	}
	else
	{
		canvasSrcPix = *c->pixmap;
	}
}

void video_canvas_resize(struct video_canvas_s *c, char resize_canvas)
{
	int x = c->draw_buffer->canvas_width;
	int y = c->draw_buffer->canvas_height;
	x *= c->videoconfig->scalex;
	y *= c->videoconfig->scaley;
	logMsg("resized canvas to %d,%d, renderer %d", x, y, c->videoconfig->rendermode);
	delete c->pixmap;
	c->pixmap = new IG::MemPixmap{{{x, y}, pixFmt}};
	resetCanvasSourcePixmap(c);
}

video_canvas_t *video_canvas_create(video_canvas_t *c, unsigned int *width, unsigned int *height, int mapped)
{
	logMsg("canvas create:0x%p renderer %d", c, c->videoconfig->rendermode);
	return c;
}

void video_canvas_destroy(struct video_canvas_s *c)
{
	logMsg("canvas destroy:0x%p", c);
	delete c->pixmap;
	c->pixmap = nullptr;
}
