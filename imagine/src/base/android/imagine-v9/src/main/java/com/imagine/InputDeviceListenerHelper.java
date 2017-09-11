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

package com.imagine;

import android.app.*;
import android.content.*;
import android.os.*;
import android.util.*;
import android.hardware.input.*;
import android.view.InputDevice;

final class InputDeviceListenerHelper
{
	private static final String logTag = "InputDeviceListenerHelper";
	private static native void deviceChanged(int change, int devID,
		InputDevice dev, String name, int src, int kbType);
	
	private final class Listener implements InputManager.InputDeviceListener
	{
		@Override public void onInputDeviceAdded(int id)
		{
			//Log.i(logTag, "added id: " + id);
			InputDevice dev = InputDevice.getDevice(id);
			if(InputDeviceHelper.shouldHandleDevice(dev))
			{
				deviceChanged(DEVICE_ADDED, id, dev, dev.getName(), dev.getSources(), dev.getKeyboardType());
			}
		}

		@Override public void onInputDeviceChanged(int id)
		{
			//Log.i(logTag, "changed id: " + id);
			InputDevice dev = InputDevice.getDevice(id);
			if(InputDeviceHelper.shouldHandleDevice(dev))
			{
				deviceChanged(DEVICE_CHANGED, id, dev, dev.getName(), dev.getSources(), dev.getKeyboardType());
			}
		}
		
		@Override public void onInputDeviceRemoved(int id)
		{
			//Log.i(logTag, "removed id: " + id);
			deviceChanged(DEVICE_REMOVED, id, null, null, 0, 0);
		}
	}

	private static final int DEVICE_ADDED = 0;
	private static final int DEVICE_CHANGED = 1;
	private static final int DEVICE_REMOVED = 2;
	private final Listener listener = new Listener();
	private InputManager inputManager = null;
	
	InputDeviceListenerHelper(Activity act)
	{
		//Log.i(logTag, "registering input device listener");
		inputManager = (InputManager)act.getSystemService(Context.INPUT_SERVICE);
	}
	
	void register()
	{
		inputManager.registerInputDeviceListener(listener, null);
	}
	
	void unregister()
	{
		inputManager.unregisterInputDeviceListener(listener);
	}
}
