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

#include <emuframework/EmuApp.hh>
#include "internal.hh"

enum
{
	gbcKeyIdxUp = EmuControls::systemKeyMapStart,
	gbcKeyIdxRight,
	gbcKeyIdxDown,
	gbcKeyIdxLeft,
	gbcKeyIdxLeftUp,
	gbcKeyIdxRightUp,
	gbcKeyIdxRightDown,
	gbcKeyIdxLeftDown,
	gbcKeyIdxSelect,
	gbcKeyIdxStart,
	gbcKeyIdxA,
	gbcKeyIdxB,
	gbcKeyIdxATurbo,
	gbcKeyIdxBTurbo
};

const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Select/Start";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const uint EmuSystem::maxPlayers = 1;
GbcInput gbcInput{};

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	using namespace gambatte;
	map[SysVController::F_ELEM] = InputGetter::A;
	map[SysVController::F_ELEM+1] = InputGetter::B;

	map[SysVController::C_ELEM] = InputGetter::SELECT;
	map[SysVController::C_ELEM+1] = InputGetter::START;

	map[SysVController::D_ELEM] = InputGetter::UP | InputGetter::LEFT;
	map[SysVController::D_ELEM+1] = InputGetter::UP;
	map[SysVController::D_ELEM+2] = InputGetter::UP | InputGetter::RIGHT;
	map[SysVController::D_ELEM+3] = InputGetter::LEFT;
	map[SysVController::D_ELEM+5] = InputGetter::RIGHT;
	map[SysVController::D_ELEM+6] = InputGetter::DOWN | InputGetter::LEFT;
	map[SysVController::D_ELEM+7] = InputGetter::DOWN;
	map[SysVController::D_ELEM+8] = InputGetter::DOWN | InputGetter::RIGHT;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	using namespace gambatte;
	turbo = 0;
	switch(input)
	{
		case gbcKeyIdxUp: return InputGetter::UP;
		case gbcKeyIdxRight: return InputGetter::RIGHT;
		case gbcKeyIdxDown: return InputGetter::DOWN;
		case gbcKeyIdxLeft: return InputGetter::LEFT;
		case gbcKeyIdxLeftUp: return InputGetter::LEFT | InputGetter::UP;
		case gbcKeyIdxRightUp: return InputGetter::RIGHT | InputGetter::UP;
		case gbcKeyIdxRightDown: return InputGetter::RIGHT | InputGetter::DOWN;
		case gbcKeyIdxLeftDown: return InputGetter::LEFT | InputGetter::DOWN;
		case gbcKeyIdxSelect: return InputGetter::SELECT;
		case gbcKeyIdxStart: return InputGetter::START;
		case gbcKeyIdxATurbo: turbo = 1;
		case gbcKeyIdxA: return InputGetter::A;
		case gbcKeyIdxBTurbo: turbo = 1;
		case gbcKeyIdxB: return InputGetter::B;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	gbcInput.bits = IG::setOrClearBits(gbcInput.bits, emuKey, state == Input::PUSHED);
}

void EmuSystem::clearInputBuffers()
{
	gbcInput.bits = 0;
}
