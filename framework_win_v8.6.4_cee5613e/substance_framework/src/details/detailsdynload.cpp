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

#include "detailsdynload.h"

#ifndef AIR_NO_DYNLOAD

#if defined(__WIN32__) || defined(_WIN32)

	// WIN32 version

	#include <Windows.h>

	//! @brief Retrieves the address of an exported function dynamic library
	//! @param module Module handle
	//! @param fnname The function name to retrieve
	//! @return Return function adress or nullptr if not found
	#define substanceAirGetProcAdress(module,name) \
		GetProcAddress((HMODULE)(module),name)

#else  //if defined(__WIN32__) || defined(_WIN32)

	// UNIX-like version

	#include <dlfcn.h>

	//! @brief Retrieves the address of an exported function dynamic library
	//! @param module Module handle
	//! @param fnname The function name to retrieve
	//! @return Return function adress or nullptr if not found
	#define substanceAirGetProcAdress(module,name) \
		dlsym(module,name)

#endif  //if defined(__WIN32__) || defined(_WIN32)

#include <assert.h>
#include <string.h>


//! @brief Function names
const char* SubstanceAir::Details::DynLoad::mFuncNames[Func_COUNT] = {
	"substanceGetCurrentVersionImpl",
	"substanceContextInitImpl",
	"substanceContextRelease",
	"substanceContextSetCallback",
	"substanceContextMemoryFree",
	"substanceContextTextureRelease",
	"substanceContextRestoreStates",
	"substanceHandleInit",
	"substanceHandleRelease",
	"substanceHandlePushOutputs",
	"substanceHandleFlush",
	"substanceHandleStart",
	"substanceHandleGetState",
	"substanceHandleSwitchHard",
	"substanceHandleStop",
	"substanceHandleGetOutputs",
	"substanceHandlePushSetInput",
	"substanceHandleGetDesc",
	"substanceHandleGetOutputDesc",
	"substanceHandleGetInputDesc",
	"substanceHandleSyncSetLabel",
	"substanceHandleSyncGetLabel",
	"substanceHandleGetParentContext",
	"substanceHandleTransferCache",
	"substanceHandleSetUserData",
	"substanceHandleGetUserData",
	"substanceHandleSetMaxNodeSize"
};


//! @brief Constructor use statically linked engine
SubstanceAir::Details::DynLoad::DynLoad()
{
	if (!setDefaults())
	{
		memset(&mVersion,0,sizeof(mVersion));
	}
}


//! @brief Switch engine dynamic library
//! @param module Dynamic library to use, result of dlopen/LoadLibrary call
//!		use nullptr to revert to default impl.
//! @return Return true if dynamic library loaded and compatible
bool SubstanceAir::Details::DynLoad::switchLibrary(void* module)
{
	if (module==nullptr)
	{
		// Revert to statically linked
		return setDefaults();
	}

	// Get function addresses
	void* funcPtrs[Func_COUNT];
	for (size_t k=0;k<Func_COUNT;++k)
	{
		funcPtrs[k] = substanceAirGetProcAdress(module,mFuncNames[k]);
		if (funcPtrs[k]==nullptr)
		{
			return false;
		}
	}

	// Check API and platform version
	SubstanceVersion verengine;
	(*(SubstanceAirDetailsDynLoad_GetCurrentVersionImpl)funcPtrs[Func_GetCurrentVersionImpl])(
		&verengine,
		SUBSTANCE_API_VERSION);

	// Require at least 1.9 engine API:
	//  - [S|G]etUserData coherent
	//  - Texture input default parameters accepted
	//  - Set max node size
	if (verengine.versionApi<0x00010009)
	{
		return false;
	}

	// Copy new function pointers
	memcpy(mFuncPtrs,funcPtrs,sizeof(mFuncPtrs));

	// Copy loaded engine version
	mVersion = verengine;

	return true;
}


//! @brief Accessor on function pointer from enum
void* SubstanceAir::Details::DynLoad::operator[](Functions func) const
{
	assert(Func_COUNT>(size_t)func);
	return mFuncPtrs[func];
}

#ifndef AIR_NO_DEFAULT_ENGINE

#include <substance/handle.h>

//! @brief Revert to default impl.
bool SubstanceAir::Details::DynLoad::setDefaults()
{
	mFuncPtrs[0]  = (void*)substanceGetCurrentVersionImpl;
	mFuncPtrs[1]  = (void*)substanceContextInitImpl;
	mFuncPtrs[2]  = (void*)substanceContextRelease;
	mFuncPtrs[3]  = (void*)substanceContextSetCallback;
	mFuncPtrs[4]  = (void*)substanceContextMemoryFree;
	mFuncPtrs[5]  = (void*)substanceContextTextureRelease;
	mFuncPtrs[6]  = (void*)substanceContextRestoreStates;
	mFuncPtrs[7]  = (void*)substanceHandleInit;
	mFuncPtrs[8]  = (void*)substanceHandleRelease;
	mFuncPtrs[9]  = (void*)substanceHandlePushOutputs;
	mFuncPtrs[10] = (void*)substanceHandleFlush;
	mFuncPtrs[11] = (void*)substanceHandleStart;
	mFuncPtrs[12] = (void*)substanceHandleGetState;
	mFuncPtrs[13] = (void*)substanceHandleSwitchHard;
	mFuncPtrs[14] = (void*)substanceHandleStop;
	mFuncPtrs[15] = (void*)substanceHandleGetOutputs;
	mFuncPtrs[16] = (void*)substanceHandlePushSetInput;
	mFuncPtrs[17] = (void*)substanceHandleGetDesc;
	mFuncPtrs[18] = (void*)substanceHandleGetOutputDesc;
	mFuncPtrs[19] = (void*)substanceHandleGetInputDesc;
	mFuncPtrs[20] = (void*)substanceHandleSyncSetLabel;
	mFuncPtrs[21] = (void*)substanceHandleSyncGetLabel;
	mFuncPtrs[22] = (void*)substanceHandleGetParentContext;
	mFuncPtrs[23] = (void*)substanceHandleTransferCache;
	mFuncPtrs[24] = (void*)substanceHandleSetUserData;
	mFuncPtrs[25] = (void*)substanceHandleGetUserData;
	mFuncPtrs[26] = (void*)substanceHandleSetMaxNodeSize;

	substanceGetCurrentVersionImpl(&mVersion,SUBSTANCE_API_VERSION);

	return true;
}

#else //ifndef AIR_NO_DEFAULT_ENGINE

//! @brief No default impl.
bool SubstanceAir::Details::DynLoad::setDefaults()
{
	memset(mFuncPtrs,0,sizeof(mFuncPtrs));

	return false;
}

#endif //ifndef AIR_NO_DEFAULT_ENGINE


#endif  // ifndef AIR_NO_DYNLOAD

