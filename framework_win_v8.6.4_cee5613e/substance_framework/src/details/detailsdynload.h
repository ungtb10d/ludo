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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDYNLOAD_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDYNLOAD_H

#include <substance/framework/platform.h>
#include <substance/framework/typedefs.h>


//! @brief Platform independents headers
//! @{
#include <substance/engineid.h>
#include <substance/callbacks.h>
#include <substance/hardresources.h>
#include <substance/enums.h>
#include <substance/inputdesc.h>
#include <substance/outputdesc.h>
#include <substance/datadesc.h>
#include <substance/version.h>
//! @}


//! @brief Pre declaration of platform dependent structures
//! @{
struct SubstanceContext_;
struct SubstanceDevice_;
struct SubstanceTexture_;
//! @}


/* Begin of the EXTERNC block (if necessary) */
#ifdef __cplusplus
SUBSTANCE_EXTERNC {
#endif /* ifdef __cplusplus */

//! @brief Function TYPE Get the current substance engine version
typedef void (*SubstanceAirDetailsDynLoad_GetCurrentVersionImpl)(
	SubstanceVersion* version,
	unsigned int apiVersion);

//! @brief Function TYPE Substance engine context initialization from platform specific data
typedef unsigned int (*SubstanceAirDetailsDynLoad_ContextInitImpl)(
	SubstanceContext_** substanceContext,
	SubstanceDevice_* platformSpecificDevice,
	unsigned int apiVersion,
	SubstanceEngineIDEnum apiPlatform);

//! @brief Function TYPE Substance engine context release
typedef unsigned int (*SubstanceAirDetailsDynLoad_ContextRelease)(
	SubstanceContext_* substanceContext);

//! @brief Function TYPE Set Substance engine context callback
typedef unsigned int (*SubstanceAirDetailsDynLoad_ContextSetCallback)(
	SubstanceContext_* substanceContext,
	SubstanceCallbackEnum callbackType,
	void *callbackPtr);

//! @brief Function TYPE Memory de-allocation function ("free") used by Substance
typedef unsigned int (*SubstanceAirDetailsDynLoad_ContextMemoryFree)(
	SubstanceContext_* substanceContext,
	void* bufferPtr);

//! @brief Function TYPE texture release function used by Substance
typedef unsigned int (*SubstanceAirDetailsDynLoad_ContextTextureRelease)(
	SubstanceContext_* substanceContext,
	SubstanceTexture_* resultTexture);

//! @brief Function TYPE Restore initial platform dependent states
typedef unsigned int (*SubstanceAirDetailsDynLoad_ContextRestoreStates)(
	SubstanceContext_* substanceContext);

//! @brief Function TYPE Substance engine handle initialization from context and data
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleInit)(
	SubstanceHandle_** substanceHandle,
	SubstanceContext_* substanceContext,
	const unsigned char* substanceDataPtr,
	size_t substanceDataSize,
	const SubstanceHardResources* substanceRscInitial);

//! @brief Function TYPE Substance engine handle release
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleRelease)(
	SubstanceHandle_* substanceHandle);

//! @brief Function TYPE Push a new set of outputs in the render list.
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandlePushOutputs)(
	SubstanceHandle_* substanceHandle,
	unsigned int flags,
	const unsigned int selectList[],
	unsigned int selectListCount,
	size_t jobUserData);

//! @brief Function TYPE Flush the render list.
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleFlush)(
	SubstanceHandle_* substanceHandle);

//! @brief Function TYPE Computation process launch, process the render list
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleStart)(
	SubstanceHandle_* substanceHandle,
	unsigned int flags);

//! @brief Function TYPE Get the computation process state
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleGetState)(
	const SubstanceHandle_* substanceHandle,
	size_t *jobUserData,
	unsigned int* state);

//! @brief Function TYPE On the fly switch to another hardware resource dispatching
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleSwitchHard)(
	SubstanceHandle_* substanceHandle,
	SubstanceSyncOption syncMode,
	const SubstanceHardResources* substanceRscNew,
	const SubstanceHardResources* substanceRscNext,
	unsigned int duration);

//! @brief Function TYPE Cancel/Stop the computation process.
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleStop)(
	SubstanceHandle_* substanceHandle,
	SubstanceSyncOption syncMode);

//! @brief Function TYPE Grab output results
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleGetOutputs)(
	SubstanceHandle_* substanceHandle,
	unsigned int flags,
	unsigned int outIndexOrId,
	unsigned int outCount,
	void* results);

//! @brief Function TYPE Set an input )(render param.) value.
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandlePushSetInput)(
	SubstanceHandle_* substanceHandle,
	unsigned int flags,
	unsigned int inIndex,
	SubstanceIOType inType,
	void* value,
	size_t jobUserData);

//! @brief Function TYPE Get the handle main description
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleGetDesc)(
	const SubstanceHandle_* substanceHandle,
	SubstanceDataDesc* substanceInfo);

