/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "EmuVideo"
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/Screenshot.hh>
#include "private.hh"

void EmuVideo::resetImage()
{
	auto desc = vidImg.usedPixmapDesc();
	vidImg.deinit();
	setFormat(desc);
}

void EmuVideo::setFormat(IG::PixmapDesc desc)
{
	if(vidImg && desc == vidImg.usedPixmapDesc())
	{
		return; // no change to format
	}
	memPix = {};
	if(!vidImg)
	{
		Gfx::TextureConfig conf{desc};
		conf.setWillWriteOften(true);
		vidImg.init(r, conf);
	}
	else
	{
		vidImg.setFormat(desc, 1);
	}
	logMsg("resized to:%dx%d", desc.w(), desc.h());
	// update all EmuVideoLayers
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	emuVideoLayer.setEffect(optionImgEffect);
	#else
	emuVideoLayer.resetImage();
	#endif
	if((uint)optionImageZoom > 100)
		placeEmuViews();
}

EmuVideoImage EmuVideo::startFrame()
{
	auto lockedTex = vidImg.lock(0);
	if(!lockedTex)
	{
		if(!memPix)
		{
			logMsg("created backing memory pixmap");
			memPix = {vidImg.usedPixmapDesc()};
		}
		return {*this, (IG::Pixmap)memPix};
	}
	return {*this, lockedTex};
}

void EmuVideo::writeFrame(Gfx::LockedTextureBuffer texBuff)
{
	if(screenshotNextFrame)
	{
		doScreenshot(texBuff.pixmap());
	}
	vidImg.unlock(texBuff);
}

void EmuVideo::writeFrame(IG::Pixmap pix)
{
	if(screenshotNextFrame)
	{
		doScreenshot(pix);
	}
	vidImg.write(0, pix, {}, vidImg.bestAlignment(pix));
}

void EmuVideo::takeGameScreenshot()
{
	screenshotNextFrame = true;
}

void EmuVideo::doScreenshot(IG::Pixmap pix)
{
	screenshotNextFrame = false;
	FS::PathString path;
	int screenshotNum = sprintScreenshotFilename(path);
	if(screenshotNum == -1)
	{
		popup.postError("Too many screenshots");
	}
	else
	{
		if(!writeScreenshot(pix, path.data()))
		{
			popup.printf(2, 1, "Error writing screenshot #%d", screenshotNum);
		}
		else
		{
			popup.printf(2, 0, "Wrote screenshot #%d", screenshotNum);
		}
	}
}

bool EmuVideo::isExternalTexture()
{
	#ifdef __ANDROID__
	return vidImg.isExternal();
	#else
	return false;
	#endif
}

void EmuVideoImage::endFrame()
{
	if(texBuff)
	{
		emuVideo->writeFrame(texBuff);
	}
	else if(pix)
	{
		emuVideo->writeFrame(pix);
	}
}

IG::WP EmuVideo::size() const
{
	if(!vidImg)
		return {};
	else
		return vidImg.usedPixmapDesc().size();
}
