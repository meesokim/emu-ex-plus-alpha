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

#include <emuframework/ConfigFile.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/FileUtils.hh>
#include <imagine/base/Base.hh>
#include <imagine/io/FileIO.hh>

static bool readKeyConfig(IO &io, uint16 &size)
{
	auto confs = io.readVal<uint8>(); // TODO: unused currently, use to pre-allocate memory for configs
	size--;
	if(!size)
		return false;

	while(size)
	{
		KeyConfig keyConf{};

		keyConf.map = io.readVal<uint8>();
		size--;
		if(!size)
			return false;

		auto nameLen = io.readVal<uint8>();
		size--;
		if(size < nameLen)
			return false;

		if(nameLen > sizeof(keyConf.name)-1)
			return 0;
		if(io.read(keyConf.name, nameLen) != nameLen)
			return false;
		size -= nameLen;
		if(!size)
			return false;

		auto categories = io.readVal<uint8>();
		size--;
		if(categories > EmuControls::categories)
		{
			return false;
		}

		iterateTimes(categories, i)
		{
			if(!size)
				return false;

			auto categoryIdx = io.readVal<uint8>();
			size--;
			if(categoryIdx >= EmuControls::categories)
			{
				return false;
			}
			if(size < 2)
			{
				return false;
			}

			auto catSize = io.readVal<uint16>();
			size -= 2;
			if(size < catSize)
				return false;

			if(catSize > EmuControls::category[categoryIdx].keys * sizeof(KeyConfig::Key))
				return false;
			auto key = keyConf.key(EmuControls::category[categoryIdx]);
			if(io.read(key, catSize) != catSize)
				return false;
			size -= catSize;

			// verify keys
			{
				const auto keyMax = Input::Event::mapNumKeys(keyConf.map);
				iterateTimes(EmuControls::category[categoryIdx].keys, i)
				{
					if(key[i] >= keyMax)
					{
						logWarn("key code 0x%X out of range for map type %d", key[i], keyConf.map);
						key[i] = 0;
					}
				}
			}

			logMsg("read category %d", categoryIdx);
		}

		logMsg("read key config %s", keyConf.name);
		if(!customKeyConfig.addToEnd(keyConf))
			return false;
	}
	return true;
}

