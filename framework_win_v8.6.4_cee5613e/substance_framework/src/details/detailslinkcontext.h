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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKCONTEXT_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKCONTEXT_H

#include <substance/framework/typedefs.h>

#include <substance/linker/handle.h>


#include <utility>


namespace SubstanceAir
{
namespace Details
{

struct GraphBinary;
class LinkDataStacking;


//! @brief Substance Link context structure used to push LinkData
struct LinkContext
{
	//! @brief UID translation container (initial,translated pair)
	typedef vector<std::pair<UInt,UInt> > UidTranslates;
	
	//! @brief One stack level definition
	struct StackLevel
	{
		//! @brief Correponding stacking link data
		const LinkDataStacking &linkData;
		
		//! @brief Inputs UID translations disabled at this level
		//! Initialy filled w/ connected+fused
		UidTranslates trPostInputs;

		//! @brief Outputs UID translations disabled at this level
		//! Initialy filled w/ connected+disabled
		UidTranslates trPreOutputs;
		
		
		//! @brief Constructor from context and stacking link data
		StackLevel(LinkContext &cxt,const LinkDataStacking &data);
		
		//! @brief Destructor, pop this level from stack
		~StackLevel();
	
		//! @brief Notify that pre graph is pushed, about to push post graph
		void beginPost();
		
		//! @brief Accessor level is actually pushing post
		bool isPost() const { return mIsPost; }
		
	protected:
		LinkContext &mLinkContext;   //!< Reference on link context
		bool mIsPost;                //!< Currently pushing Pre/Post flag 
	};
	
	//! Stacks levels container type
	typedef vector<StackLevel*> StackLevels;
	
	//! @brief Result of UID translation type
	typedef std::pair<UInt,bool> TrResult;
	
	
	//! @brief Substance Linker Handle
	SubstanceLinkerHandle *const handle;
	
	//! @brief Filled graph binary (receive alt. UID if collision)
	GraphBinary& graphBinary;
	
	//! @brief UID of graph state
	const UInt stateUid;
	
	//! @brief Stacked levels hierarchy
	StackLevels stackLevels;
	
	
	//! @brief Constructor from content
	LinkContext(SubstanceLinkerHandle *hnd,GraphBinary& gbinary,UInt uid);
	
	//! @brief Record Linker Collision UID 
	//! @param collisionType Output or input collision flag
	//! @param previousUid Initial UID that collide
	//! @param newUid New UID generated
	//! @note Called by linker callback
	void notifyLinkerUIDCollision(
		SubstanceLinkerUIDCollisionType collisionType,
		UInt previousUid,
		UInt newUid);
		
	//! @return Return translated UID following stacking
	//! @param isOutput Set to true if output UID searched, false if input
	//! @param previousUid Initial UID of the I/O to search for
	//! @param disabledValid If false, return false if I/O disabled by stacking
	//!		(connection, fuse, disable)
	//! @return Return a pair w/ translated UID value or previousUid if not
	//!		found, and boolean set to false if not found or found in stack and
	//!		disabledValid is false
	TrResult translateUid(
		bool isOutput,
		UInt previousUid,
		bool disabledValid) const;
	
	//! @return Search for translated UID 
	//! @param uidTranslates Container of I/O translated UIDs
	//! @param previousUid Initial UID of the I/O to search for
	//! @return Return a pair w/ translated UID value or previousUid if not
	//!		found, and boolean set to true if found
	static TrResult translate(
		const UidTranslates& uidTranslates,
		UInt previousUid);
		
protected:
	//! @return Return pointer on translated UID following stacking
	//! @param isOutput Set to true if output UID searched, false if input
	//! @param previousUid Initial UID of the I/O to search for
	//! @param disabledValid If false, return nullptr if I/O disabled by stacking
	//!		(connection, fuse, disable)
	//! @return Return the pointer on translated UID value (R/W) or nullptr if not
	//!		found or found in stack and disabledValid is false
	UInt* getTranslated(
		bool isOutput,
		UInt previousUid,
		bool disabledValid = true) const;

	//! @return Return pointer on translated UID in entries container
	//! @param previousUid Initial UID of the I/O to search for
	//! @return Return the pointer on translated UID value (R/W) or nullptr if not
	//!		found.
	template <typename EntriesType>
	UInt* searchTranslatedUid(EntriesType& entries,UInt previousUid) const;
		
};  // struct LinkContext


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKCONTEXT_H
