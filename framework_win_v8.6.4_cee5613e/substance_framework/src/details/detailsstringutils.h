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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSTRINGUTILS_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSTRINGUTILS_H

#include <substance/framework/typedefs.h>

namespace SubstanceAir
{
namespace Details
{
namespace StringUtils
{
//! @brief Convert a utf-16 encoded string to a utf-8 encoded string
string utf16ToUtf8(const u16string& source);

//! @brief Convert a utf-8 encoded string to a utf-16 encoded string
u16string utf8ToUtf16(const string& source);

//! @brief Convert a utf-8 encoded string to a ucs-4 encoded string
ucs4string utf8ToUcs4(const string& source);

//! @brief Returns true if the string ends with the given value.
template <typename T>
bool stringEndsWith(const T& str, const T& value)
{
	bool result = false;

	// Empty string cannot end with a value and a string doesn't
	// end with an empty string. If value is greater than the string
	// then it also cannot end with it.
	size_t strSize = str.size();
	size_t valueSize = value.size();
	if (strSize > 0u && valueSize > 0u && strSize > valueSize)
	{
		result = true;
		for (size_t i = 0u; i < valueSize; ++i)
		{
			if (str[strSize - i - 1u] != value[valueSize - i - 1u])
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

//! @brief Returns true if the string starts with the given value.
template <typename T>
bool stringStartsWith(const T& str, const T& value)
{
	bool result = false;

	size_t strSize = str.size();
	size_t valueSize = value.size();
	if (strSize > 0 && valueSize > 0 && strSize > valueSize)
	{
		result = true;
		for (size_t i = 0; i < valueSize; ++i)
		{
			if (str[i] != value[i])
			{
				result = false;
				break;
			}
		}
	}

	return result;
}
} // namespace StringUtils
} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSTRINGUTILS_H
