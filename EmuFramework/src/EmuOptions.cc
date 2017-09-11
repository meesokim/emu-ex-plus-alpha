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

#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/VideoImageEffect.hh>
#include <emuframework/VController.hh>
#include "private.hh"
#include "privateInput.hh"
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
extern SysVController vController;
#endif

template<class T>
bool optionFrameTimeIsValid(T val)
{
	return 0 || EmuSystem::frameTimeIsValid(EmuSystem::VIDSYS_NATIVE_NTSC, val);
}

template<class T>
bool optionFrameTimePALIsValid(T val)
{
	return 0 || EmuSystem::frameTimeIsValid(EmuSystem::VIDSYS_PAL, val);
}

bool optionOrientationIsValid(uint32 val)
{
	return val == Base::VIEW_ROTATE_AUTO ||
			val == Base::VIEW_ROTATE_0 ||
			val == Base::VIEW_ROTATE_90 ||
			val == Base::VIEW_ROTATE_180 ||
			val == Base::VIEW_ROTATE_270;
}

Byte1Option optionAutoSaveState(CFGKEY_AUTO_SAVE_STATE, 1);
Byte1Option optionConfirmAutoLoadState(CFGKEY_CONFIRM_AUTO_LOAD_STATE, 1);
Byte1Option optionSound(CFGKEY_SOUND, 1);

#ifdef CONFIG_AUDIO_LATENCY_HINT
Byte1Option optionSoundBuffers(CFGKEY_SOUND_BUFFERS,
	Config::envIsLinux ? 5 : Config::envIsIOS ? 5 : 8,
	0, optionIsValidWithMinMax<OPTION_SOUND_BUFFERS_MIN, 12, uint8>);
#endif

#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
OptionAudioHintStrictUnderrunCheck optionSoundUnderrunCheck(CFGKEY_SOUND_UNDERRUN_CHECK, 1);
#endif

#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
OptionAudioSoloMix optionAudioSoloMix(CFGKEY_AUDIO_SOLO_MIX, 1);
#endif

Byte4Option optionSoundRate(CFGKEY_SOUND_RATE,
	(Config::envIsPS3 || (Config::envIsLinux && !Config::MACHINE_IS_PANDORA)) ? 48000 : 44100, Config::envIsPS3, optionIsValidWithMax<48000>);

// Store in micro-meters
Byte2Option optionFontSize(CFGKEY_FONT_Y_SIZE,
	Config::MACHINE_IS_PANDORA ? 6500 :
	Config::MACHINE_IS_OUYA ? 3000 :
	Config::envIsWebOS3 ? 5000 :
	(Config::envIsIOS || Config::envIsAndroid || Config::envIsWebOS) ? 3000 :
	8000,
	0, optionIsValidWithMinMax<2000, 10500, uint16>);

Byte1Option optionVibrateOnPush(CFGKEY_TOUCH_CONTROL_VIRBRATE, 0, !Config::BASE_SUPPORTS_VIBRATOR);

Byte1Option optionPauseUnfocused(CFGKEY_PAUSE_UNFOCUSED, 1,
	!(Config::envIsPS3 || Config::envIsLinux || (Config::envIsAndroid && !Config::MACHINE_IS_OUYA)));

Byte1Option optionNotificationIcon(CFGKEY_NOTIFICATION_ICON, 1, !Config::envIsAndroid || Config::MACHINE_IS_OUYA);
Byte1Option optionTitleBar(CFGKEY_TITLE_BAR, 1, Config::envIsIOS || Config::envIsWebOS3);

OptionBackNavigation
	optionBackNavigation(CFGKEY_BACK_NAVIGATION, View::needsBackControlDefault, View::needsBackControlIsConst);

Byte1Option optionRememberLastMenu(CFGKEY_REMEMBER_LAST_MENU, 1, 0);
Byte1Option optionLowProfileOSNav(CFGKEY_LOW_PROFILE_OS_NAV, 1, !Config::envIsAndroid);
Byte1Option optionHideOSNav(CFGKEY_HIDE_OS_NAV, 0, !Config::envIsAndroid);
Byte1Option optionIdleDisplayPowerSave(CFGKEY_IDLE_DISPLAY_POWER_SAVE, 1, !Config::envIsAndroid && !Config::envIsIOS);
Byte1Option optionHideStatusBar(CFGKEY_HIDE_STATUS_BAR, 1, (!Config::envIsAndroid || Config::MACHINE_IS_OUYA) && !Config::envIsIOS);
OptionSwappedGamepadConfirm optionSwappedGamepadConfirm(CFGKEY_SWAPPED_GAMEPAD_CONFIM, Input::SWAPPED_GAMEPAD_CONFIRM_DEFAULT);
Byte1Option optionConfirmOverwriteState(CFGKEY_CONFIRM_OVERWRITE_STATE, 1, 0);
Byte1Option optionFastForwardSpeed(CFGKEY_FAST_FORWARD_SPEED, 4, 0, optionIsValidWithMinMax<2, 7>);
#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
Byte1Option optionNotifyInputDeviceChange(CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE, Config::Input::DEVICE_HOTSWAP, !Config::Input::DEVICE_HOTSWAP);
#endif
#ifdef CONFIG_INPUT_ANDROID_MOGA
Byte1Option optionMOGAInputSystem(CFGKEY_MOGA_INPUT_SYSTEM, 0, 0);
#endif

