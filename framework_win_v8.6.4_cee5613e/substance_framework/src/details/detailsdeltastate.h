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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDELTASTATE_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDELTASTATE_H

#include "detailsinputstate.h"
#include "detailsoutputstate.h"




namespace SubstanceAir
{

class GraphInstance;

namespace Details
{

class GraphState;


//! @brief Delta of graph instance inputs and output state
//! Represents a modification of a graph state: new and previous values of all
//! modified inputs and output format override
class DeltaState
{
public:
	//! @brief One I/O modification
	//! IOState is InputState or OutputState
	template <class IOState>
	struct Item
	{
		size_t index;      //!< Index in GraphState/GraphInstance order
		IOState previous;  //!< Previous state
		IOState modified;  //!< New state

	};  // struct Item

	typedef Item<InputState> Input;          //!< One input modification
	typedef vector<Input> Inputs;       //!< Vector of input states delta
	typedef Item<OutputState> Output;        //!< One output modification
	typedef vector<Output> Outputs;     //!< Vector of output states delta

	//! @brief Append modes
	enum AppendMode
	{
		Append_Default  = 0x0,    //!< Merge, use source modified values
		Append_Override = 0x1,    //!< Override previous values
		Append_Reverse  = 0x2     //!< Use source previous values
	};  // enum AppendMode


	//! @brief Accessor on inputs
	const Inputs& getInputs() const { return mInputs; }

	//! @brief Accessor on outputs
	const Outputs& getOutputs() const { return mOutputs; }

	//! @brief Returns if null delta
	bool isIdentity() const { return mInputs.empty() && mOutputs.empty(); }

	//! @brief Accessor on pointers on InputImage indexed by mInputs
	InputImage::SPtr getInputImage(size_t i) const { return mInputImagePtrs.at(i); }

	//! @brief Accessor on pointers on string indexed by input states
	const InputStringPtrs::value_type& getInputString(size_t i) const { return mInputStringPtrs.at(i); }

	//! @brief Fill a delta state from current state & current instance values
	//! @param graphState The current graph state used to generate delta
	//! @param graphInstance The pushed graph instance (not kept)
	void fill(
		const GraphState &graphState,
		const GraphInstance &graphInstance);

	//! @brief Append a delta state to current
	//! @param src Source delta state to append
	//! @param mode Append policy flag
	void append(const DeltaState &src,AppendMode mode);

protected:
	//! @brief New and previous values of all modified inputs
	//! Input::index ordered
	Inputs mInputs;

	//! @brief Array of pointers on InputImage indexed by mInputs
	InputImagePtrs mInputImagePtrs;

	//! @brief Array of pointers on string indexed by mInputs
	InputStringPtrs mInputStringPtrs;

	//! @brief New and previous values of all modified outputs
	//! Input::index ordered
	Outputs mOutputs;

	//! @brief Append a delta of I/O sub method
	//! @param destItems Destination inputs or outputs (from this)
	//! @param srcItems Source inputs or outputs (from src)
	//! @param src Source delta state to append
	//! @param mode Append policy flag
	template <typename Items>
	void append(
		Items& destItems,
		const Items& srcItems,
		const DeltaState &src,
		AppendMode mode);

	//! @brief Returns if states are equivalent
	//! Two implementations for input and output
	template <typename StateType>
	bool isEqual(
		const StateType& a,
		const StateType& b,
		const DeltaState &bparent) const;

	//! @brief Record child state
	//! @param[in,out] inputState The state to record image pointer
	//! @param parent Source delta state parent of inputState that contains
	//! 	image pointers.
	//! For inputs: record new image pointer if necessary,
	//! do nothing for outputs.
	template <typename StateType>
	void record(
		StateType& inputState,
		const DeltaState &parent);

};  // class DeltaState


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDELTASTATE_H
