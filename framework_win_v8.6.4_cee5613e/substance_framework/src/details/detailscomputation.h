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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSCOMPUTATION_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSCOMPUTATION_H


#include <substance/framework/platform.h>
#include <substance/framework/typedefs.h>

#include <substance/inputdesc.h>




namespace SubstanceAir
{
namespace Details
{

class Engine;
struct InputState;
struct InputImageToken;


//! @brief Substance Engine Computation (render) action
class Computation
{
public:
	//! @brief Container of indices
	typedef vector<UInt> Indices;


	//! @brief Constructor from engine
	//! @param flush Flush internal engine queue if true
	//! @pre Engine is valid and correctly linked
	//! @post Engine render queue is flushed if 'flush' is true
	Computation(Engine& engine,bool flush);

	//! @brief Set current user data to push w/ I/O
	void setUserData(size_t userData) { mUserData = userData; }

	//! @brief Get current user data to push w/ I/O
	size_t getUserData() const { return mUserData; }

	//! @brief Push input
	//! @param index SBSBIN index
	//! @param inputState Input type, value and other flags
	//! @param value Value to push
	//! @return Return if input really pushed
	bool pushInput(
		UInt index,
		const InputState& inputState,
		void* value);

	//! @brief Push outputs SBSBIN indices to compute
	void pushOutputs(const Indices& indices);

	//! @brief Run computation
	//! Push hints I/O and run
	void run();

	//! @brief Accessor on current engine
	Engine& getEngine() { return mEngine; }

protected:
	//! @brief Reference on parent Engine
	Engine &mEngine;

	//! @brief Hints outputs indices (SBSBIN indices)
	Indices mHintOutputs;

	//! @brief Hints inputs SBSBIN indices (24 MSB bits) and types (8 LSB bits)
	Indices mHintInputs;

	//! @brief Current pushed user data
	size_t mUserData;


	//! @brief Get hint input type from mHintInputs value
	static SubstanceIOType getType(UInt v) { return (SubstanceIOType)(v&0xFF); }

	//! @brief Get hint index from mHintInputs value
	static UInt getIndex(UInt v) { return v>>8; }

	//! @brief Get mHintInputs value from type and index
	static UInt getHintInput(SubstanceIOType t,UInt i) { return (i<<8)|(UInt)t; }

private:
	Computation(const Computation&);
	const Computation& operator=(const Computation&);
};  // class Computation


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSCOMPUTATION_H
