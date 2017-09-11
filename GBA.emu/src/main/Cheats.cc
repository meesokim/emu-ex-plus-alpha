#include <imagine/gui/TextEntry.hh>
#include <emuframework/Cheats.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <gba/Cheats.h>
static bool cheatsModified = false;

void EmuEditCheatView::renamed(const char *str)
{
	cheatsModified = true;
	auto &cheat = cheatsList[idx];
	string_copy(cheat.desc, str);
	name.t.setString(cheat.desc);
	name.compile(renderer(), projP);
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, uint cheatIdx):
	BaseEditCheatView
	{
		"Edit Code",
		attach,
		cheatsList[cheatIdx].desc,
		[this](const TableView &)
		{
			return 3;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return name;
				case 1: return code;
				default: return remove;
			}
		},
		[this](TextMenuItem &item, View &parent, Input::Event e)
		{
			cheatsModified = true;
			cheatsDelete(gGba.cpu, idx, true);
			EmuApp::refreshCheatViews();
			dismiss();
			return true;
		}
	},
	code
	{
		"Code",
		cheatsList[cheatIdx].codestring,
		[this](DualTextMenuItem &, View &, Input::Event)
		{
			EmuApp::postMessage("To change this cheat, please delete and re-add it");
		}
	},
	idx{cheatIdx}
{}

void EmuEditCheatListView::loadCheatItems()
{
	uint cheats = cheatsNumber;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back(cheatsList[c].desc,
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				auto &editCheatView = *new EmuEditCheatView{attachParams(), c};
				pushAndShow(editCheatView, e);
			});
	}
}

void EmuEditCheatListView::addNewCheat(int isGSv3)
{
	if(cheatsNumber == EmuCheats::MAX)
	{
		EmuApp::postMessage(true, "Too many cheats, delete some first");
		window().postDraw();
		return;
	}
	EmuApp::pushAndShowNewCollectTextInputView(attachParams(), {},
		isGSv3 ? "Input xxxxxxxx yyyyyyyy" : "Input xxxxxxxx yyyyyyyy (GS) or xxxxxxxx yyyy (AR)", "",
		[this, isGSv3](CollectTextInputView &view, const char *str)
		{
			if(str)
			{
				char tempStr[20];
				string_copy(tempStr, str);
				string_toUpper(tempStr);
				if(strlen(tempStr) == 17 && tempStr[8] == ' ')
				{
					logMsg("removing middle space in text");
					memmove(&tempStr[8], &tempStr[9], 9); // 8 chars + null byte
				}
				if(isGSv3 ?
					cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", true) :
					((strlen(tempStr) == 16 && cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", false))
					|| cheatsAddCBACode(gGba.cpu, tempStr, "Unnamed Cheat")))
				{
					logMsg("added new cheat, %d total", cheatsNumber);
				}
				else
				{
					EmuApp::postMessage(true, "Invalid format");
					return 1;
				}
				cheatsModified = true;
				cheatsDisable(gGba.cpu, cheatsNumber-1);
				view.dismiss();
				EmuApp::refreshCheatViews();
				EmuApp::pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
					[](CollectTextInputView &view, const char *str)
					{
						if(str)
						{
							string_copy(cheatsList[cheatsNumber-1].desc, str);
							view.dismiss();
							EmuApp::refreshCheatViews();
						}
						else
						{
							view.dismiss();
						}
						return 0;
					});
			}
			else
			{
				view.dismiss();
			}
			return 0;
		});
}

EmuEditCheatListView::EmuEditCheatListView(ViewAttachParams attach):
	BaseEditCheatListView
	{
		attach,
		[this](const TableView &)
		{
			return 2 + cheat.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return addGS12CBCode;
				case 1: return addGS3Code;
				default: return cheat[idx - 2];
			}
		}
	},
	addGS12CBCode
	{
		"Add Game Shark v1-2/Code Breaker Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			addNewCheat(false);
		}
	},
	addGS3Code
	{
		"Add Game Shark v3 Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			addNewCheat(true);
		}
	}
{
	loadCheatItems();
}

void EmuCheatsView::loadCheatItems()
{
	uint cheats = cheatsNumber;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		auto &cheatEntry = cheatsList[c];
		cheat.emplace_back(cheatEntry.desc, cheatEntry.enabled,
			[this, c](BoolMenuItem &item, View &, Input::Event e)
			{
				cheatsModified = true;
				bool on = item.flipBoolValue(*this);
				if(on)
					cheatsEnable(c);
				else
					cheatsDisable(gGba.cpu, c);
			});
	}
}

EmuCheatsView::EmuCheatsView(ViewAttachParams attach): BaseCheatsView{attach}
{
	loadCheatItems();
}

void writeCheatFile()
{
	if(!cheatsModified)
		return;

	auto filename = FS::makePathStringPrintf("%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName().data());

	if(!cheatsNumber)
	{
		logMsg("deleting cheats file %s", filename.data());
		FS::remove(filename.data());
		cheatsModified = false;
		return;
	}
	cheatsSaveCheatList(filename.data());
	cheatsModified = false;
}

void readCheatFile()
{
	auto filename = FS::makePathStringPrintf("%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName().data());
	if(cheatsLoadCheatList(filename.data()))
	{
		logMsg("loaded cheat file: %s", filename.data());
	}
}
