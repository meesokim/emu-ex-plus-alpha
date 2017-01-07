ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc \
main/input.cc \
main/options.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/FceuApi.cc \
main/Cheats.cc

CPPFLAGS += -I$(projectPath)/src \
-DHAVE_ASPRINTF \
-DPSS_STYLE=1 \
-DLSB_FIRST \
-DFRAMESKIP \
-I$(projectPath)/src/fceu

# fceux sources
FCEUX_SRC := fceu/cart.cpp \
fceu/cheat.cpp \
fceu/emufile.cpp \
fceu/fceu.cpp \
fceu/file.cpp \
fceu/filter.cpp \
fceu/ines.cpp \
fceu/input.cpp \
fceu/palette.cpp \
fceu/ppu.cpp \
fceu/sound.cpp \
fceu/state.cpp \
fceu/unif.cpp \
fceu/vsuni.cpp \
fceu/x6502.cpp \
fceu/movie.cpp \
fceu/fds.cpp \
fceu/utils/crc32.cpp \
fceu/utils/md5.cpp \
fceu/utils/memory.cpp \
fceu/utils/xstring.cpp \
fceu/utils/endian.cpp \
fceu/utils/general.cpp \
fceu/utils/guid.cpp \
fceu/input/mouse.cpp \
fceu/input/oekakids.cpp \
fceu/input/powerpad.cpp \
fceu/input/quiz.cpp \
fceu/input/shadow.cpp \
fceu/input/suborkb.cpp \
fceu/input/toprider.cpp \
fceu/input/zapper.cpp \
fceu/input/bworld.cpp \
fceu/input/arkanoid.cpp \
fceu/input/mahjong.cpp \
fceu/input/fkb.cpp \
fceu/input/ftrainer.cpp \
fceu/input/hypershot.cpp \
fceu/input/cursor.cpp \
fceu/input/snesmouse.cpp \
fceu/input/pec586kb.cpp

BOARDS_SRC := $(subst $(projectPath)/src/,,$(filter %.cpp %.c, $(wildcard $(projectPath)/src/fceu/boards/*)))
FCEUX_SRC += $(BOARDS_SRC)
FCEUX_OBJ := $(addprefix $(objDir)/,$(FCEUX_SRC:.cpp=.o))
SRC += $(FCEUX_SRC)

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
