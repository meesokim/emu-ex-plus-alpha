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

#include <emuframework/EmuApp.hh>
#include "internal.hh"

extern "C"
{
	#include <gngeo/memory.h>
}

enum
{
	neogeoKeyIdxUp = EmuControls::systemKeyMapStart,
	neogeoKeyIdxRight,
	neogeoKeyIdxDown,
	neogeoKeyIdxLeft,
	neogeoKeyIdxLeftUp,
	neogeoKeyIdxRightUp,
	neogeoKeyIdxRightDown,
	neogeoKeyIdxLeftDown,
	neogeoKeyIdxSelect,
	neogeoKeyIdxStart,
	neogeoKeyIdxA,
	neogeoKeyIdxB,
	neogeoKeyIdxX,
	neogeoKeyIdxY,
	neogeoKeyIdxATurbo,
	neogeoKeyIdxBTurbo,
	neogeoKeyIdxXTurbo,
	neogeoKeyIdxYTurbo,
	neogeoKeyIdxABC,
	neogeoKeyIdxTestSwitch = EmuControls::systemKeyMapStart + EmuControls::joystickKeys*2
};

const char *EmuSystem::inputFaceBtnName = "A/B/C/D";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const uint EmuSystem::inputFaceBtns = 4;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const uint EmuSystem::maxPlayers = 2;

namespace NGKey
{

using namespace IG;

static const uint COIN1 = bit(0), COIN2 = bit(1), SERVICE = bit(2),
	START1 = bit(0), SELECT1 = bit(1),
	START2 = bit(2), SELECT2 = bit(3),

	UP = bit(0), DOWN = bit(1), LEFT = bit(2), RIGHT = bit(3),
	A = bit(4), B = bit(5), C = bit(6), D = bit(7),

	START_EMU_INPUT = bit(8),
	SELECT_COIN_EMU_INPUT = bit(9),
	SERVICE_EMU_INPUT = bit(10);

}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	using namespace NGKey;
	uint playerMask = player << 11;
	map[SysVController::F_ELEM] = A | playerMask;
	map[SysVController::F_ELEM+1] = B | playerMask;
	map[SysVController::F_ELEM+2] = C | playerMask;
	map[SysVController::F_ELEM+3] = D | playerMask;

	map[SysVController::C_ELEM] = SELECT_COIN_EMU_INPUT | playerMask;
	map[SysVController::C_ELEM+1] = START_EMU_INPUT | playerMask;

	map[SysVController::D_ELEM] = UP | LEFT | playerMask;
	map[SysVController::D_ELEM+1] = UP | playerMask;
	map[SysVController::D_ELEM+2] = UP | RIGHT | playerMask;
	map[SysVController::D_ELEM+3] = LEFT | playerMask;
	map[SysVController::D_ELEM+5] = RIGHT | playerMask;
	map[SysVController::D_ELEM+6] = DOWN | LEFT | playerMask;
	map[SysVController::D_ELEM+7] = DOWN | playerMask;
	map[SysVController::D_ELEM+8] = DOWN | RIGHT | playerMask;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	using namespace NGKey;
	if(unlikely(input == neogeoKeyIdxTestSwitch))
	{
		return SERVICE_EMU_INPUT;
	}
	assert(input >= neogeoKeyIdxUp);
	uint player = (input - neogeoKeyIdxUp) / EmuControls::joystickKeys;
	uint playerMask = player << 11;
	input -= EmuControls::joystickKeys * player;
	switch(input)
	{
		case neogeoKeyIdxUp: return UP | playerMask;
		case neogeoKeyIdxRight: return RIGHT | playerMask;
		case neogeoKeyIdxDown: return DOWN | playerMask;
		case neogeoKeyIdxLeft: return LEFT | playerMask;
		case neogeoKeyIdxLeftUp: return LEFT | UP | playerMask;
		case neogeoKeyIdxRightUp: return RIGHT | UP | playerMask;
		case neogeoKeyIdxRightDown: return RIGHT | DOWN | playerMask;
		case neogeoKeyIdxLeftDown: return LEFT | DOWN | playerMask;
		case neogeoKeyIdxSelect: return SELECT_COIN_EMU_INPUT | playerMask;
		case neogeoKeyIdxStart: return START_EMU_INPUT | playerMask;
		case neogeoKeyIdxXTurbo: turbo = 1;
		case neogeoKeyIdxX: return C | playerMask;
		case neogeoKeyIdxYTurbo: turbo = 1;
		case neogeoKeyIdxY: return D | playerMask;
		case neogeoKeyIdxATurbo: turbo = 1;
		case neogeoKeyIdxA: return A | playerMask;
		case neogeoKeyIdxBTurbo: turbo = 1;
		case neogeoKeyIdxB: return B | playerMask;
		case neogeoKeyIdxABC: return A | B | C | playerMask;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = emuKey >> 11;

	if(emuKey & 0xFF) // joystick
	{
		auto &p = player ? memory.intern_p2 : memory.intern_p1;
		p = IG::setOrClearBits(p, (Uint8)(emuKey & 0xFF), state != Input::PUSHED);
		return;
	}

	if(emuKey & NGKey::SELECT_COIN_EMU_INPUT)
	{
		if(conf.system == SYS_ARCADE)
		{
			uint bits = player ? NGKey::COIN2 : NGKey::COIN1;
			memory.intern_coin = IG::setOrClearBits(memory.intern_coin, (Uint8)bits, state != Input::PUSHED);
		}
		else
		{
			// convert COIN to SELECT
			uint bits = player ? NGKey::SELECT2 : NGKey::SELECT1;
			memory.intern_start = IG::setOrClearBits(memory.intern_start, (Uint8)bits, state != Input::PUSHED);
		}
		return;
	}

	if(emuKey & NGKey::START_EMU_INPUT)
	{
		uint bits = player ? NGKey::START2 : NGKey::START1;
		memory.intern_start = IG::setOrClearBits(memory.intern_start, (Uint8)bits, state != Input::PUSHED);
		return;
	}

	if(emuKey & NGKey::SERVICE_EMU_INPUT)
	{
		if(state == Input::PUSHED)
			conf.test_switch = 1; // Test Switch is reset to 0 after every frame
		return;
	}
}

void EmuSystem::clearInputBuffers()
{
	memory.intern_coin = 0x7;
	memory.intern_start = 0x8F;
	memory.intern_p1 = 0xFF;
	memory.intern_p2 = 0xFF;
}
