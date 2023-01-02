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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSUTILS_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSUTILS_H

#include <substance/framework/typedefs.h>

#include <stdlib.h>
#include <assert.h>


namespace SubstanceAir
{
namespace Details
{
namespace Utils
{


//! @brief Checked delete function (from boost::checked_delete)
template<class T>
inline void checkedDelete(T *p)
{
    typedef char dummytype[ sizeof(T)? 1: -1 ];
    (void)sizeof(dummytype);
    AIR_DELETE(p);
}


//! @brief Delete functor
template <class T>
struct Deleter
{
    typedef void result_type;
    typedef T * argument_type;
    void operator()(T *p) const
	{
		checkedDelete(p);
	}
	void operator()(const T *p) const
	{
		checkedDelete(const_cast<T*>(p));
	}
};


//! @brief Vector of sparse allocated shared pointer
//! Each push items are identified w/ its index
template <class ValueType>
class SparseVectorPtr
{
public:
	typedef shared_ptr<ValueType> PtrType;
	typedef vector<PtrType> Container;

	SparseVectorPtr() : mFirstFreeIndex(0) {}
	const PtrType& operator[](size_t i) const { return mContainer.at(i); }

	//! @brief Push new element
	//! @return Allocate an return corresponding index
	size_t push(const PtrType& ptr)
	{
		const size_t posres = mFirstFreeIndex;
		if (mFirstFreeIndex>=mContainer.size())
		{
			assert(mFirstFreeIndex==mContainer.size());
			++mFirstFreeIndex;
			mContainer.push_back(ptr);
		}
		else
		{
			assert(mContainer[mFirstFreeIndex].get()==nullptr);
			mContainer[mFirstFreeIndex] = ptr;

			// Find next free cell
			do
			{
				++mFirstFreeIndex;
			}
			while (mFirstFreeIndex<mContainer.size() &&
				mContainer[mFirstFreeIndex].get()!=nullptr);
		}

		return posres;
	}

	//! @brief Remove item from its index (must be valid)
	void remove(size_t index)
	{
		assert(mContainer[index].get()!=nullptr);
		mContainer[index].reset();
		mFirstFreeIndex = std::min<size_t>(mFirstFreeIndex,index);
	}

	void reserve(size_t size) { mContainer.reserve(size); }

protected:
	Container mContainer;
	size_t mFirstFreeIndex;
};


//! @brief Convert char container to lowercase
template<class T>
inline void toLower(T &c)
{
    for (typename T::iterator ite = c.begin();ite!=c.end();++ite)
	{
		if (*ite>='A' && *ite<='Z')
		{
			*ite += 'a'-'A';
		}
	}
}

//! @brief Return log2(x), integer version,
//! @pre x must be a 2^x number, non null
inline UInt log2(UInt size)
{
	UInt res = 0;
	size=size>>1;
	for (;size;size=size>>1,++res) ;
	return res;
}


} // namespace Utils
} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSUTILS_H