static bool readConfig2(IO &io)
{
	auto blockSize = io.readVal<uint8>();
	auto fileBytesLeft = io.size() - 1;

	if(blockSize != 2)
	{
		logErr("can't read config with block size %d", blockSize);
		return false;
	}

	while(!io.eof() && fileBytesLeft >= 2)
	{
		auto size = io.readVal<uint16>();
		auto nextBlockPos = io.tell() + size;

		if(!size)
		{
			logMsg("invalid 0 size block, skipping rest of config");
			return false;
		}

		if(size > fileBytesLeft)
		{
			logErr("size of key exceeds rest of file, skipping rest of config");
			return false;
		}
		fileBytesLeft -= size;

		if(size < 3) // all blocks are at least a 2 byte key + 1 byte or more of data
		{
			logMsg("skipping %d byte block", size);
			if(io.seekC(size) == -1)
			{
				logErr("unable to seek to next block, skipping rest of config");
				return false;
			}
			continue;
		}

		auto key = io.readVal<uint16>();
		size -= 2;

		logMsg("got config key %u, size %u", key, size);
		switch(key)
		{
			default:
			{
				if(!EmuSystem::readConfig(io, key, size))
				{
					logMsg("skipping unknown key %u", (uint)key);
				}
			}
			bcase CFGKEY_SOUND: optionSound.readFromIO(io, size);
			bcase CFGKEY_SOUND_RATE: optionSoundRate.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_ALPHA: optionTouchCtrlAlpha.readFromIO(io, size);
			#ifdef CONFIG_VCONTROLS_GAMEPAD
			bcase CFGKEY_TOUCH_CONTROL_DISPLAY: optionTouchCtrl.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_SIZE: optionTouchCtrlSize.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE: optionTouchCtrlBtnSpace.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER: optionTouchCtrlBtnStagger.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE: optionTouchDpadDeadzone.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS: optionTouchCtrlTriggerBtnPos.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY: optionTouchDpadDiagonalSensitivity.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE: optionTouchCtrlExtraXBtnSize.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE: optionTouchCtrlExtraYBtnSize.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW: optionTouchCtrlExtraYBtnSizeMultiRow.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES: optionTouchCtrlBoundingBoxes.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH: optionTouchCtrlShowOnTouch.readFromIO(io, size);
				#ifdef __ANDROID__
				bcase CFGKEY_TOUCH_CONTROL_SCALED_COORDINATES: optionTouchCtrlScaledCoordinates.readFromIO(io, size);
				#endif
			#endif
			bcase CFGKEY_VCONTROLLER_LAYOUT_POS: optionVControllerLayoutPos.readFromIO(io, size);
			bcase CFGKEY_AUTO_SAVE_STATE: optionAutoSaveState.readFromIO(io, size);
			bcase CFGKEY_CONFIRM_AUTO_LOAD_STATE: optionConfirmAutoLoadState.readFromIO(io, size);
			#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
			bcase CFGKEY_FRAME_INTERVAL: optionFrameInterval.readFromIO(io, size);
			#endif
			bcase CFGKEY_SKIP_LATE_FRAMES: optionSkipLateFrames.readFromIO(io, size);
			bcase CFGKEY_FRAME_RATE: optionFrameRate.readFromIO(io, size);
			bcase CFGKEY_FRAME_RATE_PAL: optionFrameRatePAL.readFromIO(io, size);
			#if defined(CONFIG_BASE_ANDROID)
			bcase CFGKEY_DITHER_IMAGE: optionDitherImage.readFromIO(io, size);
			#endif
			bcase CFGKEY_LAST_DIR: optionLastLoadPath.readFromIO(io, size);
			bcase CFGKEY_FONT_Y_SIZE: optionFontSize.readFromIO(io, size);
			bcase CFGKEY_GAME_ORIENTATION: optionGameOrientation.readFromIO(io, size);
			bcase CFGKEY_MENU_ORIENTATION: optionMenuOrientation.readFromIO(io, size);
			bcase CFGKEY_GAME_IMG_FILTER: optionImgFilter.readFromIO(io, size);
			bcase CFGKEY_GAME_ASPECT_RATIO: optionAspectRatio.readFromIO(io, size);
			bcase CFGKEY_IMAGE_ZOOM: optionImageZoom.readFromIO(io, size);
			bcase CFGKEY_VIEWPORT_ZOOM: optionViewportZoom.readFromIO(io, size);
			#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
			bcase CFGKEY_SHOW_ON_2ND_SCREEN: optionShowOnSecondScreen.readFromIO(io, size);
			#endif
			#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
			bcase CFGKEY_IMAGE_EFFECT: optionImgEffect.readFromIO(io, size);
			bcase CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT: optionImageEffectPixelFormat.readFromIO(io, size);
			#endif
			bcase CFGKEY_OVERLAY_EFFECT: optionOverlayEffect.readFromIO(io, size);
			bcase CFGKEY_OVERLAY_EFFECT_LEVEL: optionOverlayEffectLevel.readFromIO(io, size);
			bcase CFGKEY_TOUCH_CONTROL_VIRBRATE: optionVibrateOnPush.readFromIO(io, size);
			bcase CFGKEY_RECENT_GAMES: optionRecentGames.readFromIO(io, size);
			bcase CFGKEY_SWAPPED_GAMEPAD_CONFIM: optionSwappedGamepadConfirm.readFromIO(io, size);
			bcase CFGKEY_PAUSE_UNFOCUSED: optionPauseUnfocused.readFromIO(io, size);
			bcase CFGKEY_NOTIFICATION_ICON: optionNotificationIcon.readFromIO(io, size);
			bcase CFGKEY_TITLE_BAR: optionTitleBar.readFromIO(io, size);
			bcase CFGKEY_BACK_NAVIGATION: optionBackNavigation.readFromIO(io, size);
			bcase CFGKEY_REMEMBER_LAST_MENU: optionRememberLastMenu.readFromIO(io, size);
			bcase CFGKEY_IDLE_DISPLAY_POWER_SAVE: optionIdleDisplayPowerSave.readFromIO(io, size);
			bcase CFGKEY_HIDE_STATUS_BAR: optionHideStatusBar.readFromIO(io, size);
			bcase CFGKEY_CONFIRM_OVERWRITE_STATE: optionConfirmOverwriteState.readFromIO(io, size);
			bcase CFGKEY_FAST_FORWARD_SPEED: optionFastForwardSpeed.readFromIO(io, size);
			#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
			bcase CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE: optionNotifyInputDeviceChange.readFromIO(io, size);
			#endif
			#ifdef CONFIG_INPUT_ANDROID_MOGA
			bcase CFGKEY_MOGA_INPUT_SYSTEM: optionMOGAInputSystem.readFromIO(io, size);
			#endif
			#if defined __ANDROID__
			bcase CFGKEY_LOW_PROFILE_OS_NAV: optionLowProfileOSNav.readFromIO(io, size);
			bcase CFGKEY_HIDE_OS_NAV: optionHideOSNav.readFromIO(io, size);
			bcase CFGKEY_REL_POINTER_DECEL: optionRelPointerDecel.readFromIO(io, size);
			bcase CFGKEY_ANDROID_TEXTURE_STORAGE: optionAndroidTextureStorage.readFromIO(io, size);
			bcase CFGKEY_PROCESS_PRIORITY: optionProcessPriority.readFromIO(io, size);
			bcase CFGKEY_FAKE_USER_ACTIVITY: optionFakeUserActivity.readFromIO(io, size);
			#endif
			#ifdef CONFIG_BLUETOOTH
			bcase CFGKEY_KEEP_BLUETOOTH_ACTIVE: optionKeepBluetoothActive.readFromIO(io, size);
			bcase CFGKEY_SHOW_BLUETOOTH_SCAN: optionShowBluetoothScan.readFromIO(io, size);
				#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
				bcase CFGKEY_BLUETOOTH_SCAN_CACHE: optionBlueToothScanCache.readFromIO(io, size);
				#endif
			#endif
			#ifdef CONFIG_AUDIO_LATENCY_HINT
			bcase CFGKEY_SOUND_BUFFERS: optionSoundBuffers.readFromIO(io, size);
			#endif
			#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
			bcase CFGKEY_SOUND_UNDERRUN_CHECK: optionSoundUnderrunCheck.readFromIO(io, size);
			#endif
			#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
			bcase CFGKEY_AUDIO_SOLO_MIX: optionAudioSoloMix.readFromIO(io, size);
			#endif
			bcase CFGKEY_SAVE_PATH: logMsg("reading save path"); optionSavePath.readFromIO(io, size);
			bcase CFGKEY_CHECK_SAVE_PATH_WRITE_ACCESS: optionCheckSavePathWriteAccess.readFromIO(io, size);
			bcase CFGKEY_SHOW_BUNDLED_GAMES:
			{
				if(EmuSystem::hasBundledGames)
					optionShowBundledGames.readFromIO(io, size);
			}
			#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
			bcase CFGKEY_WINDOW_PIXEL_FORMAT: optionWindowPixelFormat.readFromIO(io, size);
			#endif
			bcase CFGKEY_INPUT_KEY_CONFIGS:
			{
				if(!readKeyConfig(io, size))
				{
					logErr("error reading key configs from file");
				}
				if(size)
				{
					// skip leftover bytes
					logWarn("%d bytes leftover reading key configs due to invalid data", size);
				}
			}
			bcase CFGKEY_INPUT_DEVICE_CONFIGS:
			{
				auto confs = io.readVal<uint8>(); // TODO: unused currently, use to pre-allocate memory for configs
				size--;
				if(!size)
					break;

				while(size)
				{
					InputDeviceSavedConfig devConf;

					devConf.enumId = io.readVal<uint8>();
					size--;
					if(!size)
						break;
					if(devConf.enumId > 32)
					{
						logWarn("unusually large device id %d, skipping rest of configs", devConf.enumId);
						break;
					}

					devConf.enabled = io.readVal<uint8>();
					size--;
					if(!size)
						break;

					devConf.player = io.readVal<uint8>();
					if(devConf.player != InputDeviceConfig::PLAYER_MULTI && devConf.player > EmuSystem::maxPlayers)
					{
						logWarn("player %d out of range", devConf.player);
						devConf.player = 0;
					}
					size--;
					if(!size)
						break;

					devConf.joystickAxisAsDpadBits = io.readVal<uint8>();
					size--;
					if(!size)
						break;

					#ifdef CONFIG_INPUT_ICADE
					devConf.iCadeMode = io.readVal<uint8>();
					size--;
					if(!size)
						break;
					#endif

					auto nameLen = io.readVal<uint8>();
					size--;
					if(size < nameLen)
						break;

					if(nameLen > sizeof(devConf.name)-1)
						break;
					io.read(devConf.name, nameLen);
					size -= nameLen;
					if(!size)
						break;

					auto keyConfMap = io.readVal<uint8>();
					size--;

					if(keyConfMap)
					{
						if(!size)
							break;

						auto keyConfNameLen = io.readVal<uint8>();
						size--;
						if(size < keyConfNameLen)
							break;

						if(keyConfNameLen > sizeof(devConf.name)-1)
							break;
						char keyConfName[sizeof(devConf.name)]{};
						if(io.read(keyConfName, keyConfNameLen) != keyConfNameLen)
							break;
						size -= keyConfNameLen;

						for(auto &e : customKeyConfig)
						{
							if(e.map == keyConfMap && string_equal(e.name, keyConfName))
							{
								logMsg("found referenced custom key config %s while reading input device config", keyConfName);
								devConf.keyConf = &e;
								break;
							}
						}

						if(!devConf.keyConf) // check built-in configs after user-defined ones
						{
							uint defaultConfs = 0;
							auto defaultConf = KeyConfig::defaultConfigsForInputMap(keyConfMap, defaultConfs);
							iterateTimes(defaultConfs, c)
							{
								if(string_equal(defaultConf[c].name, keyConfName))
								{
									logMsg("found referenced built-in key config %s while reading input device config", keyConfName);
									devConf.keyConf = &defaultConf[c];
									break;
								}
							}
						}
					}

					logMsg("read input device config %s, id %d", devConf.name, devConf.enumId);
					if(!savedInputDevList.addToEnd(devConf))
						break;
				}
				if(size)
				{
					// skip leftover bytes
					logWarn("%d bytes leftover reading input device configs due to invalid data", size);
				}
			}
		}

		if(io.seekS(nextBlockPos) == -1)
		{
			logErr("unable to seek to next block, skipping rest of config");
			return false;
		}
	}
	return true;
}

