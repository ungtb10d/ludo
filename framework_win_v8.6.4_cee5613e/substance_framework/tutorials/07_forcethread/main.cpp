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
//!	and render ALL textures w/ synchronous run. The render process is forced to
//!	current thread using runRenderProcess() callback.
//! Texture are saved in DDS format

#include "../common/frameworktestddssave.h"
#include "../common/frameworktestcommon.h"

// Substance Framework include
#include <substance/framework/framework.h>

#include <vector>
#include <iostream>
#include <sstream>
#include <string>


//! @brief Definition of user callbacks to plug in renderer
//! Inherit from SubstanceAir::RenderCallbacks
//!
//! Save computed output progressively.
struct MyCallbacks : public SubstanceAir::RenderCallbacks
{
	//! @brief Render process execution callback OVERLOAD
	//! Overload of SubstanceAir::RenderCallbacks::runRenderProcess
	//! @param renderFunction Pointer to function to call.
	//! @param renderParams Parameters of the function.
	//! @return Returns true: render process execution is handled.
	//!
	//! Allows custom thread allocation for rendering process.
	//! Call renderFunction w/ renderParams. This function returns when all
	//! pending render jobs are processed.
	//! Called each time a batch a jobs need to be rendered AND at Renderer
	//! deletion.
	//!
	//! This implementation just calls renderFunction from this thread:
	//! no other threads are spawned by substance framework.
	bool runRenderProcess(
		SubstanceAir::RenderFunction renderFunction,
		void* renderParams)
	{
		std::cerr << "MyCallbacks::runRenderProcess() called." << std::endl;

		// Just call provided function w/ parameters in current thread.
		renderFunction(renderParams);

		// Render process handled, return true
		return true;
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

	// Parse package (from Archive)
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

	// Push graphs instances into renderer
	renderer.push(instances);

	// Render synchronous
	// Asynchronous flag have no effect: render process is done on same thread
	// via MyCallbacks::runRenderProcess() callback.
	renderer.run();

	// Grab all outputs of all graphs

	// Iterate on all graphs
	for (GraphInstances::const_iterator grite = instances.begin();
		grite!=instances.end();
		++grite)
	{
		const string &pkgurl = (*grite)->mDesc.mPackageUrl;

		// Iterate on all outputs
		const GraphInstance::Outputs& outputs = (*grite)->getOutputs();
		for (GraphInstance::Outputs::const_iterator outite = outputs.begin();
			outite!=outputs.end();
			++outite)
		{
			// Grab result (auto pointer on RenderResult)
			OutputInstance::Result result((*outite)->grabResult());
			if (result.get()!=nullptr && result->isImage())
			{
				// DDS filename
				std::stringstream filenamesstr;
				filenamesstr << CURRENT_DIR_PATH "output_" <<
					pkgurl.substr(pkgurl.find_last_of('/')+1) << "_" <<  // Graph name
					(*outite)->mDesc.mIdentifier << "[" <<
					(*outite)->mDesc.mUid << "].dds";

				// Save output as DDS
				Framework::Test::writeDDSFile(
					filenamesstr.str().c_str(),
					dynamic_cast<RenderResultImage*>(result.get())->getTexture());

			}
			// Buffer destroyed (auto pointer deletion at scope exit)
		}
	}

	std::cout << "Done.\n";

	return 0;
}
