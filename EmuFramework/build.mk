include $(IMAGINE_PATH)/make/imagineStaticLibBase.mk

ifneq ($(filter linux ios android webos,$(ENV)),)
 emuFramework_onScreenControls := 1
endif

SRC += CreditsView.cc \
MsgPopup.cc \
FilePicker.cc \
EmuSystem.cc \
Screenshot.cc \
ButtonConfigView.cc \
VideoImageOverlay.cc \
StateSlotView.cc \
MenuView.cc \
EmuInput.cc \
EmuOptions.cc \
OptionView.cc \
EmuView.cc \
ConfigFile.cc \
InputManagerView.cc \
FileUtils.cc \
EmuApp.cc \
BundledGamesView.cc \
VideoImageEffect.cc \
EmuVideo.cc \
EmuInputView.cc \
EmuVideoLayer.cc \
Cheats.cc \
Recent.cc \
EmuLoadProgressView.cc

ifeq ($(emuFramework_onScreenControls), 1)
 SRC += TouchConfigView.cc \
 VController.cc
endif

libName := emuframework$(libNameExt)
ifndef RELEASE
 libName := $(libName)-debug
endif

target := lib$(libName)

targetDir := lib/$(buildName)

prefix ?= $(IMAGINE_SDK_PLATFORM_PATH)

include $(projectPath)/metadata/conf.mk

imaginePkgconfigTemplate := $(projectPath)/pkgconfig/emuframework.pc
pkgName := EmuFramework
pkgDescription := Emulator App Framework
pkgVersion := $(metadata_version)
LDLIBS := -l$(libName) $(LDLIBS)

include $(IMAGINE_PATH)/make/package/imagine.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineStaticLibTarget.mk

install : main
	@echo "Installing lib & headers to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)/lib/pkgconfig $(prefix)/include/
	$(PRINT_CMD)cp lib/$(buildName)/lib$(libName).a $(prefix)/lib/
	$(PRINT_CMD)cp lib/$(buildName)/$(libName).pc $(prefix)/lib/pkgconfig/
	$(PRINT_CMD)cp -r $(projectPath)/include/emuframework $(prefix)/include/

install-links : main
	@echo "Installing symlink lib & headers to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)/lib/pkgconfig $(prefix)/include/
	$(PRINT_CMD)$(LN) -srf lib/$(buildName)/lib$(libName).a $(prefix)/lib/
	$(PRINT_CMD)$(LN) -srf lib/$(buildName)/$(libName).pc $(prefix)/lib/pkgconfig/
	$(PRINT_CMD)$(LN) -srf $(projectPath)/include/emuframework $(prefix)/include/
