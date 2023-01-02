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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSGRAPHBINARY_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSGRAPHBINARY_H

#include "detailsoutputstate.h"


#include <utility>


namespace SubstanceAir
{

class GraphInstance;

namespace Details
{


//! @brief Graph binary information
//! Contains initial UIDs and UID/index translation of a graph state into
//! linked SBSBIN.
//!
//! Filled during link process (UID collision) and at Engine handle creation
//! time (indexes).
struct GraphBinary
{
	//! @brief One input/base output binary description
	//! Only uidInitial is valid if graph not already linked
	struct Entry
	{
		UInt uidInitial;     //! Initial UID
		UInt uidTranslated;  //! Translated UID (uid collision at link time)
		UInt index;          //! Index in SBSBIN
	};  // struct Entry

	typedef Entry Input;
	typedef vector<Input> Inputs;
	typedef vector<Input*> InputsPtr;

	struct Output : public Entry
	{
		OutputState state;            //!< Required output format
		bool enqueuedLink;            //!< Currently pending in link queue
		unsigned int descFormat;      //!< Format into description
		unsigned int descMipmap;      //!< Mipmap into description
		bool descForced;              //!< Desc mipmap &| format is forced
	};  // struct Output

	typedef vector<Output> Outputs;
	typedef vector<Output*> OutputsPtr;

	//! @brief Predicate used for sorting/searching entries per initial UID
	struct InitialUidOrder
	{
		bool operator()(const Entry* a,const Entry* b) const {
			return a->uidInitial<b->uidInitial; }
		bool operator()(UInt a,const Entry* b) const { return a<b->uidInitial; }
		bool operator()(const Entry* a,UInt b) const { return a->uidInitial<b; }
	};  // struct InitialUidOrder


	//! @brief Inputs in GraphState/GraphInstance order
	Inputs inputs;

	//! @brief Outputs in GraphState/GraphInstance order
	Outputs outputs;

	//! @brief Input entry pointers sorted by UID initial order
	InputsPtr sortedInputs;

	//! @brief Output entry pointers sorted by UID initial order
	OutputsPtr sortedOutputs;

	//! @brief Invalid index
	static const UInt invalidIndex;


	//! Create from graph instance
	//! @param graphInstance The graph instance, used to fill UIDs
	GraphBinary(GraphInstance& graphInstance);

	//! Reset translated UIDs before new link process
	void resetTranslatedUids();

	//! @brief This binary is currently linked (present in current SBSBIN data)
	bool isLinked() const { return mLinked; }

	//! @brief Notify that this binary is linked
	void linked();

	//! @brief Reset linked flag and per output enqueuedLink flag
	void resetLinked();

protected:
	bool mLinked;                     //!< Is linked flag

};  // struct GraphBinary


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSGRAPHBINARY_H
