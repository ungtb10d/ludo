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
	Defines the SubstanceDataDesc structure. Filled using the substanceHandleGetDesc
	function declared in handle.h.
	Contains information about the data to render: number of outputs, inputs,
	etc.
*/

#ifndef _SUBSTANCE_DATADESC_H
#define _SUBSTANCE_DATADESC_H


/** @brief Substance cooked data description structure definition

	Filled using the substanceHandleGetDesc function declared in handle.h. */
typedef struct SubstanceDataDesc_
{
	/** @brief Number of outputs (texture results) */
	unsigned int outputsCount;
	
	/** @brief Number of inputs (render parameters) */
	unsigned int inputsCount;
	
	/** @brief Format version the data was cooked for */
	unsigned short formatVersion;	
	
	/** @brief Identifier of the platform the data was cooked for */
	unsigned char platformId;

} SubstanceDataDesc;

#endif /* ifndef _SUBSTANCE_DATADESC_H */
