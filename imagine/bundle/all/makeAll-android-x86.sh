makefilesToRun='
	src/libogg/android-x86.mk
	
	src/libvorbis/android-x86.mk
	
	src/libsndfile/android-x86.mk
	
	src/xz/android-x86.mk
	
	src/libarchive/android-x86.mk
	
	src/boost/android-x86.mk
'

source runMakefiles.sh

runMakefiles $@

