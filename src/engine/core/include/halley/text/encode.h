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
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "halleystring.h"
#include <gsl/span>

namespace Halley {
	namespace Encode {
		String encodeBase16(gsl::span<const gsl::byte> in);
		void decodeBase16(std::string_view in, gsl::span<gsl::byte> bytes);

		String encodeBase64(gsl::span<const gsl::byte> in);
		Bytes decodeBase64(std::string_view in);
		void decodeBase64(std::string_view in, gsl::span<gsl::byte> out);
		size_t getBase64Length(std::string_view in);

		template<typename T>
		void decodeBase64(std::string_view in, Vector<T>& out)
		{
			out.resize(getBase64Length(in));
			decodeBase64(in, out.byte_span());
		}

		Vector<char> encodeRLE(const Vector<char>& in);
		Vector<char> decodeRLE(const Vector<char>& in);
	}
}