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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSRENDERPUSHIO_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSRENDERPUSHIO_H

#include "detailsdeltastate.h"

#include <substance/callbacks.h>
#include <substance/iotype.h>


#include <set>


namespace SubstanceAir
{

class GraphInstance;
class OutputInstance;
struct RenderResultBase;

namespace Details
{

class Computation;
class GraphState;
class RenderJob;
class RenderToken;
class States;
struct DuplicateJob;
struct OutputsFilter;
class Engine;


//! @brief Render Push Input/Output batch
//! Contains description of which input and outputs to push for several instances
class RenderPushIO
{
public:
	//! @brief Complete return enumeration
	enum Complete
	{
		Complete_No,
		Complete_Yes,
		Complete_DontKnow       //!< No push I/O commands
	};

	//! @brief States enumeration
	//! Can be combined
	enum State
	{
		State_None            = 0x00,     //!< Nothing to push / pushed
		State_Process         = 0x10,     //!< Into current process
		State_InputsPending   = 0x01,     //!< Input(s) to push
		State_InputsPushed    = 0x02,     //!< All inputs push commands resolved
		State_OutputsPending  = 0x04,     //!< Output(s) to compute
		State_OutputsComputed = 0x08,     //!< All outputs computed
	};

	//! @brief Enqueue process return enumeration
	enum Process
	{
		Process_Append,         //!< Successfully append to process
		Process_LinkRequired,   //!< Succeeded but missing linked graphs
		Process_Collision       //!< Failed: Link output format collision
	};


	//! @brief Create from render job
	//! @param renderJob Parent render job that owns this instance
	//! @note Called from user thread
	RenderPushIO(RenderJob &renderJob);

	//! @brief Constructor from push I/O to duplicate
	//! @param renderJob Parent render job that owns this instance
	//! @param src The canceled push I/O to copy
	//! @param dup Duplicate job context
	//! Build a push I/O from a canceled one and optionally filter outputs.
	//! Push again SRC render tokens (of not filtered outputs).
	//! @warning Resulting push I/O can be empty (hasOutputs()==false) if all
	//!		outputs are filtered OR already computed. In this case this push IO
	//!		is no more useful and can be removed.
	//! @note Called from user thread
	RenderPushIO(RenderJob &renderJob,const RenderPushIO& src,DuplicateJob& dup);

	//! @brief Destructor
	//! @note Called from user thread
	~RenderPushIO();

	//! @brief Create a push I/O pair from current state & current instance
	//! @param graphState The current graph state
	//! @param graphInstance The pushed graph instance (not kept)
	//! @note Called from user thread
	//! @return Return true if at least one dirty output
	bool push(GraphState &graphState,const GraphInstance &graphInstance);

	//! @brief Prepend reverted input delta into duplicate job context
	//! @param dup The duplicate job context to accumulate reversed delta
	//! Use to restore the previous state of copied jobs.
	//! @note Called from user thread
	void prependRevertedDelta(DuplicateJob& dup) const;

	//! @brief Push input and output in engine handle
	//! @param inputsOnly If true only inputs are pushed
	//! @pre Must be enqueued into link if not input only
	//! @note Called from Render queue thread
	//! @return Return false if not processed (collision occurs)
	bool pull(Computation &computation,bool inputsOnly);

	//! @brief Fill filter outputs structure from this push IO
	//! @param[in,out] filter The filter outputs structure to fill
	void fill(OutputsFilter& filter) const;

	//! @brief Enqueue to process
	//! Prepare this push I/O for link if necessary.
	//! If already link, mark only outputs as enqueued in link
	//! @pre Output state changes must not collide (same instance & output).
	//!		Must be not already complete.
	//! @post Flatten output state changes in binary graph.
	//!		Set State_EnqueuedLink flag.
	//! @return Return process enum
	Process enqueueProcess();

	//! @brief Cancel notification, decrease RenderToken counters
	void cancel();

	//! @brief Accessor: At least one output to compute
	//! Check all render tokens if not already filled
	bool hasOutputs() const;

	//! @brief Return if the current Push I/O is completed
	//! @param inputOnly Only inputs are required
	//! @note Called from Render queue thread
	//! @return Can return "don't know" if no pending I/O
	Complete isComplete(bool inputOnly = false) const;

	//! @brief Engine job completed
	//! @note Called by engine callback from Render queue thread
	void callbackJobComplete();

	//! @brief Get output type from SBSBIN index
	//! @pre The output must be currently pushed
	//! @param index Output SBSBIN index of completed output
	//! @note Called by engine callback from Render queue thread
	SubstanceIOType getOutputType(UInt index) const;

	//! @brief Engine output completed
	//! @param index Output SBSBIN index of completed output
	//! @param renderResult The result texture, ownership transfered
	//! @note Called by engine callback from Render queue thread
	void callbackOutputComplete(UInt index,RenderResultBase* renderResult);

	//! @brief Accessor on parent job engine pointer filled when pulled
	Engine* getEngine() const;

	//! @brief Accessor on current state flags
	unsigned int getState() const { return mState; }

protected:

	//! @brief One output description
	struct Output
	{
		//! @brief GraphState/GraphInstance order index
		UInt index;

		//! @brief Output type
		//! Used at output grabbing, constant.
		SubstanceIOType type;

		//! @brief Pointer on destination render token to fill
		shared_ptr<RenderToken> renderToken;

		//! @brief Pointer on corresponding graph instance
		//! @warning Callback purpose only, may be dandling pointer
		const GraphInstance* graphInstance;

		//! @brief Pointer on corresponding output instance
		//! @warning Callback purpose only, may be dandling pointer
		OutputInstance* outputInstance;

	};  // struct Output

	//! @brief Input/Output to push for one instance
	//! Contains description of which input and outputs to push one
	//! GraphState (reference contained in DeltaState)
	struct Instance
	{
		//! @brief Container of outputs
		typedef vector<Output> Outputs;

		//! @brief Build a push I/O pair from current state & current instance
		//! @param state The current graph state
		//! @param graphInstance The pushed graph instance (not kept)
		Instance(
			GraphState &state,
			const GraphInstance &graphInstance);

		//! @brief Duplicate instance (except outputs)
		Instance(const Instance &src);

		//! @brief GraphState associated to this instance
		GraphState &graphState;

		//! @brief UID of the GraphInstance used to build this
		const UInt instanceUid;

		//! @brief Description of I/O w/ values to push
		DeltaState deltaState;

		//! @brief List of outputs indices/destination to push
		//! GraphState/GraphInstance order indices.
		Outputs outputs;

		//! @brief Accessor: At least one output to compute
		//! Check all render tokens if not already filled
		bool hasOutputs() const;

	};  // struct Instance

	//! @brief Vector of Instances (this instance ownership)
	typedef vector<Instance*> Instances;


	//! @brief Parent render job
	RenderJob &mRenderJob;

	//! @brief Current state
	atomic_uint mState;

	//! @brief I/O of all instances
	Instances mInstances;

	//! @brief Outputs array (engine index order)
	//! Filled when pushed into Engine (non required outputs set to nullptr)
	vector<const Output*> mDestOutputs;

	//! @brief Number of inputs Engine job pushed to proceed
	//! Filled when pushed into Engine
	//! Decreased by callbackJobComplete, can be negative: last job is output
	//! computation.
	int mInputJobPendingCount;

private:
	RenderPushIO(const RenderPushIO&);
	const RenderPushIO& operator=(const RenderPushIO&);
};  // class RenderPushIO


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSRENDERPUSHIO_H