static OptionBase *cfgFileOption[] =
{
	&optionAutoSaveState,
	&optionConfirmAutoLoadState,
	&optionSound,
	&optionSoundRate,
	&optionAspectRatio,
	&optionImageZoom,
	&optionViewportZoom,
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	&optionShowOnSecondScreen,
	#endif
	&optionImgFilter,
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	&optionImgEffect,
	&optionImageEffectPixelFormat,
	#endif
	&optionOverlayEffect,
	&optionOverlayEffectLevel,
	#ifdef CONFIG_INPUT_RELATIVE_MOTION_DEVICES
	&optionRelPointerDecel,
	#endif
	&optionFontSize,
	&optionPauseUnfocused,
	&optionGameOrientation,
	&optionMenuOrientation,
	&optionTouchCtrlAlpha,
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	&optionTouchCtrl,
	&optionTouchCtrlSize,
	&optionTouchDpadDeadzone,
	&optionTouchCtrlBtnSpace,
	&optionTouchCtrlBtnStagger,
	&optionTouchCtrlTriggerBtnPos,
	&optionTouchCtrlExtraXBtnSize,
	&optionTouchCtrlExtraYBtnSize,
	&optionTouchCtrlExtraYBtnSizeMultiRow,
	&optionTouchDpadDiagonalSensitivity,
	&optionTouchCtrlBoundingBoxes,
	&optionTouchCtrlShowOnTouch,
		#ifdef __ANDROID__
		&optionTouchCtrlScaledCoordinates,
		#endif
	#endif
	&optionVControllerLayoutPos,
	&optionSwappedGamepadConfirm,
	&optionConfirmOverwriteState,
	&optionFastForwardSpeed,
	#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
	&optionNotifyInputDeviceChange,
	#endif
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	&optionMOGAInputSystem,
	#endif
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	&optionFrameInterval,
	#endif
	&optionSkipLateFrames,
	&optionFrameRate,
	&optionFrameRatePAL,
	&optionVibrateOnPush,
	&optionRecentGames,
	&optionNotificationIcon,
	&optionTitleBar,
	&optionIdleDisplayPowerSave,
	&optionHideStatusBar,
	#ifdef __ANDROID__
	&optionBackNavigation,
	#endif
	&optionRememberLastMenu,
	#if defined __ANDROID__
	&optionLowProfileOSNav,
	&optionHideOSNav,
	&optionDitherImage,
	&optionAndroidTextureStorage,
	&optionProcessPriority,
	&optionFakeUserActivity,
	#endif
	#ifdef CONFIG_BLUETOOTH
	&optionKeepBluetoothActive,
	&optionShowBluetoothScan,
		#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
		&optionBlueToothScanCache,
		#endif
	#endif
	#ifdef CONFIG_AUDIO_LATENCY_HINT
	&optionSoundBuffers,
	#endif
	#ifdef EMU_FRAMEWORK_STRICT_UNDERRUN_CHECK_OPTION
	&optionSoundUnderrunCheck,
	#endif
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	&optionAudioSoloMix,
	#endif
	#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	&optionWindowPixelFormat,
	#endif
	&optionShowBundledGames,
	&optionCheckSavePathWriteAccess
};

