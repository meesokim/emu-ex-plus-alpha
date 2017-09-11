/*  This file is part of NEO.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/base/Pipe.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/ScopeGuard.hh>
#include "internal.hh"

extern "C"
{
	#include <gngeo/roms.h>
	#include <gngeo/conf.h>
	#include <gngeo/emu.h>
	#include <gngeo/fileio.h>
	#include <gngeo/timer.h>
	#include <gngeo/memory.h>
	#include <gngeo/video.h>
	#include <gngeo/screen.h>
	#include <gngeo/menu.h>

	CONFIG conf{};
	GN_Rect visible_area;

	extern int skip_this_frame;
	Uint16 play_buffer[16384]{};
	GN_Surface *buffer{};
	static CONF_ITEM rompathConfItem{};

	CONF_ITEM* cf_get_item_by_name(const char *name)
	{
		//logMsg("getting conf item %s", name);
		static CONF_ITEM conf{};
		if(string_equal(name, "rompath"))
		{
			string_copy(rompathConfItem.data.dt_str.str, EmuSystem::gamePath());
			return &rompathConfItem;
		}
		else if(string_equal(name, "dump"))
		{
			static CONF_ITEM dump{};
			return &dump;
		}
		else if(string_equal(name, "effect"))
		{
			strcpy(conf.data.dt_str.str, "none");
		}
		else if(string_equal(name, "blitter"))
		{
			strcpy(conf.data.dt_str.str, "soft");
		}
		else if(string_equal(name, "transpack"))
		{
			strcpy(conf.data.dt_str.str, "");
		}
		else
		{
			logErr("unknown conf item %s", name);
		}
		return &conf;
	}

	const char *get_gngeo_dir(void)
	{
		return EmuSystem::savePath();
	}
}

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011 the\nGngeo Team\ncode.google.com/p/gngeo";
bool EmuSystem::handlesGenericIO = false; // TODO: need to re-factor GnGeo file loading code
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static uint16 screenBuff[352*256] __attribute__ ((aligned (8))){};
static GN_Surface sdlSurf;
static FS::PathString datafilePath{};
static const int FBResX = 352;
static EmuVideo *emuVideo{};
// start image on y 16, x 24, size 304x224, 48 pixel padding on the right
static IG::Pixmap srcPix{{{304, 224}, pixFmt}, (char*)screenBuff + (16*FBResX*2) + (24*2), {FBResX, IG::Pixmap::PIXEL_UNITS}};
static EmuSystem::OnLoadProgressDelegate onLoadProgress{};

CLINK void main_frame();

const char *EmuSystem::shortSystemName()
{
	return "NeoGeo";
}

const char *EmuSystem::systemName()
{
	return "Neo Geo";
}

CLINK int gn_strictROMChecking()
{
	return optionStrictROMChecking;
}

static bool hasNeoGeoExtension(const char *name)
{
	return false; // archives handled by EmuFramework
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasNeoGeoExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasNeoGeoExtension;

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	neogeo_reset();
	cpu_z80_init();
	YM2610Reset();
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.sta", statePath, gameName, saveSlotCharUpper(slot));
}

EmuSystem::Error EmuSystem::saveState(const char *path)
{
	if(!save_stateWithName(path))
		return EmuSystem::makeFileWriteError();
	else
		return {};
}

EmuSystem::Error EmuSystem::loadState(const char *path)
{
	if(!load_stateWithName(path))
		return EmuSystem::makeFileReadError();
	else
		return {};
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
    save_nvram(conf.game);
    save_memcard(conf.game);
	}
}

void EmuSystem::closeSystem()
{
	close_game();
}

#ifdef USE_GENERATOR68K
CLINK void swap_memory(Uint8 * mem, Uint32 length);

static bool swapCPUMemForDump()
{
	bool swappedBIOS = 0;
	swap_memory(memory.rom.cpu_m68k.p, memory.rom.cpu_m68k.size);
	if (memory.rom.bios_m68k.p[0]==0x10)
	{
		logMsg("BIOS BYTE1=%08x\n",memory.rom.bios_m68k.p[0]);
		swap_memory(memory.rom.bios_m68k.p, memory.rom.bios_m68k.size);
		swappedBIOS = 1;
	}
	swap_memory(memory.game_vector, 0x80);
	return swappedBIOS;
}

static void reverseSwapCPUMemForDump(bool swappedBIOS)
{
	swap_memory(memory.game_vector, 0x80);
	if(swappedBIOS)
	{
		swap_memory(memory.rom.bios_m68k.p, memory.rom.bios_m68k.size);
	}
	swap_memory(memory.rom.cpu_m68k.p, memory.rom.cpu_m68k.size);
}

#endif

void gn_init_pbar(uint action,int size)
{
	using namespace Base;
	logMsg("init pbar %d, %d", action, size);
	if(onLoadProgress)
	{
		const char *str = "";
		switch(action)
		{
			bcase PBAR_ACTION_LOADROM:
			{
				// defaults to "Loading..."
			}
			bcase PBAR_ACTION_DECRYPT:
			{
				str = "Decrypting...";
			}
			bcase PBAR_ACTION_SAVEGNO:
			{
				str = "Building Cache...\n(may take a while)";
			}
		}
		onLoadProgress(0, size, str);
	}
}
void gn_update_pbar(int pos)
{
	using namespace Base;
	logMsg("update pbar %d", pos);
	if(onLoadProgress)
	{
		onLoadProgress(pos, 0, nullptr);
	}
}

static auto openGngeoDataIO(const char *filename)
{
	#ifdef __ANDROID__
	return openAppAssetIO(filename);
	#else
	return FS::fileFromArchive(datafilePath.data(), filename);
	#endif
}

CLINK ROM_DEF *res_load_drv(const char *name)
{
	auto drvFilename = string_makePrintf<32>(DATAFILE_PREFIX "rom/%s.drv", name);
	auto io = openGngeoDataIO(drvFilename.data());
	if(!io)
	{
		logErr("Can't open driver %s", name);
		return nullptr;
	}

	// Fill out the driver struct
	auto drv = (ROM_DEF*)calloc(sizeof(ROM_DEF), 1);
	io.read(drv->name, 32);
	io.read(drv->parent, 32);
	io.read(drv->longname, 128);
	drv->year = io.readVal<uint32>(); // TODO: LE byte-swap on uint32 reads
	iterateTimes(10, i)
		drv->romsize[i] = io.readVal<uint32>();
	drv->nb_romfile = io.readVal<uint32>();
	iterateTimes(drv->nb_romfile, i)
	{
		io.read(drv->rom[i].filename, 32);
		drv->rom[i].region = io.readVal<uint8>();
		drv->rom[i].src = io.readVal<uint32>();
		drv->rom[i].dest = io.readVal<uint32>();
		drv->rom[i].size = io.readVal<uint32>();
		drv->rom[i].crc = io.readVal<uint32>();
	}
	return drv;
}

CLINK void *res_load_data(const char *name)
{
	auto io = openGngeoDataIO(name);
	if(!io)
	{
		logErr("Can't data file %s", name);
		return nullptr;
	}
	auto size = io.size();
	auto buffer = (char*)malloc(size);
	io.read(buffer, size);
	return buffer;
}

EmuSystem::Error EmuSystem::loadGame(IO &, OnLoadProgressDelegate onLoadProgressFunc)
{
	onLoadProgress = onLoadProgressFunc;
	auto resetOnLoadProgress = IG::scopeGuard([&](){ onLoadProgress = {}; });
	ROM_DEF *drv = res_load_drv(gameName().data());
	if(!drv)
	{
		return makeError("This game isn't recognized");
	}
	auto freeDrv = IG::scopeGuard([&](){ free(drv); });
	logMsg("rom set %s, %s", drv->name, drv->longname);
	FS::PathString gnoFilename{};
	string_printf(gnoFilename, "%s/%s.gno", EmuSystem::savePath(), drv->name);
	if(optionCreateAndUseCache && FS::exists(gnoFilename))
	{
		logMsg("loading .gno file");
		char errorStr[1024];
		if(!init_game(gnoFilename.data(), errorStr))
		{
			return makeError("%s", errorStr);
		}
	}
	else
	{
		char errorStr[1024];
		if(!init_game(drv->name, errorStr))
		{
			return makeError("%s", errorStr);
		}

		if(optionCreateAndUseCache && !FS::exists(gnoFilename))
		{
			logMsg("%s doesn't exist, creating", gnoFilename.data());
			#ifdef USE_GENERATOR68K
			bool swappedBIOS = swapCPUMemForDump();
			#endif
			dr_save_gno(&memory.rom, gnoFilename.data());
			#ifdef USE_GENERATOR68K
			reverseSwapCPUMemForDump(swappedBIOS);
			#endif
		}
	}
	EmuSystem::setFullGameName(drv->longname);
	logMsg("set long game name: %s", EmuSystem::fullGameName().data());
	setTimerIntOption();
	return {};
}

void EmuSystem::onPrepareVideo(EmuVideo &video)
{
	video.setFormat(srcPix);
}

void EmuSystem::configAudioRate(double frameTime, int rate)
{
	conf.sample_rate = std::round(rate * ((60./1.001) * frameTime));
	if(gameIsRunning())
	{
		logMsg("setting YM2610 rate to %d", conf.sample_rate);
		YM2610ChangeSamplerate(conf.sample_rate);
	}
}

CLINK void screen_update()
{
	if(likely(emuVideo))
	{
		//logMsg("screen render");
		emuVideo->writeFrame(srcPix);
		EmuApp::updateAndDrawEmuVideo();
		emuVideo = {};
	}
	else
	{
		//logMsg("skipping render");
	}
}

void EmuSystem::runFrame(EmuVideo &video, bool renderGfx, bool processGfx, bool renderAudio)
{
	//logMsg("run frame %d", (int)processGfx);
	emuVideo = &video;
	skip_this_frame = !processGfx;
	if(processGfx)
		IG::fillData(screenBuff, (uint16)current_pc_pal[4095]);
	main_frame();
	YM2610Update_stream(audioFramesPerVideoFrame);
	if(renderAudio)
	{
		writeSound(play_buffer, audioFramesPerVideoFrame);
	}
}

FS::FileString EmuSystem::fullGameNameForPath(const char *path)
{
	auto gameName = fullGameNameForPathDefaultImpl(path);
	ROM_DEF *drv = res_load_drv(gameName.data());
	if(!drv)
		return gameName;
	auto freeDrv = IG::scopeGuard([&](){ free(drv); });
	return FS::makeFileString(drv->longname);
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((255./255.) * .4, (215./255.) * .4, (0./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((85./255.) * .4, (71./255.) * .4, (0./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

EmuSystem::Error EmuSystem::onInit()
{
	visible_area.x = 0;//16;
	visible_area.y = 16;
	visible_area.w = 304;//320;
	visible_area.h = 224;
	sdlSurf.pitch = FBResX*2;
	sdlSurf.w = FBResX;
	sdlSurf.pixels = screenBuff;
	buffer = &sdlSurf;
	conf.sound = 1;
	strcpy(rompathConfItem.data.dt_str.str, ".");
	if(!Config::envIsAndroid)
	{
		string_printf(datafilePath, "%s/gngeo_data.zip", Base::assetPath().data());
	}
	return {};
}

