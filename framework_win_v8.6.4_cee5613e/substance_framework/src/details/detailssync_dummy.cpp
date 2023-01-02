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

#include "detailssync.h"

#ifdef AIR_SYNC_DISABLED


SubstanceAir::Details::Sync::thread::thread()
{
}


SubstanceAir::Details::Sync::thread::~thread()
{
}

SubstanceAir::Details::Sync::thread::thread(thread&& b)
{
	*this = std::move(b);
}

SubstanceAir::Details::Sync::thread& 
SubstanceAir::Details::Sync::thread::operator=(thread&& t)
{
	mThreadFunctor = std::move(t.mThreadFunctor);
	return *this;
}


void SubstanceAir::Details::Sync::thread::start()
{
	mThreadFunctor->run();
	mThreadFunctor.reset(nullptr);
}


void SubstanceAir::Details::Sync::thread::join()
{
	mThreadFunctor.reset(nullptr);
}


bool SubstanceAir::Details::Sync::thread::joinable()
{
	return (mThreadFunctor.get() != nullptr) ? true : false;
}


SubstanceAir::Details::Sync::mutex::mutex()
{
}


SubstanceAir::Details::Sync::mutex::~mutex()
{
}


void SubstanceAir::Details::Sync::mutex::lock()
{
}


bool SubstanceAir::Details::Sync::mutex::try_lock()
{
	return true;
}


void SubstanceAir::Details::Sync::mutex::unlock()
{
}


SubstanceAir::Details::Sync::condition_variable::condition_variable()
{
}


SubstanceAir::Details::Sync::condition_variable::~condition_variable()
{
}


void SubstanceAir::Details::Sync::condition_variable::notify_one()
{
}


void SubstanceAir::Details::Sync::condition_variable::wait(unique_lock& l)
{
	(void)l;
}

#endif  // ifdef AIR_SYNC_DISABLED
