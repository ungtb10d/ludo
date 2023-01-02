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

#include "frameworktestcommon.h"

#include <iostream>


void Framework::Test::saveAllOutputsAsDDS(
	const SubstanceAir::GraphInstances& instances,
	const SubstanceAir::string& prefix)
{
	using namespace SubstanceAir;

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
			unsigned int resultindex = 0;
			for (OutputInstance::Result result((*outite)->grabResult());
				result.get()!=nullptr;
				result=(*outite)->grabResult(),++resultindex)
			{
				if (result->isImage())
				{
					// DDS filename
					std::stringstream filenamesstr;
					filenamesstr << CURRENT_DIR_PATH << prefix <<
						pkgurl.substr(pkgurl.find_last_of('/')+1) << "_" <<  // Graph name
						(*outite)->mDesc.mIdentifier << "[" <<
						(*outite)->mDesc.mUid << "]";
					if (resultindex>0)
					{
						// Several renders accumulated into output: append index
						filenamesstr << "(" << resultindex << ")";
					}
					filenamesstr << ".dds";

					// Save output as DDS
					writeDDSFile(
						filenamesstr.str().c_str(),
						dynamic_cast<RenderResultImage*>(result.get())->getTexture());
				}
				else
				{
					std::cout << "Numerical output " <<
						pkgurl << "/" <<
						(*outite)->mDesc.mIdentifier << " : " <<
						dynamic_cast<RenderResultNumericalBase*>(result.get())->getValueAsString() << "\n";
				}
			}
			// Buffer destroyed (auto pointer deletion at scope exit)
		}
	}
}
