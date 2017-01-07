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

#include <imagine/config/defs.hh>
#include <imagine/base/eventLoopDefs.hh>

#if defined CONFIG_BASE_GLIB
#include <imagine/base/eventloop/GlibEventLoop.hh>
#elif defined __ANDROID__
#include <imagine/base/eventloop/ALooperEventLoop.hh>
#elif defined __APPLE__
#include <imagine/base/eventloop/CFEventLoop.hh>
#endif

namespace Base
{

class EventLoop : public EventLoopImpl
{
public:
	using EventLoopImpl::EventLoopImpl;

	constexpr EventLoop() {}
	static EventLoop forThread();
	static EventLoop makeForThread();
	void run();
	explicit operator bool() const;
};

struct FDEventSource : public FDEventSourceImpl
{
public:
	using FDEventSourceImpl::FDEventSourceImpl;

	constexpr FDEventSource() {}
	FDEventSource(int fd);
	FDEventSource(int fd, EventLoop loop, PollEventDelegate callback, uint events);
	FDEventSource(int fd, EventLoop loop, PollEventDelegate callback):
		FDEventSource{fd, loop, callback, POLLEV_IN}
	{}
	FDEventSource(FDEventSource &&o);
	FDEventSource &operator=(FDEventSource o);
	~FDEventSource();
	static void swap(FDEventSource &a, FDEventSource &b);
	static FDEventSource makeXServerAddedToEventLoop(int fd, EventLoop loop);
	bool addToEventLoop(EventLoop loop, PollEventDelegate callback, uint events);
	void addXServerToEventLoop(EventLoop loop);
	void modifyEvents(uint events);
	void removeFromEventLoop();
	bool hasEventLoop();
	int fd() const;
};

}
