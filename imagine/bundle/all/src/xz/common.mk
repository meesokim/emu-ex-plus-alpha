ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

include $(buildSysPath)/imagineSDKPath.mk

xzVer := 5.2.2
xzSrcDir := $(tempDir)/xz-$(xzVer)
xzSrcArchive := xz-$(xzVer).tar.gz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/liblzma/.libs/liblzma.a
installIncludeDir := $(installDir)/include

cpuIsBigEndian := no

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing xz to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(xzSrcDir)/src/liblzma/api/lzma.h $(installIncludeDir)/
	cp -r $(xzSrcDir)/src/liblzma/api/lzma $(installIncludeDir)/
	cp $(buildDir)/src/liblzma/liblzma.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(xzSrcDir)/configure : | $(xzSrcArchive)
	@echo "Extracting xz..."
	@mkdir -p $(xzSrcDir)
	tar -mxzf $| -C $(xzSrcDir)/..
	cp ../gnuconfig/config.* $(xzSrcDir)/build-aux/

$(outputLibFile) : $(makeFile)
	@echo "Building xz..."
	$(MAKE) -C $(<D)

$(makeFile) : $(xzSrcDir)/configure
	@echo "Configuring xz..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && $(toolchainEnvParams) \
	CFLAGS="$(CPPFLAGS) $(CFLAGS)" \
	LDFLAGS="$(LDFLAGS) $(LDLIBS)" \
	ac_cv_c_bigendian=$(cpuIsBigEndian) \
	$(xzSrcDir)/configure \
	--prefix='$$$${pcfiledir}/../..' \
	--disable-xz \
	--disable-xzdec \
	--disable-lzmadec \
	--disable-lzmainfo \
	--disable-rpath \
	--disable-shared \
	--host=$(CHOST) \
	PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
	PKG_CONFIG=pkg-config \
	$(buildArg)