#ifdef CONFIG_BLUETOOTH
Byte1Option optionKeepBluetoothActive(CFGKEY_KEEP_BLUETOOTH_ACTIVE, 0, !Config::BASE_CAN_BACKGROUND_APP);
Byte1Option optionShowBluetoothScan(CFGKEY_SHOW_BLUETOOTH_SCAN, 1);
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	OptionBlueToothScanCache optionBlueToothScanCache(CFGKEY_BLUETOOTH_SCAN_CACHE, 1);
	#endif
#endif

OptionAspectRatio optionAspectRatio(EmuSystem::aspectRatioInfo[0].aspect);

Byte1Option optionImgFilter(CFGKEY_GAME_IMG_FILTER, 1, 0);
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
Byte1Option optionImgEffect(CFGKEY_IMAGE_EFFECT, 0, 0, optionIsValidWithMax<VideoImageEffect::LAST_EFFECT_VAL-1>);
#endif
Byte1Option optionOverlayEffect(CFGKEY_OVERLAY_EFFECT, 0, 0, optionIsValidWithMax<VideoImageOverlay::MAX_EFFECT_VAL>);
Byte1Option optionOverlayEffectLevel(CFGKEY_OVERLAY_EFFECT_LEVEL, 25, 0, optionIsValidWithMax<100>);

bool imageEffectPixelFormatIsValid(uint8 val)
{
	switch(val)
	{
		case IG::PIXEL_NONE:
		case IG::PIXEL_RGB565:
		case IG::PIXEL_RGBA8888:
			return true;
	}
	return false;
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
Byte1Option optionImageEffectPixelFormat(CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT, IG::PIXEL_NONE, 0, imageEffectPixelFormatIsValid);
#endif

#ifdef CONFIG_INPUT_RELATIVE_MOTION_DEVICES
Byte4Option optionRelPointerDecel(CFGKEY_REL_POINTER_DECEL, optionRelPointerDecelMed,
		!Config::envIsAndroid, optionIsValidWithMax<optionRelPointerDecelHigh>);
#endif

Byte4s1Option optionGameOrientation(CFGKEY_GAME_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Base::VIEW_ROTATE_AUTO : Config::envIsWebOS ? Base::VIEW_ROTATE_90 : Base::VIEW_ROTATE_0,
		Config::envIsPS3 || Config::MACHINE_IS_OUYA, optionOrientationIsValid);

Byte4s1Option optionMenuOrientation(CFGKEY_MENU_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Base::VIEW_ROTATE_AUTO : Base::VIEW_ROTATE_0,
		Config::envIsPS3, optionOrientationIsValid);

Byte1Option optionTouchCtrl(CFGKEY_TOUCH_CONTROL_DISPLAY,
		(Config::envIsLinux || Config::envIsPS3 || Config::MACHINE_IS_OUYA) ? 0 : 2,
		Config::envIsPS3 || Config::MACHINE_IS_OUYA, optionIsValidWithMax<2>);

Byte1Option optionTouchCtrlAlpha(CFGKEY_TOUCH_CONTROL_ALPHA,
		255 * .5,
		Config::envIsPS3/*, optionIsValidWithMax<255>*/);

Byte4s2Option optionTouchCtrlSize
		(CFGKEY_TOUCH_CONTROL_SIZE,
		(Config::envIsWebOS && !Config::envIsWebOS3) ? 800 : Config::envIsWebOS3 ? 1400 : 850,
		Config::envIsPS3, optionIsValidWithMax<1600>);
Byte4s2Option optionTouchDpadDeadzone
		(CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE,
		135,
		Config::envIsPS3, optionIsValidWithMax<160>);
Byte4s2Option optionTouchDpadDiagonalSensitivity
		(CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY,
		1750,
		Config::envIsPS3, optionIsValidWithMinMax<1000,2500>);
Byte4s2Option optionTouchCtrlBtnSpace
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE,
		200,
		Config::envIsPS3, optionIsValidWithMax<400>);
