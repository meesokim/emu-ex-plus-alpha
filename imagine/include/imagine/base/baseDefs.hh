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
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/bits.h>

namespace Base
{
using namespace IG;

#if defined __APPLE__ && TARGET_OS_IPHONE
using FrameTimeBase = double;

template<class T>
constexpr static FrameTimeBase frameTimeBaseFromSecs(T s)
{
	return s;
}

constexpr static double frameTimeBaseToSecsDec(FrameTimeBase time)
{
	return time;
}

template<class T>
constexpr static FrameTimeBase frameTimeBaseFromNSecs(T ns)
{
	return (double)ns / 1000000000.;
}

constexpr static int64_t frameTimeBaseToNSecs(FrameTimeBase time)
{
	return time * 1000000000.;
}
#else
using FrameTimeBase = uint64_t;

template<class T>
constexpr static FrameTimeBase frameTimeBaseFromSecs(T s)
{
	if constexpr(std::is_floating_point<T>::value)
			return (double)s * (double)1000000000.;
	else
		return (FrameTimeBase)s * (FrameTimeBase)1000000000;
}

constexpr static double frameTimeBaseToSecsDec(FrameTimeBase time)
{
	return (double)time / (double)1000000000.;
}

template<class T>
constexpr static FrameTimeBase frameTimeBaseFromNSecs(T ns)
{
	return ns;
}

constexpr static int64_t frameTimeBaseToNSecs(FrameTimeBase time)
{
	return time;
}
#endif

FrameTimeBase timeSinceFrameTime(FrameTimeBase time);

// orientation
static constexpr uint VIEW_ROTATE_0 = bit(0), VIEW_ROTATE_90 = bit(1), VIEW_ROTATE_180 = bit(2), VIEW_ROTATE_270 = bit(3);
static constexpr uint VIEW_ROTATE_AUTO = bit(5);
static constexpr uint VIEW_ROTATE_ALL = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270;
static constexpr uint VIEW_ROTATE_ALL_BUT_UPSIDE_DOWN = VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_270;

const char *orientationToStr(uint o);
bool orientationIsSideways(uint o);
uint validateOrientationMask(uint oMask);
}
