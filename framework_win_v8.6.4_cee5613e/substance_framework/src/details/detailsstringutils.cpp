/*************************************************************************
* ADOBE CONFIDENTIAL
* ___________________ *
*  Copyright 2020-2022 Adobe
*  All Rights Reserved.
* * NOTICE:  All information contained herein is, and remains
* the property of Adobe and its suppliers, if any. The intellectual
* and technical concepts contained herein are proprietary to Adobe
* and its suppliers and are protected by all applicable intellectual
* property laws, including trade secret and copyright laws.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe.
**************************************************************************/

#include <substance/framework/typedefs.h>
#include "detailsstringutils.h"

#include <utf8.h>

namespace SubstanceAir
{
namespace Details
{
namespace StringUtils
{
string utf16ToUtf8(const u16string& source)
{
	string convertedString;
	utf8::unchecked::utf16to8(source.begin(), source.end(), back_inserter(convertedString));
	return convertedString;
}

u16string utf8ToUtf16(const string& source)
{
	u16string convertedString;
	utf8::unchecked::utf8to16(source.begin(), source.end(), back_inserter(convertedString));
	return convertedString;
}

ucs4string utf8ToUcs4(const string& source)
{
	ucs4string result;

	const unsigned char* srcbeg = (const unsigned char*)&source[0];
	const unsigned char* const srcend = srcbeg + source.size();

	while (srcbeg < srcend)
	{
		unsigned int firstutf8 = *(srcbeg++);
		unsigned int resucs4 = 0u;

		// The character is not an ascii character, so a sequence must be converted
		if (firstutf8 >= 128u)
		{
			unsigned int seqsize = 0u;
			while (((firstutf8 << seqsize) & 0xC0u) == 0xC0u
					&& seqsize < 3u && srcbeg < srcend)
			{
				resucs4 = (resucs4 << 6u) | (*(srcbeg++) & 0x3Fu);
				++seqsize;
			}

			resucs4 |= (firstutf8 & (0x3Fu >> seqsize)) << (seqsize * 6u);
		}
		else
		{
			resucs4 = firstutf8;
		}

		result.push_back(resucs4);
	}

	// Append a null character and return
	result.push_back(0u);

	return result;
}
} // namespace StringUtils
} // namespace Details
} // namespace SubstanceAir
