#pragma once

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

#include <imagine/gfx/GfxSprite.hh>
#include <emuframework/VideoImageOverlay.hh>
#include <emuframework/VideoImageEffect.hh>
#include <emuframework/EmuVideo.hh>

struct AppWindowData;

class EmuVideoLayer
{
public:
	EmuVideoLayer(EmuVideo &video);
	void init();
	void deinit();
	void place(const IG::WindowRect &viewportRect, const Gfx::ProjectionPlane &projP, bool onScreenControlsOverlay);
	void draw(const Gfx::ProjectionPlane &projP);
	void setOverlay(uint effect);
	void setOverlayIntensity(Gfx::GC intensity);
	void placeOverlay();
	void setEffect(uint effect);
	void setEffectBitDepth(uint bits);
	void placeEffect();
	void setLinearFilter(bool on);
	void resetImage();

	const IG::WindowRect &gameRect() const
	{
		return gameRect_;
	}

private:
	VideoImageOverlay vidImgOverlay{};
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	VideoImageEffect vidImgEffect{};
	#endif
	EmuVideo &video;
	Gfx::Sprite disp{};
	IG::WindowRect gameRect_{};
	Gfx::GCRect gameRectG{};
	bool useLinearFilter = true;

	void compileDefaultPrograms();
};
