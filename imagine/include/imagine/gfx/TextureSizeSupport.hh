#pragma once

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/math/int.hh>
#include <algorithm>

class TextureSizeSupport
{
public:
	bool nonPow2 = Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2;
	static constexpr bool nonSquare = true;
	bool nonPow2CanMipmap = false;
	bool nonPow2CanRepeat = false;
	uint maxXSize = 0, maxYSize = 0;
	static constexpr bool forcePow2 = false;

	constexpr TextureSizeSupport() {}

	IG::PixmapDesc makePixmapDescWithSupportedSize(IG::PixmapDesc desc)
	{
		return {makeSupportedSize(desc.size()), desc.format()};
	}

	IG::WP makeSupportedSize(IG::WP size)
	{
		using namespace IG;
		IG::WP supportedSize;
		if(nonPow2 && !forcePow2)
		{
			supportedSize = size;
		}
		else if(nonSquare)
		{
			supportedSize = {(int)roundUpPowOf2((uint)size.x), (int)roundUpPowOf2((uint)size.y)};
		}
		else
		{
			supportedSize.x = supportedSize.y = roundUpPowOf2((uint)std::max(size.x, size.y));
		}
		if(Config::MACHINE_IS_PANDORA && (supportedSize.x <= 16 || supportedSize.y <= 16))
		{
			// force small textures as square due to PowerVR driver bug
			supportedSize.x = supportedSize.y = std::max(supportedSize.x, supportedSize.y);
		}
		return supportedSize;
	}

	bool supportsMipmaps(uint imageX, uint imageY)
	{
		return imageX && imageY &&
			(nonPow2CanMipmap || (IG::isPowerOf2(imageX) && IG::isPowerOf2(imageY)));
	}
};
