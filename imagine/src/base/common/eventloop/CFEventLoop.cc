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

#include <imagine/base/EventLoop.hh>
#include <imagine/logger/logger.h>

namespace Base
{

CFFDEventSource::CFFDEventSource(int fd)
{
	info = std::make_unique<CFFDEventSourceInfo>();
	CFFileDescriptorContext ctx{0, info.get()};
	info->fdRef = CFFileDescriptorCreate(kCFAllocatorDefault, fd, false,
		[](CFFileDescriptorRef fdRef, CFOptionFlags callbackEventTypes, void *info_)
		{
			//logMsg("got fd events: 0x%X", (int)callbackEventTypes);
			auto &info = *((CFFDEventSourceInfo*)info_);
			auto fd = CFFileDescriptorGetNativeDescriptor(fdRef);
			info.callback(fd, callbackEventTypes);
			if(info.fdRef) // re-enable callbacks if fd is still open
				CFFileDescriptorEnableCallBacks(fdRef, callbackEventTypes);
		}, &ctx);
}

FDEventSource::FDEventSource(int fd):
	CFFDEventSource{fd}
{}

FDEventSource::FDEventSource(int fd, EventLoop loop, PollEventDelegate callback, uint events):
	FDEventSource{fd}
{
	addToEventLoop(loop, callback, events);
}

FDEventSource::FDEventSource(FDEventSource &&o)
{
	swap(*this, o);
}

FDEventSource &FDEventSource::operator=(FDEventSource o)
{
	swap(*this, o);
	return *this;
}

FDEventSource::~FDEventSource()
{
	removeFromEventLoop();
	if(info)
	{
		CFFileDescriptorInvalidate(info->fdRef);
		CFRelease(info->fdRef);
	}
}

void FDEventSource::swap(FDEventSource &a, FDEventSource &b)
{
	std::swap(a.info, b.info);
	std::swap(a.src, b.src);
	std::swap(a.loop, b.loop);
}

bool FDEventSource::addToEventLoop(EventLoop loop, PollEventDelegate callback, uint events)
{
	assert(info);
	if(Config::DEBUG_BUILD)
	{
		logMsg("adding fd %d to run loop", fd());
	}
	info->callback = callback;
	CFFileDescriptorEnableCallBacks(info->fdRef, events);
	src = CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, info->fdRef, 0);
	if(!loop)
		loop = EventLoop::forThread();
	CFRunLoopAddSource(loop.nativeObject(), src, kCFRunLoopDefaultMode);
	this->loop = loop.nativeObject();
	return true;
}

void FDEventSource::modifyEvents(uint events)
{
	assert(info);
	uint disableEvents = ~events & 0x3;
	if(disableEvents)
		CFFileDescriptorDisableCallBacks(info->fdRef, disableEvents);
	if(events)
		CFFileDescriptorEnableCallBacks(info->fdRef, events);
}

void FDEventSource::removeFromEventLoop()
{
	if(src)
	{
		if(Config::DEBUG_BUILD)
		{
			logMsg("removing fd %d from run loop", fd());
		}
		CFRunLoopRemoveSource(loop, src, kCFRunLoopDefaultMode);
		CFRelease(src);
		src = {};
		loop = {};
	}
}

bool FDEventSource::hasEventLoop()
{
	return loop;
}

int FDEventSource::fd() const
{
	return info ? CFFileDescriptorGetNativeDescriptor(info->fdRef) : -1;
}

EventLoop EventLoop::forThread()
{
	return {CFRunLoopGetCurrent()};
}

EventLoop EventLoop::makeForThread()
{
	return forThread();
}

void EventLoop::run()
{
	logMsg("running event loop:%p", loop);
	CFRunLoopRun();
	logMsg("event loop:%p finished", loop);
}

EventLoop::operator bool() const
{
	return loop;
}

}
