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

#include <imagine/gui/TableView.hh>
#include <imagine/util/container/ArrayList.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/gui/TextTableView.hh>

class TouchConfigView : public TableView
{
public:
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	TextMenuItem touchCtrlItem[3];
	MultiChoiceMenuItem touchCtrl;
	TextMenuItem pointerInputItem[5];
	MultiChoiceMenuItem pointerInput;
	TextMenuItem sizeItem[11];
	MultiChoiceMenuItem size;
	TextMenuItem deadzoneItem[3];
	MultiChoiceMenuItem deadzone;
	TextMenuItem diagonalSensitivityItem[5];
	MultiChoiceMenuItem diagonalSensitivity;
	TextMenuItem btnSpaceItem[4];
	MultiChoiceMenuItem btnSpace;
	TextMenuItem btnExtraXSizeItem[4];
	MultiChoiceMenuItem btnExtraXSize;
	TextMenuItem btnExtraYSizeMultiRowItem[4];
	MultiChoiceMenuItem btnExtraYSizeMultiRow;
	TextMenuItem btnExtraYSizeItem[4];
	MultiChoiceMenuItem btnExtraYSize;
	BoolMenuItem triggerPos;
	TextMenuItem btnStaggerItem[6];
	MultiChoiceMenuItem btnStagger;
	TextMenuItem dPadStateItem[3];
	MultiChoiceMenuItem dPadState;
	TextMenuItem faceBtnStateItem[3];
	MultiChoiceMenuItem faceBtnState;
	TextMenuItem centerBtnStateItem[3];
	MultiChoiceMenuItem centerBtnState;
	TextMenuItem lTriggerStateItem[3];
	MultiChoiceMenuItem lTriggerState;
	TextMenuItem rTriggerStateItem[3];
	MultiChoiceMenuItem rTriggerState;
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLLER_RESOLUTION_CHANGE
		BoolMenuItem imageResolution;
		#endif
	BoolMenuItem boundingBoxes;
	BoolMenuItem vibrate;
		#ifdef CONFIG_BASE_ANDROID
		BoolMenuItem useScaledCoordinates;
		#endif
	BoolMenuItem showOnTouch;
	#endif
	TextMenuItem alphaItem[6];
	MultiChoiceMenuItem alpha;
	TextMenuItem btnPlace;
	TextMenuItem menuStateItem[3];
	MultiChoiceMenuItem menuState;
	TextMenuItem ffStateItem[3];
	MultiChoiceMenuItem ffState;
	TextMenuItem resetControls;
	TextMenuItem resetAllControls;
	TextMenuItem systemOptions;
	TextHeadingMenuItem btnTogglesHeading;
	TextHeadingMenuItem dpadtHeading;
	TextHeadingMenuItem faceBtnHeading;
	TextHeadingMenuItem otherHeading;
	StaticArrayList<MenuItem*, 32> item{};

	void refreshTouchConfigMenu();

public:
	TouchConfigView(Base::Window &win, const char *faceBtnName, const char *centerBtnName);
	void place() override;
	void draw() override;
};
