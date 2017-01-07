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

#include <array>
#include <cstddef>
#include <imagine/config/defs.hh>
#include <imagine/bluetooth/config.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/operators.hh>

class BluetoothPendingSocket;

struct BluetoothAddr : public NotEquals<BluetoothAddr>
{
	constexpr BluetoothAddr() {}
	constexpr BluetoothAddr(const uint8 b[6]): b{b[0], b[1], b[2], b[3], b[4], b[5]} {}

	const uint8 *data() const
	{
		return b.data();
	}

	uint8 *data()
	{
		return b.data();
	}

	bool operator ==(BluetoothAddr const& rhs) const
	{
		return b == rhs.b;
	}

private:
	std::array<uint8, 6>b{};
} __attribute__((packed));

class BluetoothAdapter
{
public:
	enum { INIT_FAILED, SCAN_FAILED, SCAN_PROCESSING, SCAN_NO_DEVS, SCAN_NAME_FAILED,
		SCAN_COMPLETE, SCAN_CANCELLED/*, SOCKET_OPEN_FAILED*/ };
	enum State { STATE_OFF, STATE_ON, STATE_TURNING_OFF, STATE_TURNING_ON, STATE_ERROR };
	bool inDetect = false;
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	static bool useScanCache;
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	static uint scanSecs;
	#endif
	using OnStateChangeDelegate = DelegateFunc<void (BluetoothAdapter &bta, State newState)>;
	using OnStatusDelegate = DelegateFunc<void (BluetoothAdapter &bta, uint statusCode, int arg)>;
	using OnScanDeviceClassDelegate = DelegateFunc<bool (BluetoothAdapter &bta, const uchar devClass[3])>;
	using OnScanDeviceNameDelegate = DelegateFunc<void (BluetoothAdapter &bta, const char *name, BluetoothAddr addr)>;
	using OnIncomingL2capConnectionDelegate = DelegateFunc<void (BluetoothAdapter &bta, BluetoothPendingSocket &pending)>;

	BluetoothAdapter();
	static BluetoothAdapter *defaultAdapter();
	virtual bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) = 0;
	virtual void cancelScan() = 0;
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	static void setScanCacheUsage(bool on) { useScanCache = on; }
	static bool scanCacheUsage() { return useScanCache; }
	#endif
	virtual void close() = 0;
	virtual State state() = 0;
	virtual void setActiveState(bool on, OnStateChangeDelegate onStateChange) = 0;
	const OnStatusDelegate &onScanStatus() { return onScanStatusD; }

	#ifdef CONFIG_BLUETOOTH_SERVER
	OnIncomingL2capConnectionDelegate &onIncomingL2capConnection() { return onIncomingL2capConnectionD; }
	virtual void setL2capService(uint psm, bool active, OnStatusDelegate onResult) = 0;
	//virtual bool l2capServiceRegistered(uint psm);
	#endif

protected:
	OnStatusDelegate onScanStatusD;
	OnScanDeviceClassDelegate onScanDeviceClassD;
	OnScanDeviceNameDelegate onScanDeviceNameD;
	#ifdef CONFIG_BLUETOOTH_SERVER
	OnIncomingL2capConnectionDelegate onIncomingL2capConnectionD;
	#endif
};

class BluetoothSocket
{
public:
	constexpr BluetoothSocket() {}
	virtual CallResult openL2cap(BluetoothAddr addr, uint psm) = 0;
	virtual CallResult openRfcomm(BluetoothAddr addr, uint channel) = 0;
	#ifdef CONFIG_BLUETOOTH_SERVER
	virtual CallResult open(BluetoothPendingSocket &socket) = 0;
	#endif
	virtual void close() = 0;
	virtual CallResult write(const void *data, size_t size) = 0;
	typedef DelegateFunc<bool (const char *data, size_t size)> OnDataDelegate;
	OnDataDelegate &onData() { return onDataD; }
	typedef DelegateFunc<uint (BluetoothSocket &sock, uint status)> OnStatusDelegate;
	OnStatusDelegate &onStatus() { return onStatusD; }
	enum { STATUS_CONNECTING, STATUS_CONNECT_ERROR, STATUS_OPENED, STATUS_READ_ERROR, STATUS_CLOSED };
	enum { OPEN_USAGE_NONE = 0, OPEN_USAGE_READ_EVENTS };

protected:
	OnDataDelegate onDataD;
	OnStatusDelegate onStatusD;
};

class BluetoothInputDevice
{
public:
	virtual ~BluetoothInputDevice() {}
	virtual CallResult open(BluetoothAdapter &adapter) = 0;
	virtual void removeFromSystem() = 0;
};