Byte4s2Option optionTouchCtrlBtnStagger
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER,
		1,
		Config::envIsPS3, optionIsValidWithMax<5>);
Byte1Option optionTouchCtrlTriggerBtnPos
		(CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS,
		0, Config::envIsPS3);
Byte4s2Option optionTouchCtrlExtraXBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE,
		200, Config::envIsPS3, optionIsValidWithMax<1000>);
Byte4s2Option optionTouchCtrlExtraYBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE,
		1000, Config::envIsPS3, optionIsValidWithMax<1000>);
Byte4s2Option optionTouchCtrlExtraYBtnSizeMultiRow
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW,
		200, Config::envIsPS3, optionIsValidWithMax<1000>);

bool isValidOption2DO(_2DOrigin val)
{
	return val.isValid() && val != C2DO;
}
bool isValidOption2DOCenterBtn(_2DOrigin val)
{
	return val.isValid() && !val.onYCenter();
}

Byte1Option optionTouchCtrlBoundingBoxes(CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES, 0);
Byte1Option optionTouchCtrlShowOnTouch(CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH, 1);
OptionTouchCtrlScaledCoordinates optionTouchCtrlScaledCoordinates(CFGKEY_TOUCH_CONTROL_SCALED_COORDINATES, 1, !Config::envIsAndroid);

OptionVControllerLayoutPosition optionVControllerLayoutPos;

#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
Byte1Option optionFrameInterval
	{CFGKEY_FRAME_INTERVAL,	1, !Config::envIsIOS, optionIsValidWithMinMax<1, 4>};
#endif
Byte1Option optionSkipLateFrames{CFGKEY_SKIP_LATE_FRAMES, 1, 0};
DoubleOption optionFrameRate{CFGKEY_FRAME_RATE, 0, 0, optionFrameTimeIsValid};
DoubleOption optionFrameRatePAL{CFGKEY_FRAME_RATE_PAL, 1./50., !EmuSystem::hasPALVideoSystem, optionFrameTimePALIsValid};

bool optionImageZoomIsValid(uint8 val)
{
	return val == optionImageZoomIntegerOnly || optionImageZoomIntegerOnlyY
		|| (val >= 10 && val <= 100);
}
Byte1Option optionImageZoom
		(CFGKEY_IMAGE_ZOOM, 100, 0, optionImageZoomIsValid);
Byte1Option optionViewportZoom(CFGKEY_VIEWPORT_ZOOM, 100, 0, optionIsValidWithMinMax<10, 100>);
Byte1Option optionShowOnSecondScreen{CFGKEY_SHOW_ON_2ND_SCREEN, 1, 0};

OptionRecentGames optionRecentGames;

#ifdef __ANDROID__
Byte1Option optionAndroidTextureStorage{CFGKEY_ANDROID_TEXTURE_STORAGE, OPTION_ANDROID_TEXTURE_STORAGE_AUTO,
	0, optionIsValidWithMax<OPTION_ANDROID_TEXTURE_STORAGE_MAX_VALUE>};
SByte1Option optionProcessPriority{CFGKEY_PROCESS_PRIORITY, -6, 0, optionIsValidWithMinMax<-17, 0>};
Byte1Option optionFakeUserActivity{CFGKEY_FAKE_USER_ACTIVITY, 1, 0};
#endif

Byte1Option optionDitherImage(CFGKEY_DITHER_IMAGE, 1, !Config::envIsAndroid);

#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
bool windowPixelFormatIsValid(uint8 val)
{
	switch(val)
	{
		case IG::PIXEL_NONE:
		case IG::PIXEL_RGB565:
		case IG::PIXEL_RGB888:
		case IG::PIXEL_RGBX8888:
		case IG::PIXEL_RGBA8888:
			return true;
	}
	return false;
}

Byte1Option optionWindowPixelFormat(CFGKEY_WINDOW_PIXEL_FORMAT, IG::PIXEL_NONE, 0, windowPixelFormatIsValid);
#endif

PathOption optionSavePath(CFGKEY_SAVE_PATH, EmuSystem::savePath_, "");
PathOption optionLastLoadPath(CFGKEY_LAST_DIR, lastLoadPath, "");
Byte1Option optionCheckSavePathWriteAccess{CFGKEY_CHECK_SAVE_PATH_WRITE_ACCESS, 1};

Byte1Option optionShowBundledGames(CFGKEY_SHOW_BUNDLED_GAMES, 1);

