makefilesToRun='
	src/libpng/linux-x86.mk
	
	src/libarchive/linux-x86.mk
'
source runMakefiles.sh

runMakefiles $@

