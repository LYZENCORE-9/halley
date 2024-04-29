/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		                                            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include <gsl/gsl>
#include "halley/maths/vector2.h"
#include "halley/text/halleystring.h"
#include "halley/resources/resource.h"
#include "halley/file/path.h"
#include "halley/maths/rect.h"
#include "halley/maths/colour.h"

namespace Halley {
	class ResourceDataStatic;
	class ResourceLoader;

	class Image final : public Resource {
	public:
		enum class Format : uint8_t {
			Undefined,
			Indexed,
			RGB,
			RGBA,
			RGBAPremultiplied,
			SingleChannel
		};

		Image(Format format = Format::RGBA, Vector2i size = {}, bool clear = true);
		Image(gsl::span<const gsl::byte> bytes, Format format = Format::Undefined);
		explicit Image(const ResourceDataStatic& data);
		Image(const ResourceDataStatic& data, const Metadata& meta);
		Image(Image&& other);
		~Image();

		std::unique_ptr<Image> clone();

		void setSize(Vector2i size, bool clear = true);

		void load(gsl::span<const gsl::byte> bytes, Format format = Format::Undefined, const Path& originalPath = {});

		Bytes savePNGToBytes(bool allowDepthReduce = true) const;
		Bytes saveQOIToBytes() const;
		Bytes saveHLIFToBytes(std::string_view name = {}, bool lz4hc = true) const;

		static Vector2i getImageSize(gsl::span<const gsl::byte> bytes);
		void setFormat(Format format);

		static bool isQOI(gsl::span<const gsl::byte> bytes);
		static bool isPNG(gsl::span<const gsl::byte> bytes);

		gsl::span<unsigned char> getPixelBytes();
		gsl::span<const unsigned char> getPixelBytes() const;
		gsl::span<unsigned char> getPixelBytesRow(int x0, int x1, int y);
		gsl::span<const unsigned char> getPixelBytesRow(int x0, int x1, int y) const;
		
		int getPixel4BPP(Vector2i pos) const;
		int getPixelAlpha(Vector2i pos) const;
		gsl::span<unsigned char> getPixels1BPP();
		gsl::span<const unsigned char> getPixels1BPP() const;
		gsl::span<int> getPixels4BPP();
		gsl::span<const int> getPixels4BPP() const;
		gsl::span<const int> getPixelRow4BPP(int x0, int x1, int y) const;
		size_t getByteSize() const;

		constexpr static unsigned int convertRGBAToInt(unsigned int r, unsigned int g, unsigned int b, unsigned int a=255)
		{
			return (a << 24) | (b << 16) | (g << 8) | r;
		}

		constexpr static unsigned int convertColourToInt(Colour4c col)
		{
			return (static_cast<unsigned>(col.a) << 24) | (static_cast<unsigned>(col.b) << 16) | (static_cast<unsigned>(col.g) << 8) | static_cast<unsigned>(col.r);
		}

		constexpr static void convertIntToRGBA(unsigned int col, unsigned int& r, unsigned int& g, unsigned int& b, unsigned int& a)
		{
			r = col & 0xFF;
			g = (col >> 8) & 0xFF;
			b = (col >> 16) & 0xFF;
			a = (col >> 24) & 0xFF;
		}

		constexpr static Colour4c convertIntToColour(unsigned int col)
		{
			Colour4c result;
			result.r = col & 0xFF;
			result.g = (col >> 8) & 0xFF;
			result.b = (col >> 16) & 0xFF;
			result.a = (col >> 24) & 0xFF;
			return result;
		}

		unsigned int getWidth() const { return w; }
		unsigned int getHeight() const { return h; }
		Vector2i getSize() const { return Vector2i(int(w), int(h)); }

		int getBytesPerPixel() const;
		Format getFormat() const;

		Rect4i getTrimRect() const;
		Rect4i getRect() const;

		void clear(int colour);
		void blitFrom(Vector2i pos, gsl::span<const unsigned char> buffer, size_t width, size_t height, size_t pitch, size_t srcBpp);
		void blitFromRotated(Vector2i pos, gsl::span<const unsigned char> buffer, size_t width, size_t height, size_t pitch, size_t bpp);
		void blitFrom(Vector2i pos, const Image& srcImg, bool rotated = false);
		void blitFrom(Vector2i pos, const Image& srcImg, Rect4i srcArea, bool rotated = false);
		void blitDownsampled(Image& src, int scale);

		void drawImageAlpha(const Image& src, Vector2i pos, uint8_t opacity = 255);
		void drawImageAdd(const Image& src, Vector2i pos, uint8_t opacity = 255);
		void drawImageLighten(const Image& src, Vector2i pos, uint8_t opacity = 255);

		static std::unique_ptr<Image> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Image; }
		void reload(Resource&& resource) override;

		Image& operator=(const Image& o) = delete;
		Image& operator=(Image&& o) = default;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void preMultiply();

		void flipVertically();

		ResourceMemoryUsage getMemoryUsage() const override;

	private:
		std::unique_ptr<unsigned char, void(*)(unsigned char*)> px;
		size_t dataLen = 0;
		unsigned int w = 0;
		unsigned int h = 0;
		Format format = Format::Undefined;
	};

	template <>
	struct EnumNames<Image::Format> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"undefined",
				"indexed",
				"rgb",
				"rgba",
				"rgba_premultiplied",
				"single_channel"
			}};
		}
	};
}