//! @brief Function TYPE Get an output )(texture result) description and current state
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleGetOutputDesc)(
	const SubstanceHandle_* substanceHandle,
	unsigned int outIndex,
	SubstanceOutputDesc* substanceOutDesc);

//! @brief Function TYPE Get an input )(render parameter) description
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleGetInputDesc)(
	const SubstanceHandle_* substanceHandle,
	unsigned int inIndex,
	SubstanceInputDesc* substanceInDesc);

//! @brief Function TYPE Synchronization function: set the handle synchronization label value
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleSyncSetLabel)(
	SubstanceHandle_* substanceHandle,
	size_t labelValue);

//! @brief Function TYPE Synchronization function: accessor to the synchronization label value
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleSyncGetLabel)(
	SubstanceHandle_* substanceHandle,
	size_t* labelValuePtr);

//! @brief Function TYPE Return the parent Substance context of a Substance handle
typedef SubstanceContext_* (*SubstanceAirDetailsDynLoad_HandleGetParentContext)(
	SubstanceHandle_* substanceHandle);

//! @brief Function TYPE Transfer internal cache from sourceHandle to destinationHandle
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleTransferCache)(
	SubstanceHandle_* destinationHandle,
	SubstanceHandle_* sourceHandle,
	const unsigned char* cacheMappingDataPtr);

//! @brief Function TYPE set user data
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleSetUserData)(
	SubstanceHandle_* substanceHandle,
	size_t userData);

//! @brief Function TYPE get user data
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleGetUserData)(
	SubstanceHandle_* substanceHandle,
	size_t* userData);

//! @brief Function TYPE set max node size limit
typedef unsigned int (*SubstanceAirDetailsDynLoad_HandleSetMaxNodeSize)(
	SubstanceHandle_* substanceHandle,
	unsigned int maxW,
	unsigned int maxH);

#ifdef __cplusplus
}
/* End of the EXTERNC block (if necessary) */
#endif /* ifdef __cplusplus */


#ifdef AIR_NO_DYNLOAD

	#include <substance/handle.h>

	//! @brief Substance engine function call
	//! @param name Function name without 'substance' prefix
	//! @param dynload Reference on DynLoad instance
	#define AIR_SBSCALL(name,dynload) substance##name

#else  // ifdef AIR_NO_DYNLOAD

	//! @brief Substance engine function call
	//! @param name Function name without 'substance' prefix
	//! @param dynload Reference on DynLoad instance
	#define AIR_SBSCALL(name,dynload) \
		(*(SubstanceAirDetailsDynLoad_##name)(dynload)[ \
			SubstanceAir::Details::DynLoad::Func_##name])

#endif  // ifdef AIR_NO_DYNLOAD


namespace SubstanceAir
{
namespace Details
{

//! @brief Substance Engine dynamic loader wrapper
class DynLoad
{
public:
	//! @brief Dynamic functions enum
	enum Functions
	{
		Func_GetCurrentVersionImpl ,
		Func_ContextInitImpl       ,
		Func_ContextRelease        ,
		Func_ContextSetCallback    ,
		Func_ContextMemoryFree     ,
		Func_ContextTextureRelease ,
		Func_ContextRestoreStates  ,
		Func_HandleInit            ,
		Func_HandleRelease         ,
		Func_HandlePushOutputs     ,
		Func_HandleFlush           ,
		Func_HandleStart           ,
		Func_HandleGetState        ,
		Func_HandleSwitchHard      ,
		Func_HandleStop            ,
		Func_HandleGetOutputs      ,
		Func_HandlePushSetInput    ,
		Func_HandleGetDesc         ,
		Func_HandleGetOutputDesc   ,
		Func_HandleGetInputDesc    ,
		Func_HandleSyncSetLabel    ,
		Func_HandleSyncGetLabel    ,
		Func_HandleGetParentContext,
		Func_HandleTransferCache   ,
		Func_HandleSetUserData     ,
		Func_HandleGetUserData     ,
		Func_HandleSetMaxNodeSize  ,
		Func_COUNT
	};

	//! @brief Function names
	static const char* mFuncNames[Func_COUNT];


	//! @brief Constructor use statically linked engine
	DynLoad();

	//! @brief Switch engine dynamic library
	//! @param module Dynamic library to use, result of dlopen/LoadLibrary call
	//!		use nullptr to revert to default impl.
	//! @return Return true if dynamic library loaded and compatible
	bool switchLibrary(void* module);

	//! @brief Accessor on currently loaded engine version
	//! Zero filled if no engine loaded
	const SubstanceVersion& getVersion() const { return mVersion; }

	//! @brief Accessor on function pointer from enum
	void* operator[](Functions func) const;

protected:
	//! @brief Current pointers
	void* mFuncPtrs[Func_COUNT];

	//! @brief Currently loaded engine version
	SubstanceVersion mVersion;


	//! @brief Revert to default impl.
	bool setDefaults();

};  // class DynLoad


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDYNLOAD_H