[[gnu::weak]] PathOption optionFirmwarePath(0, nullptr, 0, nullptr);

void initOptions()
{
	if(!strlen(lastLoadPath.data()))
	{
		lastLoadPath = Base::storagePath();
	}

	optionSoundRate.initDefault(AudioManager::nativeFormat().rate);

	#ifdef CONFIG_BASE_IOS
	if(Base::deviceIsIPad())
		optionFontSize.initDefault(5000);
	#endif

	#ifdef CONFIG_INPUT_ANDROID_MOGA
	if(Base::packageIsInstalled("com.bda.pivot.mogapgp"))
	{
		logMsg("MOGA Pivot app is present");
		optionMOGAInputSystem.initDefault(1);
	}
	#endif

	#ifdef CONFIG_BASE_ANDROID
	if(Base::hasHardwareNavButtons())
	{
		optionLowProfileOSNav.isConst = 1;
		optionHideOSNav.isConst = 1;
	}
	else
	{
		optionBackNavigation.initDefault(1);
		if(Base::androidSDK() >= 19)
			optionHideOSNav.initDefault(1);
	}
	if(Base::androidSDK() >= 11)
	{
		optionNotificationIcon.initDefault(0);
		// don't change dither setting on Android 3.0+
		optionDitherImage.isConst = 1;
	}
	else
	{
		optionDitherImage.initDefault(0);
	}
	if(Base::androidSDK() < 12)
	{
		#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
		optionNotifyInputDeviceChange.isConst = 1;
		#endif
	}
	if(!Base::hasVibrator())
	{
		optionVibrateOnPush.isConst = 1;
	}
	if(AudioManager::hasLowLatency()) // setup for low-latency
	{
		#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
		optionSoundUnderrunCheck.initDefault(0);
		#endif
		optionSoundBuffers.initDefault(6);
	}
	else if(Config::MACHINE_IS_GENERIC_ARMV7 && string_equal(Base::androidBuildDevice().data(), "roth")) // NVidia Shield
	{
		#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
		optionSoundUnderrunCheck.initDefault(0);
		#endif
		optionSoundBuffers.initDefault(4);
	}
	if(Base::androidSDK() < 17)
	{
		optionShowOnSecondScreen.isConst = true;
	}
	else
	{
		#ifdef CONFIG_BLUETOOTH
		optionShowBluetoothScan.initDefault(0);
		#endif
	}
	if(Base::androidSDK() < 16)
	{
		optionFakeUserActivity.initDefault(0);
		optionFakeUserActivity.isConst = true;
	}
	if(Base::androidSDK() < 11)
	{
		// never run app in onPaused state on Android 2.3
		optionPauseUnfocused.isConst = true;
	}
	#endif

	if(!Base::Screen::screen(0)->frameRateIsReliable())
	{
		optionFrameRate.initDefault(60);
	}

	if(!EmuApp::autoSaveStateDefault)
	{
		optionAutoSaveState.initDefault(false);
	}

	if(!EmuApp::hasIcon)
	{
		optionNotificationIcon.initDefault(false);
		optionNotificationIcon.isConst = true;
	}

	if(EmuSystem::forcedSoundRate)
	{
		optionSoundRate.initDefault(EmuSystem::forcedSoundRate);
		optionSoundRate.isConst = true;
	}

	if(!EmuSystem::hasSound)
	{
		optionSound.initDefault(false);
	}

	if(EmuSystem::constFrameRate)
	{
		optionFrameRate.isConst = true;
	}

	EmuSystem::initOptions();

	bool defaultToLargeControls = false;
	#ifdef CONFIG_BASE_IOS
	if(Base::deviceIsIPad())
		defaultToLargeControls = true;
	#endif
	if(Config::envIsWebOS3)
		defaultToLargeControls = true;
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(defaultToLargeControls)
		optionTouchCtrlSize.initDefault(1400);
	#endif
}

bool OptionVControllerLayoutPosition::isDefault() const
{
	return !vControllerLayoutPosChanged;
}

bool OptionVControllerLayoutPosition::writeToIO(IO &io)
{
	logMsg("writing vcontroller positions");
	std::error_code ec{};
	io.writeVal(key, &ec);
	for(auto &posArr : vControllerLayoutPos)
	{
		for(auto &e : posArr)
		{
			io.writeVal((uint8)e.origin, &ec);
			io.writeVal((uint8)e.state, &ec);
			io.writeVal((int32)e.pos.x, &ec);
			io.writeVal((int32)e.pos.y, &ec);
		}
	}
	return 1;
}

static uint sizeofVControllerLayoutPositionEntry()
{
	return 1 + 1 + 4 + 4;
}

