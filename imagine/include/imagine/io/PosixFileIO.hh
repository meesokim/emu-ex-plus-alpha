#pragma once

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

#include <imagine/io/PosixIO.hh>
#include <imagine/fs/FSDefs.hh>
#include <type_traits>

class PosixFileIO : public IOUtils<PosixFileIO>
{
public:
	using IOUtils::read;
	using IOUtils::readAtPos;
	using IOUtils::write;
	using IOUtils::seek;

	PosixFileIO();
	~PosixFileIO();
	PosixFileIO(PosixFileIO &&o);
	PosixFileIO &operator=(PosixFileIO &&o);
	operator IO*(){ return &io(); }
	operator IO&(){ return io(); }
	GenericIO makeGeneric();
	std::error_code open(const char *path, uint mode = 0);

	std::error_code open(FS::PathString path, uint mode = 0)
	{
		return open(path.data(), mode);
	}

	std::error_code create(const char *path, uint mode = 0)
	{
		mode |= IO::OPEN_WRITE | IO::OPEN_CREATE;
		return open(path, mode);
	}

	std::error_code create(FS::PathString path, uint mode = 0)
	{
		return create(path.data(), mode);
	}

	ssize_t read(void *buff, size_t bytes, std::error_code *ecOut);
	ssize_t readAtPos(void *buff, size_t bytes, off_t offset, std::error_code *ecOut);
	const char *mmapConst();
	ssize_t write(const void *buff, size_t bytes, std::error_code *ecOut);
	std::error_code truncate(off_t offset);
	off_t seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut);
	void close();
	void sync();
	size_t size();
	bool eof();
	void advise(off_t offset, size_t bytes, IO::Advice advice);
	explicit operator bool();

protected:
	std::aligned_union_t<0, PosixIO, BufferMapIO> ioStorage;
	bool usingMapIO = false;

	IO &io();
	PosixIO &posixIO()
	{
		return *(PosixIO*)&ioStorage;
	}
	BufferMapIO &bufferMapIO()
	{
		return *(BufferMapIO*)&ioStorage;
	}
};
