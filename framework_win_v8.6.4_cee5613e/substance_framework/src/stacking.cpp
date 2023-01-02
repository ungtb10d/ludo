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

#include "details/detailslinkdata.h"
#include "details/detailsrendererimpl.h"
#include "details/detailsutils.h"

#include <substance/framework/stacking.h>

#include <substance/linker/linker.h>

#include <set>


#include <utility>
#include <algorithm>
#include <iterator>

#include <assert.h>


SubstanceAir::PackageStackDesc::PackageStackDesc(
	const GraphDesc& preGraph,
	const GraphDesc& postGraph,
	const ConnectionsOptions& connOptions)
{
	if (preGraph.mParent==nullptr || postGraph.mParent==nullptr)
	{
		// TODO2 Error, Invalid graphs
		assert(0);
		return;
	}

	// Create link data structure that contains result of stacking process
	shared_ptr<Details::LinkDataStacking> lnkdata = make_shared<Details::LinkDataStacking>(preGraph, postGraph, connOptions);

	mLinkData = lnkdata;
	ConnectionsOptions& options = lnkdata->mOptions;

	// Generate mUid
	{
		static UInt uidOffset = 0;
		mUid = (uidOffset++)^preGraph.mParent->getUid()^postGraph.mParent->getUid();
	}

	// Generate description
	mGraphs.resize(1);
	GraphDesc& graph = mGraphs.back();
	graph.mParent = this;
	graph.mPackageUrl = preGraph.mPackageUrl;
	graph.mLabel = preGraph.mLabel + "/" + postGraph.mLabel;

	// Copy pre inputs, detect similar inputs
	typedef set<UInt> UidsSet;
	UidsSet inputUids,outputUids;                       // All result I/O uids
	typedef map<string,UInt> DictIdentifier;
	DictIdentifier preDefaultInputs;                    // $ prefixed inputs
	for (const auto& input : preGraph.mInputs)
	{
		if (!input->mIdentifier.empty() && input->mIdentifier[0]=='$')
		{
			preDefaultInputs.insert(std::make_pair(input->mIdentifier,input->mUid));
		}
		InputDescBase *newinp = input->clone();
		graph.mInputs.push_back(newinp);
		newinp->mAlteredOutputUids.clear();
		inputUids.insert(input->mUid);
	}

	// Copy Post outputs
	for (const auto& output : postGraph.mOutputs)
	{
		graph.mOutputs.push_back(output);
		outputUids.insert(output.mUid);
	}

	// Connections
	if (options.mConnections.empty())
	{
		// Generate auto connection if connection list is empty
		if (!options.connectAutomatically(preGraph,postGraph))
		{
//			Warning: no connections found
		}
	}
	else
	{
		// Fix existing connections
		options.fixConnections(preGraph,postGraph);
	}

	// Collect all disabled output and inputs
	// List replacements for future fix of mAlteringInputUids and
	// mAlteringOutputUids
	typedef map<UInt,UidsSet> Replacements;
	Replacements replaceIn;
	UidsSet connectedOut;
	for (const auto& pairinout : options.mConnections)
	{
		connectedOut.insert(pairinout.first);
		GraphDesc::Outputs::const_iterator oite = preGraph.mOutputs.begin();
		std::advance(oite,preGraph.findOutput(pairinout.first));
		assert(oite!=preGraph.mOutputs.end());
		replaceIn[pairinout.second].insert(
			oite->mAlteringInputUids.begin(),
			oite->mAlteringInputUids.end());
	}

	// Add non connected post inputs, fuse if duplicate default input
	GraphDesc::Inputs newInputs;
	for (const auto& input : postGraph.mInputs)
	{
		if (replaceIn.find(input->mUid)==replaceIn.end())
		{
			// Not connected input
			DictIdentifier::const_iterator dfltite =
				preDefaultInputs.find(input->mIdentifier);
			if (dfltite!=preDefaultInputs.end())
			{
				// Fused
				lnkdata->mFuseInputs.push_back(std::make_pair(
					dfltite->second,input->mUid));        // Fuse pre into post
				replaceIn[input->mUid].insert(dfltite->second);
			}
			else
			{
				// Inserted
				InputDescBase* newinp = input->clone();
				if (!inputUids.insert(newinp->mUid).second)
				{
					newinp->mUid = (*inputUids.rbegin())+1;  // Generate new UID
					inputUids.insert(newinp->mUid);
					lnkdata->mTrPostInputs.push_back(
						std::make_pair(input->mUid,newinp->mUid));
					replaceIn[input->mUid].insert(newinp->mUid);
				}
				newinp->mAlteredOutputUids.clear();
				newInputs.push_back(newinp);
			}
		}
	}

	graph.mInputs.insert(graph.mInputs.end(),newInputs.begin(),newInputs.end());

	// Sort input indices by UID
	graph.commitInputs();
	std::sort(lnkdata->mTrPostInputs.begin(),lnkdata->mTrPostInputs.end());

	// Fix mAlteringInputUids of Post (already pushed in result)
	for (auto& output : graph.mOutputs)
	{
		Uids uids;
		for (const auto& uidin : output.mAlteringInputUids)
		{
			Replacements::const_iterator rite = replaceIn.find(uidin);
			if (rite!=replaceIn.end())
			{
				uids.insert(uids.end(),rite->second.begin(),rite->second.end());
			}
			else
			{
				uids.push_back(uidin);
			}
		}

		std::sort(uids.begin(),uids.end());
		uids.erase(std::unique(uids.begin(),uids.end()),uids.end());
		output.mAlteringInputUids = uids;
	}

	// Add non connected pre outputs, or mark as disabled
	for (const auto& output : preGraph.mOutputs)
	{
		if (connectedOut.find(output.mUid)==connectedOut.end())
		{
			if (options.mNonConnected==NonConnected_Remove)
			{
				lnkdata->mDisabledOutputs.push_back(output.mUid);
			}
			else
			{
				// Insert in result graph
				graph.mOutputs.push_back(output);
				OutputDesc& newout = graph.mOutputs.back();
				if (!outputUids.insert(newout.mUid).second)
				{
					newout.mUid = (*outputUids.rbegin())+1;  // Generate new UID
					outputUids.insert(newout.mUid);
					lnkdata->mTrPreOutputs.push_back(
						std::make_pair(output.mUid,newout.mUid));
				}
			}
		}
	}

	// Sort outputs indices by UID
	graph.commitOutputs();
	std::sort(lnkdata->mTrPreOutputs.begin(),lnkdata->mTrPreOutputs.end());

	// Fill mAlteredOutputUids of all inputs
	for (const auto& output : graph.mOutputs)
	{
		for (const auto& uidin : output.mAlteringInputUids)
		{
			const size_t inpindex = graph.findInput(uidin);
			assert(inpindex<graph.mInputs.size());

			if (inpindex<graph.mInputs.size())
			{
				const_cast<InputDescBase*>(graph.mInputs[inpindex])->
					mAlteredOutputUids.push_back(output.mUid);
			}
		}
	}

	// Sort mAlteredOutputUids
	for (const auto& input : graph.mInputs)
	{
		Uids &uids = const_cast<InputDescBase*>(input)->mAlteredOutputUids;
		std::sort(uids.begin(),uids.end());
	}
}


