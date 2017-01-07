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
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/TextEntry.hh>
#include <imagine/logger/logger.h>
#include <imagine/gui/TableView.hh>

void TextEntry::setAcceptingInput(bool on)
{
	if(on)
	{
		Input::showSoftInput();
		logMsg("accepting input");
	}
	else
	{
		Input::hideSoftInput();
		logMsg("stopped accepting input");
	}
	acceptingInput = on;
}

void TextEntry::inputEvent(Input::Event e)
{
	if(e.isPointer() && e.pushed() && b.overlaps({e.x, e.y}))
	{
		{
			setAcceptingInput(1);
		}
		return;
	}

	if(acceptingInput && e.pushed() && e.map == e.MAP_SYSTEM)
	{
		bool updateText = false;

		if(e.button == Input::Keycode::BACK_SPACE)
		{
			int len = strlen(str);
			if(len > 0)
			{
				str[len-1] = '\0';
				updateText = true;
			}
		}
		else
		{
			auto keyStr = e.keyString();
			if(strlen(keyStr.data()))
			{
				if(!multiLine)
				{
					if(keyStr[0] == '\r' || keyStr[0] == '\n')
					{
						setAcceptingInput(0);
						return;
					}
				}
				string_cat(str, keyStr.data());
				updateText = true;
			}
		}

		if(updateText)
		{
			t.setString(str);
			t.compile(projP);
			Base::mainWindow().postDraw();
		}
	}
}

void TextEntry::draw()
{
	using namespace Gfx;
	texAlphaProgram.use();
	t.draw(projP.unProjectRect(b).pos(LC2DO) + GP{TableView::globalXIndent, 0}, LC2DO, projP);
}

void TextEntry::place()
{
	t.compile(projP);
}

void TextEntry::place(IG::WindowRect rect, const Gfx::ProjectionPlane &projP)
{
	this->projP = projP;
	b = rect;
	place();
}

CallResult TextEntry::init(const char *initText, Gfx::GlyphTextureSet *face, const Gfx::ProjectionPlane &projP)
{
	string_copy(str, initText);
	t = {str, face};
	t.compile(projP);
	acceptingInput = 0;
	return OK;
}

void CollectTextInputView::init(const char *msgText, const char *initialContent, Gfx::PixmapTexture *closeRes, Gfx::GlyphTextureSet *face)
{
	#ifndef CONFIG_BASE_ANDROID
	if(View::needsBackControl && closeRes)
	{
		cancelSpr.init({-.5, -.5, .5, .5}, *closeRes);
		if(cancelSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
			Gfx::autoReleaseShaderCompiler();
	}
	#endif
	message = {msgText, face};
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	textEntry.init(initialContent, face, projP);
	textEntry.setAcceptingInput(1);
	#else
	Input::startSysTextInput(
		[this](const char *str)
		{
			Gfx::bind();
			if(!str)
			{
				logMsg("text collection canceled by external source");
				dismiss();
				return;
			}
			if(onTextD(*this, str))
			{
				logMsg("text collection canceled by text delegate");
				dismiss();
			}
		},
		initialContent, msgText, face->settings.pixelHeight());
	#endif
}

CollectTextInputView::~CollectTextInputView()
{
	#ifdef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	Input::cancelSysTextInput();
	#endif
}

void CollectTextInputView::place()
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		cancelBtn.setPosRel(rect.pos(RT2DO), View::defaultFace.nominalHeight() * 1.75, RT2DO);
		cancelSpr.setPos(-projP.unprojectXSize(cancelBtn)/3., -projP.unprojectYSize(cancelBtn)/3., projP.unprojectXSize(cancelBtn)/3., projP.unprojectYSize(cancelBtn)/3.);
	}
	#endif
	message.maxLineSize = projP.w * 0.95;
	message.compile(projP);
	IG::WindowRect textRect;
	int xSize = rect.xSize() * 0.95;
	int ySize = View::defaultFace.nominalHeight()* (Config::envIsAndroid ? 2. : 1.5);
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	textRect.setPosRel({rect.xPos(C2DO), rect.yPos(C2DO)}, {xSize, ySize}, C2DO);
	textEntry.place(textRect, projP);
	#else
	textRect.setPosRel(rect.pos(C2DO) - IG::WP{0, (int)rect.ySize()/4}, {xSize, ySize}, C2DO);
	Input::placeSysTextInput(textRect);
	#endif
}

void CollectTextInputView::inputEvent(Input::Event e)
{
	if(e.state == Input::PUSHED)
	{
		if(e.isDefaultCancelButton() || (e.isPointer() && cancelBtn.overlaps({e.x, e.y})))
		{
			dismiss();
			return;
		}
	}
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	bool acceptingInput = textEntry.acceptingInput;
	textEntry.inputEvent(e);
	if(!textEntry.acceptingInput && acceptingInput)
	{
		logMsg("calling on-text delegate");
		if(onTextD.callCopy(*this, textEntry.str))
		{
			textEntry.setAcceptingInput(1);
		}
	}
	#endif
}

void CollectTextInputView::draw()
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		setColor(COLOR_WHITE);
		setBlendMode(BLEND_MODE_ALPHA);
		TextureSampler::bindDefaultNearestMipClampSampler();
		cancelSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(cancelBtn).pos(C2DO)));
		cancelSpr.draw();
	}
	#endif
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	setColor(0.25);
	noTexProgram.use(projP.makeTranslate());
	GeomRect::draw(textEntry.b, projP);
	setColor(COLOR_WHITE);
	textEntry.draw();
	texAlphaProgram.use();
	message.draw(0, projP.unprojectY(textEntry.b.pos(C2DO).y) + message.nominalHeight, CB2DO, projP);
	#else
	setColor(COLOR_WHITE);
	texAlphaProgram.use(projP.makeTranslate());
	message.draw(0, projP.unprojectY(Input::sysTextInputRect().pos(C2DO).y) + message.nominalHeight, CB2DO, projP);
	#endif
}
