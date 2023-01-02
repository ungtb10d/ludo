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
	Used by Substance Cooker and Linker to embed and extract extra data into 
	assemblies.
*/

#ifndef _SUBSTANCE_EXTRADATA_H
#define _SUBSTANCE_EXTRADATA_H


/** Basic type defines */
#include <stddef.h>


/** @brief Substance extra data UID enumeration */
typedef enum
{
	Substance_ExtraData_INVALID    = 0x0,       /**< Unknown, do not use */
	
	/* etc */
	
	Substance_ExtraData_User       = 0x8000,    /**< User defined extra data */
	
	Substance_ExtraData_FORCEDWORD = 0xFFFFFFFF /**< Force DWORD enumeration */
	
} SubstanceExtraDataEnum;


#endif /* ifndef _SUBSTANCE_EXTRADATA_H */
