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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKDATA_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKDATA_H

#include <substance/framework/stacking.h>


#include <utility>


namespace SubstanceAir
{
namespace Details
{

struct LinkContext;


//! @brief Link data abstract base class
class LinkData
{
public:
	//! @brief Default constructor
	LinkData() {}

	//! @brief Virtual destructor
	virtual ~LinkData();
		
	//! @brief Push data to link
	//! @param cxt Used to push link data
	virtual bool push(LinkContext& cxt) const = 0;

	//! @brief Accessor on assembly data, nullptr if not assembly
	virtual const string* getAssembly() const { return nullptr; }

private:
	LinkData(const LinkData&);
	const LinkData& operator=(const LinkData&);	
};  // class LinkData


//! @brief Link data simple package from assembly class
class LinkDataAssembly : public LinkData
{
public:
	//! @brief Constructor from assembly data
	//! @param ptr Pointer on assembly data
	//! @param size Size of assembly data in bytes
	LinkDataAssembly(const char* ptr,size_t size);

	//! @brief Constructor from assembly data string: use copy-free swap
	//! @param str Assembly data string: reseted after by this call (swap)
	LinkDataAssembly(string &str);
		
	//! @brief Push data to link
	//! @param cxt Used to push link data
	bool push(LinkContext& cxt) const;

	//! @brief Accessor on assembly data overload
	const string* getAssembly() const { return &mAssembly; }
	
protected:
	//! @brief Assembly data
	string mAssembly;
	
};  // class LinkDataAssembly


//! @brief Link data simple package from stacking class
class LinkDataStacking : public LinkData
{
public:
	//! @brief UID translation container (initial,translated pair)
	typedef vector<std::pair<UInt,UInt> > UidTranslates;

	//! @brief Construct from pre and post Graphs, and connection options.
	//! @param preGraph Source pre graph.
	//! @param postGraph Source post graph.
	//! @param connOptions Connections options.
	LinkDataStacking(
		const GraphDesc& preGraph,
		const GraphDesc& postGraph,
		const ConnectionsOptions& connOptions);
		
	//! @brief Push data to link
	//! @param cxt Used to push link data
	bool push(LinkContext& cxt) const;
	
	//! @brief Connections options.
	ConnectionsOptions mOptions;
	
	//! @brief Pair of pre,post UIDs to fuse (initial UIDs)
	//! Post UID sorted (second)
	ConnectionsOptions::Connections mFuseInputs;  
	
	//! @brief Disabled UIDs (initial UIDs)
	//! UID sorted
	Uids mDisabledOutputs;
	
	//! @brief Post graph Inputs UID translation (initial->translated)
	//! initial UID sorted
	UidTranslates mTrPostInputs;
	
	//! @brief Pre graph Outputs UID translation (initial->translated)
	//! initial UID sorted
	UidTranslates mTrPreOutputs;
	
protected:
	//! @brief Link data of Source pre graph.
	shared_ptr<LinkData> mPreLinkData;
	
	//! @brief Link data of Source post graph.
	shared_ptr<LinkData> mPostLinkData;
	
};  // class LinkDataStacking


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKDATA_H
