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

#include "details/detailsxml.h"
#include "details/detailsutils.h"

#include <substance/framework/preset.h>
#include <substance/framework/graph.h>
#include <substance/framework/callbacks.h>

#include <utility>
#include <algorithm>
#include <iterator>


//! @brief Apply this preset to a graph instance
//! @param graph The target graph instance to apply this preset
//! @param mode Reset to default other inputs of merge w/ previous values
//! @return Return true whether at least one input value is applied
bool SubstanceAir::Preset::apply(GraphInstance& graph,ApplyMode mode) const
{
	typedef std::pair<string,size_t> IdentifierInputPair;
	typedef vector<IdentifierInputPair> IdentifierInputDict;
	typedef vector<int> InputsSet;

	IdentifierInputDict mIdentifierInputDict;

	bool atleastoneset = false;
	const GraphDesc &desc = graph.mDesc;

	InputsSet mInputsSet;
	mInputsSet.resize(desc.mInputs.size());
	std::fill(mInputsSet.begin(),mInputsSet.end(),0);

	for (const auto& inpvalue : mInputValues)
	{
		// Found by input UID
		GraphDesc::Inputs::const_iterator inpite = desc.mInputs.begin();
		std::advance(inpite,desc.findInput(inpvalue.mUid));

		if (inpite==desc.mInputs.end())
		{
			// Try per input identifier

			// If not already done, build the dict
			if (mIdentifierInputDict.empty())
			{
				size_t index = 0;
				mIdentifierInputDict.reserve(desc.mInputs.size());
				for (const auto& inputbase : desc.mInputs)
				{
					mIdentifierInputDict.push_back(std::make_pair(
						inputbase->mIdentifier,
						index++));
				}
				std::sort(
					mIdentifierInputDict.begin(),
					mIdentifierInputDict.end());
			}

			IdentifierInputDict::const_iterator idite = std::lower_bound(
				mIdentifierInputDict.begin(),
				mIdentifierInputDict.end(),
				std::make_pair(inpvalue.mIdentifier,(size_t)0));
			if (idite!=mIdentifierInputDict.end() &&
				idite->first==inpvalue.mIdentifier)
			{
				// Identifier found
				inpite = desc.mInputs.begin();
				std::advance(inpite,idite->second);
			}
		}

		if (inpite!=desc.mInputs.end() &&
			(*inpite)->mType==inpvalue.mType)
		{
			// Retrieve corresponding (and type match)
			size_t index = std::distance(desc.mInputs.begin(),inpite);
			assert(graph.getInputs().size()>=desc.mInputs.size());
			InputInstanceBase *inputbase = graph.getInputs()[index];

			if (inputbase->mDesc.isNumerical())
			{
				// numerical
				static_cast<InputInstanceNumericalBase*>(
					inputbase)->setValue(inpvalue.mValue);
			}
			else if (inputbase->mDesc.isString())
			{
				// string
				static_cast<InputInstanceString*>(
					inputbase)->setString(inpvalue.mValue);
			}
			else if (inputbase->mDesc.isImage() &&
				GlobalCallbacks::getInstance()!=nullptr)
			{
				// image
				GlobalCallbacks::getInstance()->loadInputImage(
					*static_cast<InputInstanceImage*>(inputbase),
					inpvalue.mValue);
			}

			atleastoneset = true;
			mInputsSet[index]++;
		}
	}

	if (mode==Apply_Reset)
	{
		// Reset non modified inputs (only numerical and string)
		InputsSet::const_iterator inpsite = mInputsSet.begin();
		for (const auto& inputbase : graph.getInputs())
		{
			if (*inpsite==0 && !inputbase->mDesc.isImage())
			{
				inputbase->reset();
			}
			++inpsite;
		}
	}

	return atleastoneset;
}


