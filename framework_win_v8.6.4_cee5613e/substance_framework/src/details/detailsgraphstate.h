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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSGRAPHSTATE_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSGRAPHSTATE_H

#include "detailsinputstate.h"
#include "detailsoutputstate.h"
#include "detailsutils.h"


namespace SubstanceAir
{

class GraphInstance;
class InputInstanceBase;

namespace Details
{

struct GraphBinary;
class RenderJob;
class LinkData;
class DeltaState;


//! @brief Graph Instance State
//! Represent the state of one or several Graph instance.
//! This is the state of input values w/ all render job PUSHED.
//!
//! Also contains a pointer on associated GraphBinary that contains UID/index
//! translation of linked SBSBIN.
class GraphState
{
public:
	//! @brief Pointer on LinkData type
	typedef shared_ptr<LinkData> LinkDataPtr;


	//! Create from graph instance
	//! @param graphInstance The source graph instance, initialize state
	//! @note Called from user thread
	GraphState(GraphInstance& graphInstance);

	//! @brief Destructor
	~GraphState();

	//! @brief Accessor on input state
	const InputState& getInput(size_t i) const { return mInputStates.at(i); }

	//! @brief Accessor on output state
	const OutputState& getOutput(size_t i) const { return mOutputStates.at(i); }

	//! @brief Accessor on pointers on InputImage indexed by input states
	const InputImage::SPtr& getInputImage(size_t i) const { return mInputImagePtrs[i]; }

	//! @brief Accessor on pointers on string indexed by input states
	const InputStringPtr& getInputString(size_t i) const { return mInputStringPtrs[i]; }

	//! @brief Apply delta state to current state
	void apply(const DeltaState& deltaState);

	//! @brief This state is currently linked (present in current SBSBIN data)
	bool isLinked() const;

	//! @brief Accessor on Graph state uid
	UInt getUid() const { return mUid; }

	//! @brief Accessor on current GraphBinary
	GraphBinary& getBinary() const { return *mGraphBinary; }

	//! @brief Accessor on Pointer to link data corresponding to graph instance
	const LinkDataPtr& getLinkData() const { return mLinkData; }

protected:
	typedef vector<InputState> InputStates;    //!< Vector of InputState
	typedef vector<OutputState> OutputStates;  //!< Vector of OutputState


	//! @brief Graph state uid
	const UInt mUid;

	//! @brief Pointer link data corresponding to graph instance
	//! Allows to keep ownership on datas required at link time
	const LinkDataPtr mLinkData;

	//! @brief Array of input states in GraphInstance/GraphDesc order
	InputStates mInputStates;

	//! @brief Array of pointers on InputImage indexed by mInputStates
	Utils::SparseVectorPtr<InputImage> mInputImagePtrs;

	//! @brief Array of pointers on string indexed by mInputStates
	Utils::SparseVectorPtr<ucs4string> mInputStringPtrs;

	//! @brief Array of output states in GraphInstance/GraphDesc order
	OutputStates mOutputStates;

	//! @brief Graph binary associated with this instance of GraphState.
	//! Owned by GraphState.
	GraphBinary* mGraphBinary;

private:
	GraphState(const GraphState&);
	const GraphState& operator=(const GraphState&);
};  // class GraphState


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSGRAPHSTATE_H
