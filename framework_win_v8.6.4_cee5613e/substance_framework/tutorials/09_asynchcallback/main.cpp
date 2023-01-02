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

//!	This tutorial load a Substance Archive file (.SBSAR) and render ALL
//!	textures. Texture are saved in DDS format


#include "../common/frameworktestddssave.h"
#include "../common/frameworktestcommon.h"

// Substance Framework include
#include <substance/framework/framework.h>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

/** @brief Custom Render Callback to demonstrate async job completion callback */
class AsynchCallbackCallbacks : public SubstanceAir::RenderCallbacks
{
public:
	//! @brief Job computed callback
	//! @param runUid The UID of the corresponding computation (returned by
	//! 	Renderer::run()).
	//! @param userData The user data set at corresponding run
	//!		(as second argument Renderer::run(xxx,userData))
	//! This is called every time a job is completed by the renderer.
	virtual void jobComputed(SubstanceAir::UInt runUid, size_t) override
	{
		// record the job completion and signal
		std::unique_lock<std::mutex> lock(mJobMutex);
		mCompletedJobs.emplace(runUid);
		mJobCv.notify_all();
	}

	//! @brief Wait for given runUid to complete
	//! @param runUid The UID of the corresponding computation (returned by
	//! 	Renderer::run()).
	void waitForJob(SubstanceAir::UInt runUid)
	{
		std::unique_lock<std::mutex> lock(mJobMutex);
		mJobCv.wait(lock, [&]{
			if (mCompletedJobs.count(runUid))
			{
				mCompletedJobs.erase(runUid);
				return true;
			}
			else
			{
				return false;
			}
		});
	}

private:
	//! @brief Store completed job uid's
	SubstanceAir::set<SubstanceAir::UInt> mCompletedJobs;

	//! @brief Condition Variable to implement waitForJob functionality
	std::condition_variable mJobCv;

	//! @brief Mutex for locking condition variable
	std::mutex mJobMutex;
};

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

	// Create renderer object
	Renderer renderer;

	// Assign render callbacks
	static AsynchCallbackCallbacks sRenderCallbacks;
	renderer.setRenderCallbacks(&sRenderCallbacks);

	// Push graphs instances into renderer
	renderer.push(instances);

	// Render asynchronous
	SubstanceAir::UInt runUid = renderer.run(SubstanceAir::Renderer::Run_Asynchronous);

	// Wait for signal from job complete callback before continuing
	sRenderCallbacks.waitForJob(runUid);

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
			// Grab result (auto pointer on RenderResultBase)
			OutputInstance::Result result((*outite)->grabResult());
			if (result.get()!=nullptr)
			{
				std::cout << "Render result:\n"
					"\tOutput identifier: " << (*outite)->mDesc.mIdentifier <<
					"\n\tOutput UID: " << (*outite)->mDesc.mUid <<
					"\n\tPackage URL: " << pkgurl <<
					"\n\tType: " << (*outite)->mDesc.mType << "\n";

				if (result->isImage())
				{
					RenderResultImage*const resimg = dynamic_cast<RenderResultImage*>(result.get());
					std::cout <<
						"\tWidth: " << resimg->getTexture().level0Width <<
						"\n\tHeight: " << resimg->getTexture().level0Height <<
						"\n\tFormat: " << (int)resimg->getTexture().pixelFormat<<"\n";

					// resimg->getTexture().buffer -> Pixel data

					// DDS filename
					std::stringstream filenamesstr;
					filenamesstr << CURRENT_DIR_PATH "output_" <<
						pkgurl.substr(pkgurl.find_last_of('/')+1) << "_" <<  // Graph name
						(*outite)->mDesc.mIdentifier << "[" <<
						(*outite)->mDesc.mUid << "].dds";

					// Save output as DDS or numerical print to console
					Framework::Test::writeDDSFile(
						filenamesstr.str().c_str(),
						resimg->getTexture());
				}
				else
				{
					std::cout <<
						"\tValue: " << dynamic_cast<RenderResultNumericalBase*>(result.get())->getValueAsString() << "\n";
				}
			}
			// Buffer destroyed (auto pointer deletion at scope exit)
		}
	}

	std::cout << "Done.\n";

	return 0;
}
