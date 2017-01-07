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

#define LOGTAG "ResFontFreetype"
#include <imagine/font/Font.hh>
#include <imagine/logger/logger.h>
#include <imagine/gfx/Gfx.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include <imagine/io/FileIO.hh>
#ifdef CONFIG_PACKAGE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_SIZES_H

static FT_Library library{};

#ifdef CONFIG_PACKAGE_FONTCONFIG
static FcConfig *fcConf{};

static FS::PathString fontPathWithPattern(FcPattern *pat)
{
	if(!FcConfigSubstitute(nullptr, pat, FcMatchPattern))
	{
		logErr("error applying font substitutions");
		return {};
	}
	FcDefaultSubstitute(pat);
	FcResult result = FcResultMatch;
	auto matchPat = FcFontMatch(nullptr, pat, &result);
	auto destroyPattern = IG::scopeGuard(
		[&]()
		{
			if(matchPat)
			{
				FcPatternDestroy(matchPat);
			}
		});
	if(!matchPat || result == FcResultNoMatch)
	{
		logErr("fontconfig couldn't find a valid font");
		return {};
	}
	FcChar8 *patternStr;
	if(FcPatternGetString(matchPat, FC_FILE, 0, &patternStr) != FcResultMatch)
	{
		logErr("fontconfig font missing file path");
		return {};
	}
	FS::PathString path;
	string_copy(path, (char*)patternStr);
	return path;
}

static FS::PathString fontPathContainingChar(int c, int weight)
{
	if(!fcConf)
	{
		fcConf = FcInitLoadConfigAndFonts();
		if(!fcConf)
		{
			logErr("error initializing fontconfig");
			return {};
		}
	}
	logMsg("looking for font with char: %c", c);
	auto pat = FcPatternCreate();
	if(!pat)
	{
		logErr("error allocating fontconfig pattern");
		return {};
	}
	auto destroyPattern = IG::scopeGuard([&](){ FcPatternDestroy(pat); });
	auto charSet = FcCharSetCreate();
	if(!charSet)
	{
		logErr("error allocating fontconfig char set");
		return {};
	}
	auto destroyCharSet = IG::scopeGuard([&](){ FcCharSetDestroy(charSet); });
	FcCharSetAddChar(charSet, c);
	FcPatternAddCharSet(pat, FC_CHARSET, charSet);
	FcPatternAddInteger(pat, FC_WEIGHT, weight);
	FS::PathString path = fontPathWithPattern(pat);
	if(!strlen(path.data()))
	{
		return {};
	}
	return path;
}

[[gnu::destructor]] static void finalizeFc()
{
	if(fcConf)
	{
		FcConfigDestroy(fcConf);
		FcFini();
	}
}
#endif

