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

#define LOGTAG "BaseAlertView"

#include <imagine/gui/AlertView.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math/int.hh>

BaseAlertView::BaseAlertView(ViewAttachParams attach, const char *label, TableView::ItemsDelegate items, TableView::ItemDelegate item):
	View{attach},
	text{label, &View::defaultFace},
	menu
	{
		attach,
		items,
		item
	}
{
	menu.setAlign(C2DO);
	menu.setScrollableIfNeeded(true);
}

void BaseAlertView::place()
{
	using namespace Gfx;
	int xSize = rect.xSize() * .8;
	text.maxLineSize = projP.unprojectXSize(xSize) * 0.95_gc;
	text.compile(renderer(), projP);

	int menuYSize = menu.cells() * text.face->nominalHeight()*2;
	int labelYSize = IG::makeEvenRoundedUp(projP.projectYSize(text.ySize + (text.nominalHeight * .5_gc)));
	IG::WindowRect viewFrame;
	viewFrame.setPosRel({rect.xSize()/2, rect.ySize()/2},
			{xSize, labelYSize + menuYSize}, C2DO);

	labelFrame = projP.unProjectRect(viewFrame.x, viewFrame.y, viewFrame.x2, viewFrame.y + labelYSize);

	IG::WindowRect menuViewFrame;
	menuViewFrame.setPosRel({viewFrame.x, viewFrame.y + (int)labelYSize},
			{viewFrame.xSize(), menuYSize}, LT2DO);
	menu.setViewRect(menuViewFrame, projP);
	menu.place();
}

bool BaseAlertView::inputEvent(Input::Event e)
{
	if(e.state() == Input::PUSHED)
	{
		if(e.isDefaultCancelButton())
		{
			dismiss();
			return true;
		}
	}
	return menu.inputEvent(e);
}

void BaseAlertView::draw()
{
	using namespace Gfx;
	auto &r = renderer();
	r.setBlendMode(BLEND_MODE_ALPHA);
	r.noTexProgram.use(r, projP.makeTranslate());
	r.setColor(.4, .4, .4, .8);
	GeomRect::draw(r, labelFrame);
	r.setColor(.1, .1, .1, .6);
	GeomRect::draw(r, menu.viewRect(), projP);
	r.setColor(COLOR_WHITE);
	r.texAlphaReplaceProgram.use(r);
	text.draw(r, labelFrame.xPos(C2DO), projP.alignYToPixel(labelFrame.yPos(C2DO)), C2DO, projP);
	//setClipRect(1);
	//setClipRectBounds(menu.viewRect());
	menu.draw();
	//setClipRect(0);
}

void BaseAlertView::onAddedToController(Input::Event e)
{
	menu.setController(controller, e);
}

void BaseAlertView::setLabel(const char *label)
{
	text.setString(label);
}

AlertView::AlertView(ViewAttachParams attach, const char *label, uint menuItems):
	BaseAlertView{attach, label, item},
	item{menuItems}
{}

void AlertView::setItem(uint idx, const char *name, TextMenuItem::SelectDelegate del)
{
	assert(idx < item.size());
	item[idx].t.setString(name);
	item[idx].setOnSelect(del);
}

YesNoAlertView::YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr, const char *noStr,
	TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo):
	BaseAlertView(attach, label,
		[](const TableView &)
		{
			return 2;
		},
		[this](const TableView &, int idx) -> MenuItem&
		{
			return idx == 0 ? yes : no;
		}),
	yes{yesStr ? yesStr : "Yes", onYes ? onYes : makeDefaultSelectDelegate()},
	no{noStr ? noStr : "No", onNo ? onNo : makeDefaultSelectDelegate()}
{}

void YesNoAlertView::setOnYes(TextMenuItem::SelectDelegate del)
{
	yes.setOnSelect(del);
}

void YesNoAlertView::setOnNo(TextMenuItem::SelectDelegate del)
{
	no.setOnSelect(del);
}

TextMenuItem::SelectDelegate YesNoAlertView::makeDefaultSelectDelegate()
{
	return TextMenuItem::wrapSelectDelegate([this]() { dismiss(); });
}
