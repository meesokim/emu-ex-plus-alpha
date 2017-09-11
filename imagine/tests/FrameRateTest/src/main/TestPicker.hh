#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include "tests.hh"

class TestTableEntry : public DualTextMenuItem
{
public:
	std::array<char, 64> testStr{};
	std::array<char, 9> fpsStr{};
	bool redText{};

	TestTableEntry(SelectDelegate selectDel): DualTextMenuItem{testStr.data(), nullptr, selectDel} {}
	void draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const final;
};

class TestPicker : public TableView
{
public:
	TestPicker(ViewAttachParams attach);
	void setTests(const TestParams *testParams, uint tests);

private:
	std::vector<TestTableEntry> testEntry{};
	const TestParams *testParamPtr{};
};
