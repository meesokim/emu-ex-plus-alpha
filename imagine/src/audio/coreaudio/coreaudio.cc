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

#define LOGTAG "CoreAudio"
#include <imagine/audio/Audio.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ringbuffer/MachRingBuffer.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>
#include <AudioUnit/AudioUnit.h>
#include <TargetConditionals.h>

namespace Audio
{

PcmFormat pcmFormat;
static uint wantedLatency = 100000;
static AudioComponentInstance outputUnit{};
static AudioStreamBasicDescription streamFormat;
static bool isPlaying_ = false, isOpen_ = false, hadUnderrun = false;
static StaticMachRingBuffer<> rBuff;

int maxRate()
{
	return 48000;
}

static bool isInit()
{
	return outputUnit;
}

void setHintOutputLatency(uint us)
{
	wantedLatency = us;
}

uint hintOutputLatency()
{
	return wantedLatency;
}

static OSStatus outputCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
		const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
	auto *buf = (char*)ioData->mBuffers[0].mData;
	uint bytes = inNumberFrames * streamFormat.mBytesPerFrame;

	if(unlikely(hadUnderrun))
	{
		*ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
		std::fill_n(buf, bytes, 0);
		return 0;
	}

	uint read = rBuff.read(buf, bytes);
	if(unlikely(read != bytes))
	{
		//logMsg("underrun, read %d out of %d bytes", read, bytes);
		hadUnderrun = true;
		uint padBytes = bytes - read;
		//logMsg("padding %d bytes", padBytes);
		std::fill_n(&buf[read], padBytes, 0);
	}
	return 0;
}

static std::error_code openUnit(AudioStreamBasicDescription &fmt, uint bufferSize)
{
	logMsg("creating unit %dHz %d channels", (int)fmt.mSampleRate, (int)fmt.mChannelsPerFrame);
	if(!rBuff.init(bufferSize))
	{
		return {ENOMEM, std::system_category()};
	}
	auto err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
			0, &fmt, sizeof(AudioStreamBasicDescription));
	if(err)
	{
		logErr("error %d setting stream format", (int)err);
		rBuff.deinit();
		return {EINVAL, std::system_category()};
	}
	AudioUnitInitialize(outputUnit);
	isOpen_ = true;
	return {};
}

static void init()
{
	logMsg("setting up playback audio unit");
	AudioComponentDescription defaultOutputDescription =
	{
		kAudioUnitType_Output,
		#if TARGET_OS_IPHONE
		kAudioUnitSubType_RemoteIO,
		#else
		kAudioUnitSubType_DefaultOutput,
		#endif
		kAudioUnitManufacturer_Apple
	};
	AudioComponent defaultOutput = AudioComponentFindNext(nullptr, &defaultOutputDescription);
	assert(defaultOutput);
	auto err = AudioComponentInstanceNew(defaultOutput, &outputUnit);
	if(!outputUnit)
	{
		bug_exit("error creating output unit: %d", (int)err);
	}
	AURenderCallbackStruct renderCallbackProp =
	{
		outputCallback,
		//nullptr
	};
	err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
	    0, &renderCallbackProp, sizeof(renderCallbackProp));
	if(err)
	{
		bug_exit("error setting callback: %d", (int)err);
	}
}

std::error_code openPcm(const PcmFormat &format)
{
	if(isOpen())
	{
		logWarn("audio unit already open");
		return {};
	}
	if(unlikely(!isInit()))
		init();
	streamFormat.mSampleRate = format.rate;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
	streamFormat.mBytesPerPacket = format.framesToBytes(1);
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mBytesPerFrame = format.framesToBytes(1);
	streamFormat.mChannelsPerFrame = format.channels;
	streamFormat.mBitsPerChannel = format.sample.bits == 16 ? 16 : 8;
	pcmFormat = format;
	uint bufferSize = format.uSecsToBytes(wantedLatency);
	return openUnit(streamFormat, bufferSize);
}

void closePcm()
{
	if(!isOpen())
	{
		logWarn("audio unit already closed");
		return;
	}
	AudioOutputUnitStop(outputUnit);
	AudioUnitUninitialize(outputUnit);
	rBuff.deinit();
	isPlaying_ = false;
	isOpen_ = false;
	hadUnderrun = false;
	logMsg("closed audio unit");
}

bool isOpen()
{
	return isOpen_;
}

bool isPlaying()
{
	return isPlaying_ && !hadUnderrun;
}

void pausePcm()
{
	if(unlikely(!isOpen()))
		return;
	AudioOutputUnitStop(outputUnit);
	isPlaying_ = false;
	hadUnderrun = false;
}

void resumePcm()
{
	if(unlikely(!isOpen()))
		return;
	if(!isPlaying_ || hadUnderrun)
	{
		logMsg("playback starting with %u frames", (uint)(rBuff.writtenSize() / streamFormat.mBytesPerFrame));
		hadUnderrun = false;
		if(!isPlaying_)
		{
			auto err = AudioOutputUnitStart(outputUnit);
			if(err)
			{
				logErr("error %d in AudioOutputUnitStart", (int)err);
			}
			else
				isPlaying_ = true;
		}
	}
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	pausePcm();
	rBuff.reset();
}

void writePcm(const void *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	uint bytes = framesToWrite * streamFormat.mBytesPerFrame;
	auto written = rBuff.write(samples, bytes);
	if(written != bytes)
	{
		//logMsg("overrun, wrote %d out of %d bytes", written, bytes);
	}
}

BufferContext getPlayBuffer(uint wantedFrames)
{
	if(unlikely(!isOpen()) || !framesFree())
		return {};
	if((uint)framesFree() < wantedFrames)
	{
		logDMsg("buffer has only %d/%d frames free", framesFree(), wantedFrames);
	}
	// will always have a contiguous block from mirrored pages
	return {rBuff.writeAddr(), std::min(wantedFrames, (uint)framesFree())};
}

void commitPlayBuffer(BufferContext buffer, uint frames)
{
	assert(frames <= buffer.frames);
	auto bytes = frames * streamFormat.mBytesPerFrame;
	rBuff.commitWrite(bytes);
}

// TODO
int frameDelay() { return 0; }

int framesFree()
{
	return rBuff.freeSpace() / streamFormat.mBytesPerFrame;
}

}
