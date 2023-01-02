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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_PACKAGE_H
#define _SUBSTANCE_AIR_FRAMEWORK_PACKAGE_H

#include "graph.h"
#include "outputopt.h"




namespace SubstanceAir
{

namespace Details
{
	class LinkData;
}


//! @brief Class that represents a Substance assembly (data)
//! Contains graphs definitions.
//! @warning The package desc must be alive as long as GraphInstance objects
//!		built from this content are alive.
class PackageDesc
{
public:
	//! @brief Graph descriptions container definition
	typedef vector<GraphDesc> Graphs;

	//! @brief Construct from Substance archive (.sbsar) file
	//! @param archivePtr Pointer on archive data (this buffer can be
	//!		deleted just after).
	//! @param archiveSize Size of buffer referenced by archivePtr
	//! @param outputOptions Optional output options. Allows to override
	//!		outputs format, mipmap, layout, etc.
	PackageDesc(
		const void *archivePtr,
		size_t archiveSize,
		const OutputOptions& outputOptions = OutputOptions());

	//! @brief Construct from assembly data (.sbsasm) and its xml description
	//! @param assemblyPtr Pointer on assembly data (assembly data is copied,
	//!		this buffer can be deleted just after).
	//! @param assemblySize Size of buffer referenced by assemblyPtr
	//! @param xmlString Pointer on UTF8 Null terminated string that contains
	//!		XML description of assemblyBinary I/O bindings (data copied)
	//! @param outputOptions Optional output options. Allows to override
	//!		outputs format, mipmap, etc.
	PackageDesc(
		const void *assemblyPtr,
		size_t assemblySize,
		const char *xmlString,
		const OutputOptions& outputOptions = OutputOptions());

	//! @brief Construct from package w/ different output options
	//! @param sourcePackage Source package to copy data. Data are shared and
	//!		not deep copied. Thread safe ownership management.
	//! @param outputOptions Output options. Allows to override outputs format,
	//!		mipmap, etc.
	PackageDesc(
		const PackageDesc &sourcePackage,
		const OutputOptions& outputOptions);

	//! @brief Destructor
	//! @pre No more graph instances (created from graph descriptions contained
	//!		in this package) still alive.
	virtual ~PackageDesc();

	//! @brief Return whether the package is valid (loading succeed) or not
	bool isValid() const { return !mGraphs.empty(); }

	//! @brief Accessor on Unique ID of this package/assembly
	//! Assembly UID are renew at each compilation
	UInt getUid() const { return mUid; }

	//! @brief Accessor on graph descriptions contained in this package
	//! Graphs contains inputs (tweaks) and outputs (images to render).
	const Graphs& getGraphs() const { return mGraphs; }

	//! @brief Accessor on package level XMP metadata packet in XML encoding
	const string& getXMP() const { return mXMP; }

	//! @brief Advanced usage: accessor on assembly data (.sbsasm)
	//! @return Return pointer on assembly data or nullptr if not valid or stacking
	//!		package.
	const string* getAssemblyData() const;

	//! @brief Advanced usage: accessor on I/O assembly bindings XML description
	//! @return Return UTF8 string. Empty if not valid or stacking package.
	const string& getXmlString() const;

	//! @brief Accessor on extra resources stored in the package
	//! @param filename     C-string, STL char-string or u16-string with the name of the file to retrieve
	//!                     If it starts with a sbsar:// prefix, the prefix will be ignored
	//! @param extraContent A reference to a vector of bytes that is filled with the file content
	//!                     The vector stays untouched if the file cannot be found in the package
	//! @return Returns true if the file is present in the package, false otherwise
	bool getExtraResource(const char*      filename, vector<uint8_t>& extraContent) const;
	bool getExtraResource(const string&    filename, vector<uint8_t>& extraContent) const;
	bool getExtraResource(const u16string& filename, vector<uint8_t>& extraContent) const;

	//! @brief Accessor on the SBSASM version number
	UInt getSBSASMVersionNumber() const { return mLastSBSASMVersion; }

	//! @brief Internal use only
	shared_ptr<Details::LinkData> getLinkData() const { return mLinkData; }

	//! @brief Used to store user data
	size_t mUserData;

protected:
	UInt mUid;
	Graphs mGraphs;
	string mXmlString;
	shared_ptr<Details::LinkData> mLinkData;
	size_t mInstancesCount;
	string mXMP;
	UInt mLastSBSASMVersion;

	map<u16string, GraphDesc::Thumbnail> mThumbnailMap;
	map<u16string, vector<uint8_t> > mExtraMap; // Unrecognized files

	PackageDesc();
	void init(const OutputOptions& outputOptions);
	static void fixFormat(int&,unsigned int);
	bool parseXml(const char *xml);

	friend class GraphInstance;
private:
	PackageDesc(const PackageDesc&);
	const PackageDesc& operator=(const PackageDesc&);
};


//! @brief Helper: Instantiate graphs from a package description
//! @param[out] instances Instances of all graphs of the package
//! @param packageDesc The package description which contains graphs to
//!		instantiate.
//! @invariant The package description 'packageDesc' must remain valid during
//! 	all instances lifetimes.
//!
//! Graphs instances are built from graph descriptions. Each graph instance
//! is independent and can be used to render outputs with its own input values.
void instantiate(GraphInstances& instances,const PackageDesc& packageDesc);


} // namespace SubstanceAir

#endif //_SUBSTANCE_AIR_FRAMEWORK_PACKAGE_H
