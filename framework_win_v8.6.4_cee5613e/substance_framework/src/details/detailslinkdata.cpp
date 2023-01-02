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

#include "detailslinkdata.h"
#include "detailslinkcontext.h"
#include "detailsgraphbinary.h"
#include "detailsutils.h"

#include <assert.h>


//! @brief Destructor
SubstanceAir::Details::LinkData::~LinkData()
{
}


SubstanceAir::Details::LinkDataAssembly::LinkDataAssembly(
		const char* ptr,
		size_t size) :
	mAssembly(ptr,size)
{
}


SubstanceAir::Details::LinkDataAssembly::LinkDataAssembly(
		string &str)
{
	mAssembly.swap(str);
}

	
//! @brief Push data to link
//! @param cxt Used to push link data
bool SubstanceAir::Details::LinkDataAssembly::push(LinkContext& cxt) const
{
	int err;
	
	// Push assembly
	{
		SubstanceLinkerPush srcpush;
		srcpush.uid = cxt.stateUid;
		srcpush.srcType = Substance_Linker_Src_Buffer;
		srcpush.src.buffer.ptr = mAssembly.data();
		srcpush.src.buffer.size = mAssembly.size();

		err = substanceLinkerHandlePushAssemblyEx(
			cxt.handle,
			&srcpush);
	}

	if (err)
	{
		// TODO2 error pushing assembly to linker
		assert(0);
		return false;
	}

	return true;
}


//! @brief Construct from pre and post Graphs, and connection options.
//! @param preGraph Source pre graph.
//! @param postGraph Source post graph.
//! @param connOptions Connections options.
SubstanceAir::Details::LinkDataStacking::LinkDataStacking(
		const GraphDesc& preGraph,
		const GraphDesc& postGraph,
		const ConnectionsOptions& connOptions) :
	mOptions(connOptions),
	mPreLinkData(preGraph.mParent->getLinkData()),
	mPostLinkData(postGraph.mParent->getLinkData())
{
}


//! @brief Push data to link
//! @param cxt Used to push link data
bool SubstanceAir::Details::LinkDataStacking::push(LinkContext& cxt) const
{
	// Push this stack level in context, popped at deletion (RAII)
	LinkContext::StackLevel level(cxt,*this);

	// Push pre to link
	if (mPreLinkData.get()==nullptr || !mPreLinkData->push(cxt))
	{
		// TODO2 Error: cannot push pre
		assert(0);
		return false;
	}

	// Fix fuse pre UIDs
	ConnectionsOptions::Connections fusefixed(mFuseInputs);
	for (auto& fuse : fusefixed)
	{
		LinkContext::TrResult trres = cxt.translateUid(false,fuse.first,true);
		assert(trres.second);
		if (trres.second)
		{
			fuse.first = trres.first;
		}
	}
	
	// Switch to post
	level.beginPost();

	// Push post to link
	if (mPostLinkData.get()==nullptr || !mPostLinkData->push(cxt))
	{
		// TODO2 Error: cannot push post
		assert(0);
		return false;
	}
	
	// Push connections
	for (const auto& c : mOptions.mConnections)
	{
		int err = substanceLinkerConnectOutputToInput(
			cxt.handle,
			LinkContext::translate(level.trPostInputs,c.second).first,
			LinkContext::translate(level.trPreOutputs,c.first).first);
			
		if (err)
		{
			// TODO2 Warning: connect failed
			assert(0);
		}
	}
	
	// Fuse all $ prefixed inputs
	for (const auto& fuse : fusefixed)
	{
		substanceLinkerFuseInputs(
			cxt.handle,
			fuse.first,
			LinkContext::translate(level.trPostInputs,fuse.second).first);
	}
	
	return true;
}