static void writeConfig2(IO &io)
{
	if(!io)
	{
		logMsg("not writing config file");
		return;
	}

	std::error_code ec{};
	uint8 blockHeaderSize = 2;
	io.writeVal(blockHeaderSize, &ec);

	for(auto &e : cfgFileOption)
	{
		if(!e->isDefault())
		{
			io.writeVal((uint16)e->ioSize(), &ec);
			e->writeToIO(io);
		}
	}

	if(customKeyConfig.size())
	{
		bool writeCategory[MAX_CUSTOM_KEY_CONFIGS][EmuControls::categories];
		uint8 writeCategories[MAX_CUSTOM_KEY_CONFIGS] {0};
		// compute total size
		static_assert(sizeof(KeyConfig::name) <= 255, "key config name array is too large");
		uint bytes = 2; // config key size
		uint8 configs = 0;
		bytes += 1; // number of configs
		for(auto &e : customKeyConfig)
		{
			bytes += 1; // input map type
			bytes += 1; // name string length
			bytes += strlen(e.name); // name string
			bytes += 1; // number of categories present
			iterateTimes(EmuControls::categories, cat)
			{
				bool write = 0;
				const auto key = e.key(EmuControls::category[cat]);
				iterateTimes(EmuControls::category[cat].keys, k)
				{
					if(key[k]) // check if category has any keys defined
					{
						write = 1;
						break;
					}
				}
				writeCategory[configs][cat] = write;
				if(!write)
				{
					logMsg("category %d of key conf %s skipped", cat, e.name);
					continue;
				}
				writeCategories[configs]++;
				bytes += 1; // category index
				bytes += 2; // category key array size
				bytes += EmuControls::category[cat].keys * sizeof(KeyConfig::Key); // keys array
			}
			configs++;
		}
		if(bytes > 0xFFFF)
		{
			bug_exit("excessive key config size, should not happen");
		}
		// write to config file
		logMsg("saving %d key configs, %d bytes", customKeyConfig.size(), bytes);
		io.writeVal(uint16(bytes), &ec);
		io.writeVal((uint16)CFGKEY_INPUT_KEY_CONFIGS, &ec);
		io.writeVal((uint8)customKeyConfig.size(), &ec);
		configs = 0;
		for(auto &e : customKeyConfig)
		{
			logMsg("writing config %s", e.name);
			io.writeVal(uint8(e.map), &ec);
			uint8 nameLen = strlen(e.name);
			io.writeVal(nameLen, &ec);
			io.write(e.name, nameLen, &ec);
			io.writeVal(writeCategories[configs], &ec);
			iterateTimes(EmuControls::categories, cat)
			{
				if(!writeCategory[configs][cat])
					continue;
				io.writeVal((uint8)cat, &ec);
				uint16 catSize = EmuControls::category[cat].keys * sizeof(KeyConfig::Key);
				io.writeVal(catSize, &ec);
				io.write(e.key(EmuControls::category[cat]), catSize, &ec);
			}
			configs++;
		}
	}

	if(savedInputDevList.size())
	{
		// input device configs must be saved after key configs since
		// they reference the key configs when read back from the config file

		// compute total size
		static_assert(sizeof(InputDeviceSavedConfig::name) <= 255, "input device config name array is too large");
		uint bytes = 2; // config key size
		bytes += 1; // number of configs
		for(auto &e : savedInputDevList)
		{
			bytes += 1; // device id
			bytes += 1; // enabled
			bytes += 1; // player
			bytes += 1; // mapJoystickAxis1ToDpad
			#ifdef CONFIG_INPUT_ICADE
			bytes += 1; // iCade mode
			#endif
			bytes += 1; // name string length
			bytes += strlen(e.name); // name string
			bytes += 1; // key config map
			if(e.keyConf)
			{
				bytes += 1; // name of key config string length
				bytes += strlen(e.keyConf->name); // name of key config string
			}
		}
		if(bytes > 0xFFFF)
		{
			bug_exit("excessive input device config size, should not happen");
		}
		// write to config file
		logMsg("saving %d input device configs, %d bytes", savedInputDevList.size(), bytes);
		io.writeVal((uint16)bytes, &ec);
		io.writeVal((uint16)CFGKEY_INPUT_DEVICE_CONFIGS, &ec);
		io.writeVal((uint8)savedInputDevList.size(), &ec);
		for(auto &e : savedInputDevList)
		{
			logMsg("writing config %s, id %d", e.name, e.enumId);
			io.writeVal((uint8)e.enumId, &ec);
			io.writeVal((uint8)e.enabled, &ec);
			io.writeVal((uint8)e.player, &ec);
			io.writeVal((uint8)e.joystickAxisAsDpadBits, &ec);
			#ifdef CONFIG_INPUT_ICADE
			io.writeVal((uint8)e.iCadeMode, &ec);
			#endif
			uint8 nameLen = strlen(e.name);
			io.writeVal(nameLen, &ec);
			io.write(e.name, nameLen, &ec);
			uint8 keyConfMap = e.keyConf ? e.keyConf->map : 0;
			io.writeVal(keyConfMap, &ec);
			if(keyConfMap)
			{
				logMsg("has key conf %s, map %d", e.keyConf->name, keyConfMap);
				uint8 keyConfNameLen = strlen(e.keyConf->name);
				io.writeVal(keyConfNameLen, &ec);
				io.write(e.keyConf->name, keyConfNameLen, &ec);
			}
		}
	}

	optionLastLoadPath.writeToIO(io);
	optionSavePath.writeToIO(io);

	EmuSystem::writeConfig(io);
}

void loadConfigFile()
{
	FS::PathString configFilePath;
	if(Base::documentsPathIsShared())
		string_printf(configFilePath, "%s/explusalpha.com/%s", Base::documentsPath().data(), EmuSystem::configFilename);
	else
		string_printf(configFilePath, "%s/config", Base::documentsPath().data());
	FileIO configFile;
	auto ec = configFile.open(configFilePath.data());
	if(ec)
	{
		logMsg("no config file");
		return;
	}
	readConfig2(configFile);
}

void saveConfigFile()
{
	FS::PathString configFilePath;
	if(Base::documentsPathIsShared())
	{
		auto documentsPath = Base::documentsPath();
		string_printf(configFilePath, "%s/explusalpha.com", documentsPath.data());
		FS::create_directory(configFilePath);
		fixFilePermissions(configFilePath);
		string_printf(configFilePath, "%s/explusalpha.com/%s", documentsPath.data(), EmuSystem::configFilename);
	}
	else
	{
		string_printf(configFilePath, "%s/config", Base::documentsPath().data());
	}
	FileIO configFile;
	configFile.create(configFilePath.data());
	writeConfig2(configFile);
}
