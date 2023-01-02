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
	Defines the SubstanceLinkerVersion structure. Filled using the
	substanceLinkerGetCurrentVersion function.
*/

#ifndef _SUBSTANCE_LINKER_VERSION_H
#define _SUBSTANCE_LINKER_VERSION_H


/** Platform dependent definitions */
#include "platformdep.h"


/** @brief Version of the header of the Substance Linker API */
#define SUBSTANCE_LINKER_API_VERSION 0x00010003


/** @brief Substance linker current version description

	Filled using the substanceLinkerGetCurrentVersion function declared
	just below. */
typedef struct
{
	/** @brief Current linker version: major */
	unsigned int versionMajor;

	/** @brief Current linker version: minor */
	unsigned int versionMinor;

	/** @brief Current linker version: patch */
	unsigned int versionPatch;

	/** @brief Current linker stage (Release, alpha, beta, RC, etc.) */
	const char* currentStage;

	/** @brief Substance linker build date (mm dd yyyy) */
	const char* buildDate;

	/** @brief Substance linker sources revision */
	unsigned int sourcesRevision;

} SubstanceLinkerVersion;


/** @brief Substance linker supported engine description

	Used by substanceLinkerGetEnginesList function declared
	just below. */
typedef struct
{
	/** @brief Engine ID
		@warning Equal to zero to signify the end of the engines list.

		See SubstanceEngineIDEnum in ../engineid.h */
	unsigned int id;

	/** @brief Engine short name (lowercase) */
	const char* shortName;

	/** @brief Engine description */
	const char* description;

	/** @brief Version of binary data format written by current linker
		No backward or forward compatibility: engine version must match.
		This version number is engine platform specific. */
	unsigned int versionData;

} SubstanceLinkerEngineDesc;


/** @brief Get the current substance linker version
	@param[out] version Pointer to the version struct to fill. */
SUBSTANCE_EXPORT void substanceLinkerGetCurrentVersion(
	SubstanceLinkerVersion* version);

/** @brief Get the list of engines supported by current substance linker version
	@return An array of SubstanceLinkerEngineDesc elements terminated with
		a zero filled structure. */
SUBSTANCE_EXPORT const SubstanceLinkerEngineDesc* substanceLinkerGetEnginesList();


#endif /* ifndef _SUBSTANCE_LINKER_VERSION_H */
