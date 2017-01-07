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

#define LOGTAG "X11"
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/util/string.h>
#include "../common/windowPrivate.hh"
#include "../common/screenPrivate.hh"
#include "../common/basePrivate.hh"
#include "x11.hh"
#include "internal.hh"
#include "xdnd.hh"
#include "xlibutils.h"

#define ASCII_LF 0xA
#define ASCII_CR 0xD

namespace Base
{

Display *dpy{};

// TODO: move into generic header after testing
static void fileURLToPath(char *url)
{
	char *pathStart = url;
	//lookup the 3rd slash which will signify the root directory of the file system
	for(uint i = 0; i < 3; i++)
	{
		pathStart = strchr(pathStart + 1, '/');
	}
	assert(pathStart != NULL);

	// strip trailing new line junk at the end, needed for Nautilus
	char *pathEnd = &pathStart[strlen(pathStart)-1];
	assert(pathEnd >= pathStart);
	for(; *pathEnd == ASCII_LF || *pathEnd == ASCII_CR || *pathEnd == ' '; pathEnd--)
	{
		*pathEnd = '\0';
	}

	// copy the path over the old string
	size_t destPos = 0;
	for(size_t srcPos = 0; pathStart[srcPos] != '\0'; ({srcPos++; destPos++;}))
	{
		if(pathStart[srcPos] != '%') // plain copy case
		{
			url[destPos] = pathStart[srcPos];
		}
		else // decode the next 2 chars as hex digits
		{
			srcPos++;
			int msd = char_hexToInt(pathStart[srcPos]) << 4;
			srcPos++;
			int lsd = char_hexToInt(pathStart[srcPos]);
			url[destPos] = msd + lsd;
		}
	}
	url[destPos] = '\0';
}

static void ewmhFullscreen(Display *dpy, ::Window win, int action)
{
	assert(action == _NET_WM_STATE_REMOVE || action == _NET_WM_STATE_ADD || action == _NET_WM_STATE_TOGGLE);

	XEvent xev{};
	xev.xclient.type = ClientMessage;
	xev.xclient.send_event = True;
	xev.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xev.xclient.window = win;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = action;
	xev.xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

	// TODO: test if DefaultRootWindow(dpy) works on other screens
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, win, &attr);
	if(!XSendEvent(dpy, attr.root, False,
		SubstructureRedirectMask | SubstructureNotifyMask, &xev))
	{
		logWarn("couldn't send root window NET_WM_STATE message");
	}
}

void toggleFullScreen(::Window xWin)
{
	logMsg("toggle fullscreen");
	ewmhFullscreen(dpy, xWin, _NET_WM_STATE_TOGGLE);
}

static int eventHandler(XEvent &event)
{
	//logMsg("got event type %s (%d)", xEventTypeToString(event.type), event.type);
	
	switch(event.type)
	{
		bcase Expose:
		{
			auto &win = *windowForXWindow(event.xexpose.window);
			if (event.xexpose.count == 0)
				win.postDraw();
		}
		bcase ConfigureNotify:
		{
			//logMsg("ConfigureNotify");
			auto &win = *windowForXWindow(event.xconfigure.window);
			if(event.xconfigure.width == win.width() && event.xconfigure.height == win.height())
				break;
			win.updateSize({event.xconfigure.width, event.xconfigure.height});
			win.postDraw();
		}
		bcase ClientMessage:
		{
			auto &win = *windowForXWindow(event.xclient.window);
			auto type = event.xclient.message_type;
			char *clientMsgName = XGetAtomName(dpy, type);
			//logDMsg("got client msg %s", clientMsgName);
			if(string_equal(clientMsgName, "WM_PROTOCOLS"))
			{
				if((Atom)event.xclient.data.l[0] == XInternAtom(dpy, "WM_DELETE_WINDOW", True))
				{
					//logMsg("got window manager delete window message");
					win.dispatchDismissRequest();
				}
				else
				{
					logMsg("unknown WM_PROTOCOLS message");
				}
			}
			else if(Config::Base::XDND && dndInit)
			{
				handleXDNDEvent(dpy, event.xclient, win.xWin, win.draggerXWin, win.dragAction);
			}
			XFree(clientMsgName);
		}
		bcase PropertyNotify:
		{
			//logMsg("PropertyNotify");
		}
		bcase SelectionNotify:
		{
			logMsg("SelectionNotify");
			if(Config::Base::XDND && event.xselection.property != None)
			{
				auto &win = *windowForXWindow(event.xselection.requestor);
				int format;
				unsigned long numItems;
				unsigned long bytesAfter;
				unsigned char *prop;
				Atom type;
				XGetWindowProperty(dpy, win.xWin, event.xselection.property, 0, 256, False, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &prop);
				logMsg("property read %lu items, in %d format, %lu bytes left", numItems, format, bytesAfter);
				logMsg("property is %s", prop);
				sendDNDFinished(dpy, win.xWin, win.draggerXWin, win.dragAction);
				auto filename = (char*)prop;
				fileURLToPath(filename);
				win.dispatchDragDrop(filename);
				XFree(prop);
			}
		}
		bcase MapNotify:
		{
			//logDMsg("MapNotfiy");
		}
		bcase ReparentNotify:
		{
			//logDMsg("ReparentNotify");
		}
		bcase GenericEvent:
		{
			Input::handleXI2GenericEvent(event);
		}
		bdefault:
		{
			logDMsg("got unhandled message type %d", event.type);
		}
		break;
	}

	return 1;
}

bool x11FDPending()
{
  return XPending(dpy) > 0;
}

void x11FDHandler()
{
	while(x11FDPending())
	{
		XEvent event;
		XNextEvent(dpy, &event);
		eventHandler(event);
	}
}

void initXScreens()
{
	auto defaultScreenIdx = DefaultScreen(dpy);
	static Screen main;
	main.init(ScreenOfDisplay(dpy, defaultScreenIdx));
	Screen::addScreen(&main);
	#ifdef CONFIG_BASE_MULTI_SCREEN
	iterateTimes(ScreenCount(dpy), i)
	{
		if((int)i == defaultScreenIdx)
			continue;
		Screen s = new Screen();
		screen->init(ScreenOfDisplay(dpy, i));
		Screen::addScreen(s);
		if(!extraScreen.freeSpace())
			break;
	}
	#endif
}

CallResult initWindowSystem(EventLoop loop, FDEventSource &eventSrc)
{
	#ifndef CONFIG_BASE_X11_EGL
	// needed to call glXWaitVideoSyncSGI in separate thread
	XInitThreads();
	#endif
	dpy = XOpenDisplay(0);
	if(!dpy)
	{
		logErr("couldn't open display");
		return IO_ERROR;
	}
	initXScreens();
	initFrameTimer(loop);
	Input::init();
	eventSrc = FDEventSource::makeXServerAddedToEventLoop(ConnectionNumber(dpy), loop);
	return OK;
}

void setSysUIStyle(uint flags) {}

bool hasTranslucentSysUI() { return false; }

bool hasHardwareNavButtons() { return false; }

}
