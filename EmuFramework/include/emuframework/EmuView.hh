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

#include <imagine/gui/View.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuInputView.hh>

class EmuView : public View
{
public:
	EmuView(ViewAttachParams attach, EmuVideoLayer *layer, EmuInputView *inputView);
	IG::WindowRect &viewRect() final { return rect; }
	void place() final;
	void draw() final;
	bool inputEvent(Input::Event e) final;
	void onAddedToController(Input::Event e) final {}
	bool hasLayer() const { return layer; }
	void swapLayers(EmuView &view);

private:
	EmuVideoLayer *layer{};
	EmuInputView *inputView{};
	IG::WindowRect rect{};
};