//! @brief Fill this preset from a graph instance input values
//! @param graph The source graph instance.
//!
//! 'mInputValues' is filled w/ non default input values of 'graph'.
//! 'mPackageUrl' is also set. All other members are not modified (label
//!	and description).
void SubstanceAir::Preset::fill(const GraphInstance& graph)
{
	mPackageUrl = graph.mDesc.mPackageUrl;

	for (const auto& inputbase : graph.getInputs())
	{
		if (inputbase->isNonDefault())
		{
			string value;
			if (inputbase->mDesc.isNumerical())
			{
				// Numeric input
				value = static_cast<const InputInstanceNumericalBase*>(
					inputbase)->getValueAsString();
			}
			else if (inputbase->mDesc.isString())
			{
				// string
				value = static_cast<InputInstanceString*>(
					inputbase)->getString();
			}
			else if (inputbase->mDesc.isImage() &&
				GlobalCallbacks::getInstance()!=nullptr)
			{
				// Image input, use global callback to obtain filepath
				value = GlobalCallbacks::getInstance()->getImageFilepath(
					*static_cast<const InputInstanceImage*>(inputbase)
						->getImage().get());
			}

			if (!value.empty() || inputbase->mDesc.isString())
			{
				// Append to input values if defined
				mInputValues.resize(mInputValues.size()+1);
				InputValue &inpvalue = mInputValues.back();
				inpvalue.mUid = inputbase->mDesc.mUid;
				inpvalue.mIdentifier = inputbase->mDesc.mIdentifier;
				inpvalue.mType = inputbase->mDesc.mType;
				inpvalue.mValue = value;
			}
		}
	}
}


//! @brief Preset serialization (to UTF8 XML description) stream operator
std::ostream& SubstanceAir::operator<<(std::ostream& os,const Preset& preset)
{
	os << " <sbspreset pkgurl=\""<<Details::encodeToXmlEntities(preset.mPackageUrl)<<
		"\" label=\""<<Details::encodeToXmlEntities(preset.mLabel)<<"\" >\n";

	for (const auto& inpvalue : preset.mInputValues)
	{
		os << "  <presetinput identifier=\""<<inpvalue.mIdentifier
			<<"\" uid=\""<<inpvalue.mUid
			<<"\" type=\""<<(unsigned int)inpvalue.mType
			<<(inpvalue.mType==Substance_IOType_Image ?
				"\" filepath=\"" :
				"\" value=\"")
			<<(inpvalue.mType == Substance_IOType_String ?
				Details::encodeToXmlEntities(inpvalue.mValue) :
				inpvalue.mValue)<<"\" />\n";
	}

	os << " </sbspreset>\n";

	return os;
}


//! @brief Parse preset from a XML preset element (<sbspreset>)
//! @param[out] preset Preset description to fill from XML string
//! @param xmlString Pointer on UTF8 Null terminated string that contains
//!		serialized XML preset element <sbspreset>
//! @return Return whether preset parsing succeed
bool SubstanceAir::parsePreset(Preset &preset,const char* xmlString)
{
	Details::XmlDocument document;
	document.Parse(xmlString);

	if (!document.Error())
	{
		Details::XmlElement* rootnode = document.FirstChildElement("sbspreset");

		if (rootnode)
		{
			Details::parsePreset(preset,rootnode,nullptr);
			return true;
		}
		else
		{
			// TODO Warning: not a preset entry
		}
	}

	return false;
}


//! @brief Presets serialization (to UTF8 XML description) stream operator
std::ostream& SubstanceAir::operator<<(std::ostream& os,const Presets& presets)
{
	os << "<sbspresets formatversion=\"1.1\" count=\""<<presets.size()<<"\" >\n";

	for (const auto& preset : presets)
	{
		os << preset;
	}

	os << "</sbspresets>\n";

	return os;
}


//! @brief Parse presets from a XML preset file (.SBSPRS)
//! @param[out] presets Container filled with loaded presets
//! @param xmlString Pointer on UTF8 Null terminated string that contains
//!		presets XML description
//! @return Return whether preset parsing succeed
bool SubstanceAir::parsePresets(Presets &presets,const char* xmlString)
{
	Details::XmlDocument document;
	document.Parse(xmlString);

	if (document.Error())
	{
		// TODO Warning: not a XML file
		return false;
	}

	Details::XmlElement* rootnode = document.FirstChildElement("sbspresets");

	const size_t initialsize = presets.size();
	if (rootnode)
	{
		Details::parsePresets(presets,rootnode,nullptr);
	}
	else
	{
		// TODO Warning: not a preset file
	}

	return presets.size()>initialsize;
}
