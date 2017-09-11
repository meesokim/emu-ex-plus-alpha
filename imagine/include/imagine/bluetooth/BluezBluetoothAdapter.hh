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
#include "BluetoothAdapter.hh"
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <imagine/base/Base.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/base/Pipe.hh>
#ifdef CONFIG_BLUETOOTH_SERVER
#include <imagine/util/container/DLList.hh>
#endif

class BluetoothPendingSocket
{
public:
	int fd = -1;
	sockaddr_l2 addr {};

	constexpr BluetoothPendingSocket() {}
	void close();
	uint channel() { return addr.l2_psm; }
	void requestName(BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName);

	explicit operator bool() const
	{
		return fd != -1;
	}
};

class BluezBluetoothAdapter : public BluetoothAdapter
{
public:
	static BluezBluetoothAdapter *defaultAdapter();
	bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) final;
	void cancelScan() final;
	void close() final;
	#ifdef CONFIG_BLUETOOTH_SERVER
	void setL2capService(uint psm, bool active, OnStatusDelegate onResult) final;
	//bool l2capServiceRegistered(uint psm) final;
	#endif
	void requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName);
	State state() final;
	void setActiveState(bool on, OnStateChangeDelegate onStateChange) final;

private:
	int devId = -1, socket = -1;
	Base::Pipe statusPipe;
	bool scanCancelled = false;
	#ifdef CONFIG_BLUETOOTH_SERVER
	struct L2CapServer
	{
		constexpr L2CapServer() {}
		constexpr L2CapServer(uint psm, int fd): psm(psm), fd(fd) {}
		uint psm = 0;
		int fd = -1;
		Base::FDEventSource connectSrc;
	};
	StaticDLList<L2CapServer, 2> serverList;
	#endif

	bool openDefault();
	CallResult doScan(const OnScanDeviceClassDelegate &onDeviceClass, const OnScanDeviceNameDelegate &onDeviceName);
	void sendBTScanStatusDelegate(uint8 type, uint8 arg);
};

class BluezBluetoothSocket : public BluetoothSocket
{
public:
	BluezBluetoothSocket() {}
	CallResult openL2cap(BluetoothAddr addr, uint psm) final;
	CallResult openRfcomm(BluetoothAddr addr, uint channel) final;
	#ifdef CONFIG_BLUETOOTH_SERVER
	CallResult open(BluetoothPendingSocket &socket) final;
	#endif
	void close() final;
	CallResult write(const void *data, size_t size) final;
	int readPendingData(int events);

private:
	Base::FDEventSource fdSrc;
	int fd = -1;
	void setupFDEvents(int events);
};