SubstanceAir::PackageStackDesc::~PackageStackDesc()
{
}


bool SubstanceAir::ConnectionsOptions::connectAutomatically(
	const GraphDesc& preGraph,
	const GraphDesc& postGraph)
{
	mConnections.clear();

	// Looking at post inputs
	typedef map<string,UInt> DictIdentifier;
	DictIdentifier postIdentifiers,postLabels;
	for (const auto& input : postGraph.mInputs)
	{
		if (input->isImage())
		{
			string str(input->mLabel);
			Details::Utils::toLower(str);
			postLabels.insert(std::make_pair(str,input->mUid));
			str = input->mIdentifier;
			Details::Utils::toLower(str);
			postIdentifiers.insert(std::make_pair(str,input->mUid));
		}
	}
	postIdentifiers.insert(postLabels.begin(),postLabels.end());

	// Looking at pre outputs
	DictIdentifier preIdentifiers,preLabels,preUsage;
	for (const auto& output : preGraph.mOutputs)
	{
		string str(output.mIdentifier);
		Details::Utils::toLower(str);
		preIdentifiers.insert(std::make_pair(str,output.mUid));
		str = output.mLabel;
		Details::Utils::toLower(str);
		preLabels.insert(std::make_pair(str,output.mUid));

		for (const auto& channel : output.mChannels)
		{
			if ((size_t)channel<(size_t)Channel_INTERNAL_COUNT)
			{
				str = getChannelNames()[(size_t)channel];
				Details::Utils::toLower(str);
				preUsage.insert(std::make_pair(str,output.mUid));
			}
		}
	}
	preIdentifiers.insert(preLabels.begin(),preLabels.end());
	preIdentifiers.insert(preUsage.begin(),preUsage.end());

	// Introduce channel flexibility
	{
		DictIdentifier::const_iterator oite = postIdentifiers.find("specular");
		oite = oite!=postIdentifiers.end() ? oite : postIdentifiers.find("specularcolor");
		oite = oite!=postIdentifiers.end() ? oite : postIdentifiers.find("specularlevel");
		oite = oite!=postIdentifiers.end() ? oite : postIdentifiers.find("glossiness");
		oite = oite!=postIdentifiers.end() ? oite : postIdentifiers.find("roughness");
		if (oite!=postIdentifiers.end())
		{
			// Skipped if already present
			postIdentifiers.insert(std::make_pair("specularcolor",oite->second));
			postIdentifiers.insert(std::make_pair("specularlevel",oite->second));
			postIdentifiers.insert(std::make_pair("glossiness",oite->second));
			postIdentifiers.insert(std::make_pair("roughness",oite->second));
			postIdentifiers.insert(std::make_pair("specular",oite->second));
		}
	}

	{
		DictIdentifier::const_iterator oite = postIdentifiers.find("bump");
		oite = oite!=postIdentifiers.end() ? oite : postIdentifiers.find("height");
		oite = oite!=postIdentifiers.end() ? oite : postIdentifiers.find("displacement");
		if (oite!=postIdentifiers.end())
		{
			// Skipped if already present
			postIdentifiers.insert(std::make_pair("bump",oite->second));
			postIdentifiers.insert(std::make_pair("height",oite->second));
			postIdentifiers.insert(std::make_pair("displacement",oite->second));
		}
	}

	// Try to match inputs to outputs
	set<UInt> connectedInputs;
	for (const auto& input : postIdentifiers)
	{
		if (connectedInputs.find(input.second)==connectedInputs.end())
		{
			const DictIdentifier::const_iterator oite =
				preIdentifiers.find(input.first);
			if (oite!=preIdentifiers.end())
			{
				// Match!
				mConnections.push_back(std::make_pair(
					oite->second,
					input.second));
				connectedInputs.insert(input.second);
			}
		}
	}

	if (mConnections.empty() && postIdentifiers.size()==1)
	{
		if (preIdentifiers.size()==1)
		{
			// 1 to 1
			mConnections.push_back(std::make_pair(
				preIdentifiers.begin()->second,
				postIdentifiers.begin()->second));
		}
		else if (preIdentifiers.find("diffuse")!=preIdentifiers.end())
		{
			// diffuse to 1
			mConnections.push_back(std::make_pair(
				preIdentifiers.find("diffuse")->second,
				postIdentifiers.begin()->second));
		}
	}

	return !mConnections.empty();
}


void SubstanceAir::ConnectionsOptions::fixConnections(
	const GraphDesc& preGraph,
	const GraphDesc& postGraph)
{
	Connections connections;

	// Check for UIDs validity
	for (const auto& pairinout : mConnections)
	{
		if (preGraph.findOutput(pairinout.first)<preGraph.mOutputs.size() &&
			postGraph.findInput(pairinout.second)<postGraph.mInputs.size())
		{
			connections.push_back(pairinout);
		}
		else
		{
//				Warning: not valid UIDs
		}
	}

	mConnections = connections;
}