namespace IG
{

static FT_Size makeFTSize(FT_Face face, int x, int y, std::error_code &ec)
{
	logMsg("creating new size object, %dx%d pixels", x, y);
	FT_Size size{};
	auto error = FT_New_Size(face, &size);
	if(error)
	{
		logErr("error creating new size object");
		ec = {EINVAL, std::system_category()};
		return {};
	}
	error = FT_Activate_Size(size);
	if(error)
	{
		logErr("error activating size object");
		ec = {EINVAL, std::system_category()};
		return {};
	}
	error = FT_Set_Pixel_Sizes(face, x, y);
	if(error)
	{
		logErr("error occurred setting character pixel size");
		ec = {EINVAL, std::system_category()};
		return {};
	}
	ec = {};
	//logMsg("Face max bounds %dx%d,%dx%d, units per EM %d", face->bbox.xMin, face->bbox.xMax, face->bbox.yMin, face->bbox.yMax, face->units_per_EM);
	//logMsg("scaled ascender x descender %dx%d", (int)size->metrics.ascender >> 6, (int)size->metrics.descender >> 6);
	return size;
}

static FreetypeFont::GlyphRenderData makeGlyphRenderDataWithFace(FT_Face face, int c, bool keepPixData, std::error_code &ec)
{
	auto idx = FT_Get_Char_Index(face, c);
	if(!idx)
	{
		ec = {EINVAL, std::system_category()};
		return {};
	}
	auto error = FT_Load_Glyph(face, idx, FT_LOAD_RENDER);
	if(error)
	{
		logErr("error occurred loading/rendering character 0x%X", c);
		ec = {EINVAL, std::system_category()};
		return {};
	}
	auto &glyph = face->glyph;
	FT_Bitmap bitmap{};
	if(glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
	{
		// rendered glyph is not in 8-bit gray-scale
		logMsg("converting mode %d bitmap", glyph->bitmap.pixel_mode);
		auto error = FT_Bitmap_Convert(library, &glyph->bitmap, &bitmap, 1);
		if(error)
		{
			logErr("error occurred converting character 0x%X", c);
			ec = {EINVAL, std::system_category()};
			return {};
		}
		assert(bitmap.num_grays == 2); // only handle 2 gray levels for now
		//logMsg("new bitmap has %d gray levels", convBitmap.num_grays);
		// scale 1-bit values to 8-bit range
		iterateTimes(bitmap.rows, y)
		{
			iterateTimes(bitmap.width, x)
			{
				if(bitmap.buffer[(y * bitmap.pitch) + x] != 0)
					bitmap.buffer[(y * bitmap.pitch) + x] = 0xFF;
			}
		}
		FT_Bitmap_Done(library, &glyph->bitmap);
	}
	else
	{
		bitmap = glyph->bitmap;
		glyph->bitmap = {};
	}
	GlyphMetrics metrics;
	metrics.xSize = bitmap.width;
	metrics.ySize = bitmap.rows;
	metrics.xOffset = glyph->bitmap_left;
	metrics.yOffset = glyph->bitmap_top;
	metrics.xAdvance = glyph->advance.x >> 6;;
	if(!keepPixData)
	{
		FT_Bitmap_Done(library, &bitmap);
	}
	ec = {};
	return {metrics, bitmap};
}

std::error_code FreetypeFaceData::openFont(GenericIO file)
{
	if(!file)
		return {EINVAL, std::system_category()};
	if(unlikely(!library))
	{
		auto error = FT_Init_FreeType(&library);
		if(error)
		{
			logErr("error in FT_Init_FreeType");
			return {EINVAL, std::system_category()};
		}
		/*FT_Int major, minor, patch;
		FT_Library_Version(library, &major, &minor, &patch);
		logMsg("init freetype version %d.%d.%d", (int)major, (int)minor, (int)patch);*/
	}
	streamRec.size = file.size();
	auto fileOffset =	file.tell();
	streamRec.pos = fileOffset;
	streamRec.descriptor.pointer = file.release();
	streamRec.read = [](FT_Stream stream, unsigned long offset,
		unsigned char* buffer, unsigned long count) -> unsigned long
		{
			auto &io = *((IO*)stream->descriptor.pointer);
			stream->pos = offset;
			if(!count) // no bytes to read
				return 0;
			auto bytesRead = io.readAtPos(buffer, count, offset);
			if(bytesRead == -1)
			{
				logErr("error reading bytes in IO func");
				return 0;
			}
			return bytesRead;
		};
	FT_Open_Args openS{};
	openS.flags = FT_OPEN_STREAM;
	openS.stream = &streamRec;
	auto error = FT_Open_Face(library, &openS, 0, &face);
	if(error == FT_Err_Unknown_File_Format)
	{
		logErr("unknown font format");
		return {EINVAL, std::system_category()};
	}
	else if(error)
	{
		logErr("error occurred opening the font");
		return {EIO, std::system_category()};
	}
	return {};
}

Font::Font() {}

Font::Font(GenericIO io)
{
	loadIntoNextSlot(std::move(io));
}

Font::Font(const char *name)
{
	FileIO io;
	io.open(name);
	if(!io)
	{
		logMsg("unable to open file");
		return;
	}
	loadIntoNextSlot(io);
}

Font Font::makeSystem()
{
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	logMsg("locating system fonts with fontconfig");
	// Let fontconfig handle loading specific fonts on-demand
	return {};
	#else
	return makeFromAsset("Vera.ttf");
	#endif
}

Font Font::makeBoldSystem()
{
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	Font font = makeSystem();
	font.isBold = true;
	return font;
	#else
	return makeFromAsset("Vera.ttf");
	#endif
}

Font Font::makeFromAsset(const char *name)
{
	return {openAppAssetIO(name)};
}

Font::Font(Font &&o)
{
	swap(*this, o);
}

Font &Font::operator=(Font o)
{
	swap(*this, o);
	return *this;
}

Font::~Font()
{
	for(auto &e : f)
	{
		if(e.face)
		{
			FT_Done_Face(e.face);
		}
		if(e.streamRec.descriptor.pointer)
		{
			delete (IO*)e.streamRec.descriptor.pointer;
		}
	}
}

void Font::swap(Font &a, Font &b)
{
	std::swap(a.f, b.f);
	std::swap(a.isBold, b.isBold);
}

Font::operator bool() const
{
	return f.size();
}

std::error_code FreetypeFont::loadIntoNextSlot(GenericIO io)
{
	if(f.isFull())
		return {ENOENT, std::system_category()};
	f.emplace_back();
	auto ec = f.back().openFont(std::move(io));
	if(ec)
	{
		logErr("error reading font");
		f.pop_back();
		return ec;
	}
	return {};
}

std::error_code FreetypeFont::loadIntoNextSlot(const char *name)
{
	if(f.isFull())
		return {ENOENT, std::system_category()};
	FileIO io;
	io.open(name);
	if(!io)
	{
		logMsg("unable to open file %s", name);
		return {EINVAL, std::system_category()};
	}
	auto ec = loadIntoNextSlot(io);
	if(ec)
	{
		return ec;
	}
	return {};
}

FreetypeFont::GlyphRenderData FreetypeFont::makeGlyphRenderData(int idx, FreetypeFontSize &fontSize, bool keepPixData, std::error_code &ec)
{
	iterateTimes(f.size(), i)
	{
		auto &font = f[i];
		if(!font.face)
			continue;
		auto ftError = FT_Activate_Size(fontSize.ftSize[i]);
		if(ftError)
		{
			logErr("error activating size object");
			ec = {EINVAL, std::system_category()};
			return {};
		}
		std::error_code ec;
		auto data = makeGlyphRenderDataWithFace(font.face, idx, keepPixData, ec);
		if(ec)
		{
			logMsg("glyph 0x%X not found in slot %d", idx, i);
			continue;
		}
		return data;
	}
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	// try to find a font with the missing char and load into next free slot
	if(f.isFull())
	{
		logErr("no slots left");
		ec = {ENOSPC, std::system_category()};
		return {};
	}
	auto fontPath = fontPathContainingChar(idx, isBold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM);
	if(!strlen(fontPath.data()))
	{
		logErr("no font file found for char %c (0x%X)", idx, idx);
		ec = {ENOENT, std::system_category()};
		return {};
	}
	uint newSlot = f.size();
	ec = loadIntoNextSlot(fontPath.data());
	if(ec)
		return {};
	auto &font = f[newSlot];
	fontSize.ftSize[newSlot] = makeFTSize(font.face, fontSize.settings.pixelWidth(), fontSize.settings.pixelHeight(), ec);
	if(ec)
	{
		logErr("couldn't allocate font size");
		return {};
	}
	auto data = makeGlyphRenderDataWithFace(font.face, idx, keepPixData, ec);
	if(ec)
	{
		logMsg("glyph 0x%X still not found", idx);
		return {};
	}
	return data;
	#else
	ec = {EINVAL, std::system_category()};
	return {};
	#endif
}

Font::Glyph Font::glyph(int idx, FontSize &size, std::error_code &ec)
{
	auto data = makeGlyphRenderData(idx, size, true, ec);
	if(ec)
	{
		return {};
	}
	return {{data.bitmap}, data.metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size, std::error_code &ec)
{
	auto data = makeGlyphRenderData(idx, size, false, ec);
	if(ec)
	{
		return {};
	}
	return data.metrics;
}

FontSize Font::makeSize(FontSettings settings, std::error_code &ec)
{
	FontSize size{settings};
	// create FT_Size objects for slots in use
	iterateTimes(f.size(), i)
	{
		if(!f[i].face)
		{
			continue;
		}
		size.ftSize[i] = makeFTSize(f[i].face, settings.pixelWidth(), settings.pixelHeight(), ec);
		if(ec)
		{
			return {};
		}
	}
	ec = {};
	return size;
}

FreetypeFontSize::~FreetypeFontSize()
{
	iterateTimes(IG::size(ftSize), i)
	{
		if(ftSize[i])
		{
			//logMsg("freeing size %p", ftSize[i]);
			auto error = FT_Done_Size(ftSize[i]);
			assert(!error);
		}
	}
}

FreetypeFontSize::FreetypeFontSize(FreetypeFontSize &&o)
{
	swap(*this, o);
}

FreetypeFontSize &FreetypeFontSize::operator=(FreetypeFontSize o)
{
	swap(*this, o);
	return *this;
}

void FreetypeFontSize::swap(FreetypeFontSize &a, FreetypeFontSize &b)
{
	std::swap(a.settings, b.settings);
	std::swap(a.ftSize, b.ftSize);
}

GlyphImage::GlyphImage(GlyphImage &&o)
{
	swap(*this, o);
}

GlyphImage &GlyphImage::operator=(GlyphImage o)
{
	swap(*this, o);
	return *this;
}

void GlyphImage::swap(GlyphImage &a, GlyphImage &b)
{
	std::swap(a.bitmap, b.bitmap);
}

void GlyphImage::unlock()
{
	if(bitmap.buffer)
	{
		FT_Bitmap_Done(library, &bitmap);
	}
}

IG::Pixmap IG::GlyphImage::pixmap()
{
	return
		{
			{{(int)bitmap.width, (int)bitmap.rows}, IG::PIXEL_FMT_A8},
			bitmap.buffer,
			{(uint)bitmap.pitch, IG::Pixmap::BYTE_UNITS}
		};
}

IG::GlyphImage::operator bool() const
{
	return (bool)bitmap.buffer;
}

}
