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

#include <imagine/config/defs.hh>
#include <imagine/util/operators.hh>
#include <imagine/io/ArchiveIO.hh>
#include <system_error>

namespace FS
{

class ArchiveIterator :
	public std::iterator<std::input_iterator_tag, ArchiveEntry>,
	public NotEquals<ArchiveIterator>
{
public:
	ArchiveEntry archEntry{};

	ArchiveIterator() {}
	ArchiveIterator(PathString path): ArchiveIterator{path.data()} {}
	ArchiveIterator(const char *path);
	ArchiveIterator(PathString path, std::error_code &result): ArchiveIterator{path.data(), result} {}
	ArchiveIterator(const char *path, std::error_code &result);
	ArchiveIterator(GenericIO io);
	ArchiveIterator(GenericIO io, std::error_code &result);
	ArchiveIterator(ArchiveEntry &&entry): archEntry{std::move(entry)} {}
	~ArchiveIterator();
	ArchiveEntry& operator*();
	ArchiveEntry* operator->();
	void operator++();
	bool operator==(ArchiveIterator const &rhs) const;
	void rewind();

private:
	void init(const char *path, std::error_code &result);
	void init(GenericIO io, std::error_code &result);
};

static const ArchiveIterator &begin(const ArchiveIterator &iter)
{
	return iter;
}

static ArchiveIterator end(const ArchiveIterator &)
{
	return {};
}

ArchiveIO fileFromArchive(const char *archivePath, const char *filePath);
static ArchiveIO fileFromArchive(PathString archivePath, PathString filePath)
{
	return fileFromArchive(archivePath.data(), filePath.data());
}

};
