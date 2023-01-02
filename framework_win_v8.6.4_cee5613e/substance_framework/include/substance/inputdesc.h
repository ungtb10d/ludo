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

/*
	Defines the SubstanceInputDesc structure. Filled via the
	substanceHandleGetInputDesc function declared in handle.h.
*/

#ifndef _SUBSTANCE_INPUTDESC_H
#define _SUBSTANCE_INPUTDESC_H

/** Input/Output type enumeration definition include */
#include "iotype.h"


/** @brief Substance font input description structure definition */
typedef struct SubstanceInputFont_
{
	/** @brief TrueType or OpenType font data buffer */
	const void* data;

	/** @brief Font data buffer size */
	unsigned int size;

	/** @brief Font index in font data
	    @note Useful if several fonts are defined in the same file */
	unsigned int index;

} SubstanceInputFont;


/** @brief Substance input description structure definition

	Filled using the substanceHandleGetInputDesc function declared in handle.h. */
typedef struct SubstanceInputDesc_
{
	/** @brief Input unique identifier */
	unsigned int inputId;

	/** @brief Input type */
	SubstanceIOType inputType;

	/** @brief Current value of the Input. Depending on the type. */
	union
	{
		/** @brief Pointer on the current value of FloatX type inputs
		    @note Valid only if inputType is Float Float2 Float3 or Float4 */
		const float *typeFloatX;

		/** @brief Pointer on the current value of Integer type input
		    @note Valid only if inputType is Integer or Integer2,3,4 */
		const int *typeIntegerX;

		/** @brief Pointer on the current U32b string set to String type input
		    @note Valid only if inputType is Substance_IOType_String */
		const unsigned int **typeString;

	} value;

} SubstanceInputDesc;

#endif /* ifndef _SUBSTANCE_INPUTDESC_H */
