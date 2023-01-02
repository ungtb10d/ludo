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

#include <substance/framework/callbacks.h>
#include <substance/framework/typedefs.h>

namespace SubstanceAir
{
	void* alignedMalloc(size_t bytesCount, size_t alignment)
	{
		return SubstanceAir::GlobalCallbacks::getInstance()->memoryAlloc(bytesCount, alignment);
	}

	void alignedFree(void* bufferPtr)
	{
		SubstanceAir::GlobalCallbacks::getInstance()->memoryFree(bufferPtr);
	}
}

#if defined(AIR_OVERRIDE_GLOBAL_NEW)
void* operator new (size_t size) 
{ 
	return SubstanceAir::alignedMalloc(size, AIR_DEFAULT_ALIGNMENT); 
}

void* operator new(size_t size, std::nothrow_t&) 
{ 
	return SubstanceAir::alignedMalloc(size, AIR_DEFAULT_ALIGNMENT);  
}

void* operator new [](size_t size) 
{ 
	return SubstanceAir::alignedMalloc(size, AIR_DEFAULT_ALIGNMENT);  
}

void* operator new [](size_t size, std::nothrow_t&) 
{ 
	return SubstanceAir::alignedMalloc(size, AIR_DEFAULT_ALIGNMENT);  
}

void operator delete(void* ptr) 
{ 
	SubstanceAir::alignedFree(ptr); 
}

void operator delete(void* ptr, std::nothrow_t&) 
{ 
	SubstanceAir::alignedFree(ptr); 
}

void operator delete [](void* ptr) 
{ 
	SubstanceAir::alignedFree(ptr); 
}

void operator delete [](void* ptr, std::nothrow_t&) 
{ 
	SubstanceAir::alignedFree(ptr); 
}
#endif
