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

#define LOGTAG "Intent"
#include <android/native_activity.h>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"

namespace Base
{

static JavaInstMethod<void(jstring, jstring, jstring)> jAddNotification{};
static JavaInstMethod<void()> jRemoveNotification{};
static JavaInstMethod<void(jstring, jstring)> jAddViewShortcut{};
static JavaInstMethod<jobject()> jIntentDataPath{};

void addNotification(const char *onShow, const char *title, const char *message)
{
	logMsg("adding notificaion icon");
	auto env = jEnv();
	if(unlikely(!jAddNotification))
	{
		jAddNotification.setup(env, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	}
	jAddNotification(env, jBaseActivity, env->NewStringUTF(onShow), env->NewStringUTF(title), env->NewStringUTF(message));
}

void removePostedNotifications()
{
	// check if notification functions were used at some point
	// and remove the posted notification
	if(!jAddNotification)
		return;
	auto env = jEnv();
	if(unlikely(!jRemoveNotification))
	{
		jRemoveNotification.setup(env, jBaseActivityCls, "removeNotification", "()V");
	}
	jRemoveNotification(env, jBaseActivity);
}

void addLauncherIcon(const char *name, const char *path)
{
	logMsg("adding launcher icon: %s, for path: %s", name, path);
	if(unlikely(!jAddViewShortcut))
	{
		jAddViewShortcut.setup(jEnv(), jBaseActivityCls, "addViewShortcut", "(Ljava/lang/String;Ljava/lang/String;)V");
	}
	jAddViewShortcut(jEnv(), jBaseActivity, jEnv()->NewStringUTF(name), jEnv()->NewStringUTF(path));
}

void handleIntent(JNIEnv *env, jobject activity)
{
	if(!onInterProcessMessage())
		return;
	// check for view intents
	if(!jIntentDataPath)
	{
		jIntentDataPath.setup(env, jBaseActivityCls, "intentDataPath", "()Ljava/lang/String;");
	}
	jstring intentDataPathJStr = (jstring)jIntentDataPath(env, activity);
	if(intentDataPathJStr)
	{
		const char *intentDataPathStr = env->GetStringUTFChars(intentDataPathJStr, nullptr);
		logMsg("got intent with path: %s", intentDataPathStr);
		dispatchOnInterProcessMessage(intentDataPathStr);
		env->ReleaseStringUTFChars(intentDataPathJStr, intentDataPathStr);
	}
}

void openURL(const char *url)
{
	// TODO
}

void registerInstance(const char *appID, int argc, char** argv) {}

void setAcceptIPC(const char *appID, bool on) {}

}
