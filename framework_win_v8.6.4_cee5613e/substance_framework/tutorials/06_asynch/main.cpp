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

//!	This tutorial load a Substance Archive file (.SBSAR),
//!	and render ALL textures w/ asynchronous run and user callbacks usage.
//! User can launch computation w/ a different drawn random seed, even if the
//! previous one is not finished.
//! Texture are saved in DDS format

#include "../common/frameworktestcommon.h"

// Substance Framework include
#include <substance/framework/framework.h>

#include <vector>
#include <iostream>
#include <string>

#include <stdlib.h>
#include <stdio.h>


//! @brief Definition of user callbacks to plug in renderer
//! Inherit from SubstanceAir::RenderCallbacks
//!
//! Save computed output progressively.
struct MyCallbacks : public SubstanceAir::RenderCallbacks
{
	//! @brief Output computed callback (render result available) OVERLOAD
	//! Overload of SubstanceAir::RenderCallbacks::outputComputed
	virtual void outputComputed(
		SubstanceAir::UInt runUid,
		size_t /*userData*/,
		const SubstanceAir::GraphInstance* graphInstance,
		SubstanceAir::OutputInstance* outputInstance) override
	{
		// Grab result (auto pointer on RenderResult)
		SubstanceAir::OutputInstance::Result result(outputInstance->grabResult());
		if (result.get()!=nullptr && result->isImage())
		{
			const SubstanceAir::string &pkgurl = graphInstance->mDesc.mPackageUrl;

			// DDS filename
			std::stringstream filenamesstr;
			filenamesstr << CURRENT_DIR_PATH "output_" <<
				std::hex << runUid << std::dec << "_" <<             // Run UID
				pkgurl.substr(pkgurl.find_last_of('/')+1) << "_" <<  // Graph name
				outputInstance->mDesc.mIdentifier << "[" <<
				outputInstance->mDesc.mUid << "].dds";

			// Save output as DDS
			// The following write operation should not be done here
			// (time consuming operation), but in another thread!
			// For the sake of simplicity this test does not include synchronization
			// primitives that allow to transmit the render result to the main thread.
			Framework::Test::writeDDSFile(
				filenamesstr.str().c_str(),
				dynamic_cast<SubstanceAir::RenderResultImage*>(result.get())->getTexture());

			std::cerr<<filenamesstr.str()<<" written.\n";
		}
		// Buffer destroyed (auto pointer deletion at scope exit)
	}

};  // struct MyCallbacks


/** @brief main entry point */
int main(int argc,char **argv)
{
	// expect .sbsar (Archive) filepath
	if (argc<2)
	{
		std::cerr<<"Usage: "<<argv[0]<<" <sbsar filepath>\n";
		return 1;
	}

	// Load .sbsar file
	std::vector<unsigned char> archiveData;
	Framework::Test::loadFile(archiveData,argv[1],true);

	if (archiveData.empty())
	{
		std::cerr << "Unable to read sbsar files : \n\t"
			<< argv[1] << std::endl;
		return 2;
	}

	using namespace SubstanceAir;

	// Parse package (from Archive AND output options)
	PackageDesc packagedesc(
		&archiveData[0],
		archiveData.size());

	if (!packagedesc.isValid())
	{
		std::cerr << "Invalid package.\n";
		return 3;
	}

	// Instantiate all graphs of the package
	GraphInstances instances;
	instantiate(instances,packagedesc);

	// Callbacks instance
	MyCallbacks callbacks;

	// Create renderer object
	Renderer renderer;

	// Plug callbacks instance
	renderer.setRenderCallbacks(&callbacks);

	char command = 0;
	UInt runId = 0;

	// Commands help
	std::cerr << "Substance Framework Asynchronous sample:\n"
		"\tType 'q' to queing a new render w/ a different random seed.\n"
		"\tType 'f' to queing a new render, computed first.\n"
		"\tType 'o' for a new render, overwriting previous one.\n"
		"\tType 'e' to exit.\n\n";

	// Interactive loop
	do
	{
		// Set all "$randomseed" tweaks w/ a random draw value
		// Iterate on all graphs
		for (GraphInstances::const_iterator grite = instances.begin();
			grite!=instances.end();
			++grite)
		{
			// Iterate on all inputs
			const GraphInstance::Inputs& inputs = (*grite)->getInputs();
			for (GraphInstance::Inputs::const_iterator inite = inputs.begin();
				inite!=inputs.end();
				++inite)
			{
				InputInstanceBase &input = **inite;
				if (input.mDesc.mIdentifier=="$randomseed" &&   // randomseed tweak
					input.mDesc.mType==Substance_IOType_Integer) // security!
				{
					static_cast<InputInstanceInt*>(&input)->setValue(
						(rand()<<16)^rand());
				}
			}
		}

		// Push graphs instances into renderer
		renderer.push(instances);

		if (runId!=0 && renderer.isPending(runId))
		{
			// If not the first iteration
			// Display a message if previous computation is still active
			std::cerr << "Still computing! "<< (command=='o' ?
				"Previous renders are canceled.\n" :
				"Queuing this render.\n");
		}

		// Render asynchronous (w/ Replace flag if not Queuing command)
		// Store the corresponding Run UID into runId
		runId = renderer.run( Renderer::Run_Asynchronous |
			(command=='o' ? Renderer::Run_Replace : 0) |
			(command=='f' ? Renderer::Run_First : 0) );

		std::cerr << "Render w/ UID [" <<
				std::hex << runId << std::dec << "] launched.\n";

		// Wait for user command
		do
		{
			command = getc(stdin);
		}
		while (command!='e' && command!='q' && command!='o' && command!='f');
	}
	while (command!='e');

	if (renderer.isPending(runId))
	{
		// Display a message if last computation is still active
		std::cerr << "Still computing! Render canceled.\n.";
	}

	std::cout << "Bye.\n";

	return 0;
}
