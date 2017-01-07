/*  This file is part of GBA.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <gambatte.h>
#include <resample/resampler.h>
#include <resample/resamplerinfo.h>
#include <main/Cheats.hh>
#include <main/Palette.hh>
#include "internal.hh"

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011\nthe Gambatte Team\ngambatte.sourceforge.net";
gambatte::GB gbEmu;
static Resampler *resampler{};
static uint8 activeResampler = 1;
static const GBPalette *gameBuiltinPalette{};
static const int gbResX = 160, gbResY = 144;

#ifdef GAMBATTE_COLOR_RGB565
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
#else
static constexpr auto pixFmt = IG::PIXEL_FMT_RGBA8888;
#endif

bool EmuSystem::hasCheats = true;
EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](const char *name)
	{
		return string_hasDotExtension(name, "gb") ||
			string_hasDotExtension(name, "gbc");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = defaultFsFilter;

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "Test Game", "game.gb"	}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "GB";
}

const char *EmuSystem::systemName()
{
	return "Game Boy";
}

void applyGBPalette()
{
	uint idx = optionGBPal;
	assert(idx < IG::size(gbPal));
	bool useBuiltin = optionUseBuiltinGBPalette && gameBuiltinPalette;
	if(useBuiltin)
		logMsg("using built-in game palette");
	else
		logMsg("using palette index %d", idx);
	const GBPalette &pal = useBuiltin ? *gameBuiltinPalette : gbPal[idx];
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(0, i, pal.bg[i]);
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(1, i, pal.sp1[i]);
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(2, i, pal.sp2[i]);
}

void EmuSystem::onOptionsLoaded()
{
	gbEmu.setInputGetter(&gbcInput);
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	gbEmu.reset();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.gqs", statePath, gameName, saveSlotChar(slot));
}

std::error_code EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	logMsg("saving state %s", saveStr.data());
	if(!gbEmu.saveState(/*screenBuff*/0, 160, saveStr.data()))
		return {EIO, std::system_category()};
	else
		return {};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FS::exists(saveStr.data()))
	{
		logMsg("loading state %s", saveStr.data());
		if(!gbEmu.loadState(saveStr.data()))
			return {{EIO, std::system_category()}};
		else
			return {{}};
	}
	return {{ENOENT, std::system_category()}};
}

void EmuSystem::saveBackupMem()
{
	logMsg("saving battery");
	gbEmu.saveSavedata();

	writeCheatFile();
}

void EmuSystem::savePathChanged()
{
	if(gameIsRunning())
		gbEmu.setSaveDir(savePath());
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		logMsg("saving auto-state");
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		gbEmu.saveState(/*screenBuff*/0, 160, saveStr.data());
	}
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
	cheatList.clear();
	cheatsModified = 0;
	gameBuiltinPalette = nullptr;
}

static int loadGameCommon(gambatte::LoadRes result)
{
	if(result != gambatte::LOADRES_OK)
	{
		popup.printf(3, 1, "%s", gambatte::to_string(result).c_str());
		return 0;
	}
	emuVideo.initImage(0, gbResX, gbResY);
	if(!gbEmu.isCgb())
	{
		gameBuiltinPalette = findGbcTitlePal(gbEmu.romTitle().c_str());
		if(gameBuiltinPalette)
			logMsg("game %s has built-in palette", gbEmu.romTitle().c_str());
		applyGBPalette();
	}

	readCheatFile();
	applyCheats();

	logMsg("started emu");
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame();
	setupGamePaths(path);
	gbEmu.setSaveDir(EmuSystem::savePath());
	auto size = io.size();
	auto mmapData = io.mmapConst();
	gambatte::LoadRes result;
	if(mmapData)
	{
		result = gbEmu.load(mmapData, size, origFilename, optionReportAsGba ? gbEmu.GBA_CGB : 0);
	}
	else
	{
		auto romData = std::make_unique<char[]>(size);
		if(io.read(romData.get(), size) != (ssize_t)size)
		{
			popup.printf(3, 1, "IO Error");
			return 0;
		}
		result = gbEmu.load(romData.get(), size, origFilename, optionReportAsGba ? gbEmu.GBA_CGB : 0);
	}
	return loadGameCommon(result);
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	long outputRate = std::round(optionSoundRate * (59.73 * frameTime));
	long inputRate = 2097152;
	if(optionAudioResampler >= ResamplerInfo::num())
		optionAudioResampler = std::min((int)ResamplerInfo::num(), 1);
	if(!resampler || optionAudioResampler != activeResampler || resampler->outRate() != outputRate)
	{
		logMsg("setting up resampler %d for input rate %ldHz", (int)optionAudioResampler, inputRate);
		delete resampler;
		resampler = ResamplerInfo::get(optionAudioResampler).create(inputRate, outputRate, 35112 + 2064);
		activeResampler = optionAudioResampler;
	}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	alignas(std::max_align_t) uint8 snd[(35112+2064)*4];
	size_t samples = 35112;
	int frameSample;
	DelegateFunc<void()> frameCallback{};
	if(processGfx)
	{
		auto img = emuVideo.startFrame();
		frameCallback =
			[&img, renderGfx]()
			{
				img.endFrame();
				if(renderGfx)
				{
					updateAndDrawEmuVideo();
				}
			};
		frameSample = gbEmu.runFor((gambatte::PixelType*)img.pixmap().pixel({}), img.pixmap().pitchPixels(),
			(uint_least32_t*)snd, samples, frameCallback);
	}
	else
	{
		if(renderGfx)
		{
			frameCallback = [&](){ updateAndDrawEmuVideo(); };
		}
		frameSample = gbEmu.runFor(nullptr, 160, (uint_least32_t*)snd, samples, frameCallback);
	}
	if(renderAudio)
	{
		if(frameSample == -1)
		{
			logMsg("no emulated frame with %d samples", (int)samples);
		}
		//else logMsg("emulated frame at %d with %d samples", frameSample, samples);
		if(unlikely(samples < 34000))
		{
			uint repeatPos = std::max((int)samples-1, 0);
			uint32 *sndFrame = (uint32*)snd;
			logMsg("only %d, repeat %d", (int)samples, (int)sndFrame[repeatPos]);
			for(uint i = samples; i < 35112; i++)
			{
				sndFrame[i] = sndFrame[repeatPos];
			}
			samples = 35112;
		}
		// video rendered in runFor()
		short destBuff[(Audio::maxRate()/54)*2];
		uint destFrames = resampler->resample(destBuff, (const short*)snd, samples);
		assert(Audio::pcmFormat.framesToBytes(destFrames) <= sizeof(destBuff));
		EmuSystem::writeSound(destBuff, destFrames);
	}
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((8./255.) * .4, (232./255.) * .4, (222./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((8./255.) * .4, (232./255.) * .4, (222./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((0./255.) * .4, (77./255.) * .4, (74./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

CallResult EmuSystem::onInit()
{
	emuVideo.initFormat(pixFmt);
	return OK;
}
