/*  This file is part of PCE.emu.

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

#include <emuframework/EmuApp.hh>
#include "MDFN.hh"
#include "internal.hh"

const char *EmuSystem::configFilename = "PceEmu.config";
Byte1Option optionArcadeCard{CFGKEY_ARCADE_CARD, 1};
static PathOption optionSysCardPath{CFGKEY_SYSCARD_PATH, sysCardPath, ""};

const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_ARCADE_CARD: optionArcadeCard.readFromIO(io, readSize);
		bcase CFGKEY_SYSCARD_PATH: optionSysCardPath.readFromIO(io, readSize);
		logMsg("syscard path %s", sysCardPath.data());
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionArcadeCard.writeWithKeyIfNotDefault(io);
	optionSysCardPath.writeToIO(io);
}
