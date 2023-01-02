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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSINPUTSTATE_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSINPUTSTATE_H

#include <substance/framework/inputimage.h>

#include <substance/inputdesc.h>




namespace SubstanceAir
{

struct InputDescBase;
class InputInstanceBase;

namespace Details
{


//! @brief Dynamic array of pointers on InputImage, used w/ InputState
typedef vector<InputImage::SPtr> InputImagePtrs;

//! @brief Shared ptr on string, used w/ InputState
typedef shared_ptr<ucs4string> InputStringPtr;

//! @brief Dynamic array of pointers on string, used w/ InputState
typedef vector<InputStringPtr> InputStringPtrs;


//! @brief Input Instance State
//! Contains a input value
struct InputState
{
	//! @brief Input state flags
	enum Flag
	{
		Flag_Constified = 0x80000000u,   //!< This input is constified
		Flag_Cache =      0x40000000u,   //!< This input should be cached
		Flag_CacheOnly =  0x10000000u,   //!< Cache only, value not modified

		Flag_MASKTYPE   = 0xFFu,         //!< Type selection mask
		Flag_FORCEDWORD = 0xFFFFFFFFu    //!< Internal Use Only
	};

	//! @brief SubstanceIOType and suppl. flags (combination of Flag)
	UInt flags;

	union
	{
		//! @brief Numeric values (float/int), for numeric inputs
		UInt numeric[4];

		//! @brief Index of shared pointer on InputImage (in external array)
		//! Image input only
		size_t imagePtrIndex;

		// @brief Index of shared pointer on String (in external array)
		//! String input only
		size_t stringPtrIndex;
	} value;

	//! @brief Invalid image pointer index
	static const size_t invalidIndex;


	//! @brief Accessor on input type
	SubstanceIOType getType() const { return (SubstanceIOType)(flags&Flag_MASKTYPE); }

	//! @brief Return if image type
	bool isImage() const { return Substance_IOType_Image==getType(); }

	//! @brief Return if string type
	bool isString() const { return Substance_IOType_String==getType(); }

	//! @brief Accessor on constified state
	bool isConstified() const { return (flags&Flag_Constified)!=0; }

	//! @brief Accessor on cache flag
	bool isCacheEnabled() const { return (flags&Flag_Cache)!=0; }

	//! @brief Accessor on cache only flag
	bool isCacheOnly() const { return (flags&Flag_CacheOnly)!=0; }

	//! @brief Is image/string pointer index valid
	bool isIndexValid() const { return invalidIndex!=value.imagePtrIndex; }


	//! @brief Set input state value from instance
	//! @param[out] dst Destination input state
	//! @param inst Input instance to get value
	//! @param[in,out] imagePtrs Array of pointers on InputImage indexed by
	//!		imagePtrIndex
	//! @param[in,out] stringsPtrs Array of pointers on InputImage indexed by
	//!		stringPtrIndex
	void fillValue(
		const InputInstanceBase* inst,
		InputImagePtrs &imagePtrs,
		InputStringPtrs &stringPtrs);

	//! @brief Initialize value from description default value
	//! @param desc Input description to get default value
	void initValue(const InputDescBase& desc);

};  // struct InputState


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSINPUTSTATE_H
