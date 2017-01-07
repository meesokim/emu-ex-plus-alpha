/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <archive.h>
#include <archive_entry.h>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include "ziphelper.h"

static struct archive *writeArch{};

void zipCacheReadOnlyZip(const char* zipName)
{
	// TODO
}

void* zipLoadFile(const char* zipName, const char* fileName, int* size)
{
	ArchiveIO io{};
	std::error_code ec{};
	for(auto &entry : FS::ArchiveIterator{zipName, ec})
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		auto name = entry.name();
		//logMsg("archive file entry:%s", entry.name());
		if(string_equal(name, fileName))
		{
			io = entry.moveIO();
			int fileSize = io.size();
			void *buff = malloc(fileSize);
			io.read(buff, fileSize);
			*size = fileSize;
			return buff;
		}
	}
	if(ec)
	{
		logErr("error opening archive:%s", zipName);
		return nullptr;
	}
	else
	{
		logErr("file %s not in archive:%s", fileName, zipName);
		return nullptr;
	}
}

CallResult zipStartWrite(const char *fileName)
{
	assert(!writeArch);
	writeArch = archive_write_new();
	archive_write_set_format_zip(writeArch);
	if(archive_write_open_filename(writeArch, fileName) != ARCHIVE_OK)
	{
		archive_write_free(writeArch);
		writeArch = {};
		return IO_ERROR;
	}
	return OK;
}

int zipSaveFile(const char* zipName, const char* fileName, int append, const void* buffer, int size)
{
	assert(writeArch);
	auto entry = archive_entry_new();
	auto freeEntry = IG::scopeGuard([&](){ archive_entry_free(entry); });
	archive_entry_set_pathname(entry, fileName);
	archive_entry_set_size(entry, size);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0644);
	if(archive_write_header(writeArch, entry) < ARCHIVE_OK)
	{
		logErr("error writing archive header: %s", archive_error_string(writeArch));
		return 0;
	}
	if(archive_write_data(writeArch, buffer, size) < ARCHIVE_OK)
	{
		logErr("error writing archive data: %s", archive_error_string(writeArch));
		return 0;
	}
	return 1;
}

void zipEndWrite()
{
	assert(writeArch);
	archive_write_close(writeArch);
	archive_write_free(writeArch);
	writeArch = {};
}
