makefilesToRun='
	src/btstack/ios-arm64.mk

	src/libogg/ios-arm64.mk
	
	src/libvorbis/ios-arm64.mk
	
	src/libsndfile/ios-arm64.mk
	
	src/xz/ios-arm64.mk
	
	src/libarchive/ios-arm64.mk
	
	src/boost/ios-arm64.mk
'

source runMakefiles.sh

runMakefiles $@