bool OptionVControllerLayoutPosition::readFromIO(IO &io, uint readSize_)
{
	int readSize = readSize_;

	for(auto &posArr : vControllerLayoutPos)
	{
		for(auto &e : posArr)
		{
			if(readSize < (int)sizeofVControllerLayoutPositionEntry())
			{
				logMsg("expected position data but only %d bytes left", readSize);
				break;
			}

			_2DOrigin origin = _2DOrigin{(uchar)io.readVal<int8>()};
			if(!origin.isValid())
			{
				logWarn("invalid v-controller origin from config file");
			}
			else
				e.origin = origin;
			uint state = io.readVal<int8>();
			if(state > 2)
			{
				logWarn("invalid v-controller state from config file");
			}
			else
				e.state = state;
			e.pos.x = io.readVal<int32>();
			e.pos.y = io.readVal<int32>();
			vControllerLayoutPosChanged = true;
			readSize -= sizeofVControllerLayoutPositionEntry();
		}
	}

	if(readSize)
	{
		logMsg("skipping excess %d bytes", readSize);
	}

	return 1;
}

uint OptionVControllerLayoutPosition::ioSize()
{
	uint positions = IG::size(vControllerLayoutPos[0]) * IG::size(vControllerLayoutPos);
	return sizeof(key) + positions * sizeofVControllerLayoutPositionEntry();
}

bool vControllerUseScaledCoordinates()
{
	#ifdef CONFIG_BASE_ANDROID
	return vController.useScaledCoordinates;
	#else
	return false;
	#endif
}

void setVControllerUseScaledCoordinates(bool on)
{
	#ifdef CONFIG_BASE_ANDROID
	vController.useScaledCoordinates = on;
	#endif
}

void setupFont(Gfx::Renderer &r)
{
	float size = optionFontSize / 1000.;
	logMsg("setting up font size %f", (double)size);
	View::defaultFace.setFontSettings(r, IG::FontSettings(mainWin.win.heightSMMInPixels(size)));
	View::defaultBoldFace.setFontSettings(r, IG::FontSettings(mainWin.win.heightSMMInPixels(size)));
}

#ifdef __ANDROID__
Gfx::Texture::AndroidStorageImpl makeAndroidStorageImpl(uint8 val)
{
	using namespace Gfx;
	switch(val)
	{
		case OPTION_ANDROID_TEXTURE_STORAGE_NONE: return Texture::ANDROID_NONE;
		case OPTION_ANDROID_TEXTURE_STORAGE_GRAPHIC_BUFFER: return Texture::ANDROID_GRAPHIC_BUFFER;
		case OPTION_ANDROID_TEXTURE_STORAGE_SURFACE_TEXTURE: return Texture::ANDROID_SURFACE_TEXTURE;
		default: return Texture::ANDROID_AUTO;
	}
}
#endif

bool OptionRecentGames::isDefault() const
{
	return recentGameList.size() == 0;
}

bool OptionRecentGames::writeToIO(IO &io)
{
	logMsg("writing recent list");
	std::error_code ec{};
	io.writeVal(key, &ec);
	for(auto &e : recentGameList)
	{
		uint len = strlen(e.path.data());
		io.writeVal((uint16)len, &ec);
		io.write(e.path.data(), len, &ec);
	}
	return true;
}

bool OptionRecentGames::readFromIO(IO &io, uint readSize_)
{
	int readSize = readSize_;
	while(readSize && !recentGameList.isFull())
	{
		if(readSize < 2)
		{
			logMsg("expected string length but only %d bytes left", readSize);
			break;
		}

		auto len = io.readVal<uint16>();
		readSize -= 2;

		if(len > readSize)
		{
			logMsg("string length %d longer than %d bytes left", len, readSize);
			break;
		}

		RecentGameInfo info;
		auto bytesRead = io.read(info.path.data(), len);
		if(bytesRead == -1)
		{
			logErr("error reading string option");
			return true;
		}
		if(!bytesRead)
			continue; // don't add empty paths
		info.path[bytesRead] = 0;
		readSize -= len;
		info.name = EmuSystem::fullGameNameForPath(info.path.data());
		//logMsg("adding game to recent list: %s, name: %s", info.path, info.name);
		recentGameList.push_back(info);
	}

	if(readSize)
	{
		logMsg("skipping excess %d bytes", readSize);
	}

	return true;
}

uint OptionRecentGames::ioSize()
{
	uint strSizes = 0;
	for(auto &e : recentGameList)
	{
		strSizes += 2;
		strSizes += strlen(e.path.data());
	}
	return sizeof(key) + strSizes;
}
