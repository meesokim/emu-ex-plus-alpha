ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk
include $(imagineSrcDir)/input/build.mk

LDLIBS += -lpthread

SRC += base/linux/linux.cc \
 base/linux/DRMFrameTimer.cc \
 base/common/timer/TimerFD.cc \
 base/common/PosixPipe.cc \
 util/string/glibc.c

include $(IMAGINE_PATH)/make/package/libdrm.mk

linuxWinSystem ?= x11

ifeq ($(linuxWinSystem), x11)
 include $(imagineSrcDir)/base/x11/build.mk
endif

linuxEventLoop ?= glib

ifeq ($(linuxEventLoop), glib)
 configDefs += CONFIG_BASE_GLIB
 SRC += base/common/eventloop/GlibEventLoop.cc
 include $(IMAGINE_PATH)/make/package/glib.mk
endif

ifneq ($(SUBENV), pandora)
 configDefs += CONFIG_BASE_DBUS
 SRC += base/linux/dbus.cc
 ifeq ($(linuxEventLoop), glib)
  include $(IMAGINE_PATH)/make/package/dbus-glib.mk
 else
  include $(IMAGINE_PATH)/make/package/dbus.mk
 endif
endif

endif
