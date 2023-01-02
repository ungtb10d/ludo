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

#ifdef AIR_USE_PTHREAD_SYNC

#include <errno.h>


SubstanceAir::Details::Sync::thread::thread()
{
}


SubstanceAir::Details::Sync::thread::~thread()
{
	join();
}

SubstanceAir::Details::Sync::thread::thread(thread&& b)
{
    *this = std::move(b);
}

void SubstanceAir::Details::Sync::thread::join()
{
	if (mThreadFunctor.get())
	{
		pthread_join(mThread,nullptr);
		mThread = 0;
		mThreadFunctor.reset(nullptr);
	}
}

SubstanceAir::Details::Sync::thread& 
SubstanceAir::Details::Sync::thread::operator=(thread&& b)
{
    join();

    mThread = b.mThread;
    b.mThread = 0;

    mThreadFunctor = std::move(b.mThreadFunctor);

    return *this;
}


void SubstanceAir::Details::Sync::thread::start()
{
    auto routine = [](void* arg) -> void* {
        IThreadFunctor* functor = reinterpret_cast<IThreadFunctor*>(arg);
        functor->run();
        return nullptr;
    };

#if defined (AIR_SYNC_USE_STACK_SIZE)
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, AIR_SYNC_STACK_SIZE);
	pthread_create(&mThread,&attr,routine,mThreadFunctor.get());
	pthread_attr_destroy(&attr);
#else
	pthread_create(&mThread,nullptr,routine,mThreadFunctor.get());
#endif
}


bool SubstanceAir::Details::Sync::thread::joinable()
{
	return (mThreadFunctor.get()) ? true : false;
}


SubstanceAir::Details::Sync::mutex::mutex()
{
	pthread_mutex_init(&mMutex,nullptr);
}


SubstanceAir::Details::Sync::mutex::~mutex()
{
	pthread_mutex_destroy(&mMutex);
}


void SubstanceAir::Details::Sync::mutex::lock()
{
	pthread_mutex_lock(&mMutex);
}


bool SubstanceAir::Details::Sync::mutex::try_lock()
{
	return pthread_mutex_trylock(&mMutex)!=EBUSY;
}


void SubstanceAir::Details::Sync::mutex::unlock()
{
	pthread_mutex_unlock(&mMutex);
}


SubstanceAir::Details::Sync::condition_variable::condition_variable()
{
	pthread_cond_init(&mCond,nullptr);
}


SubstanceAir::Details::Sync::condition_variable::~condition_variable()
{
	pthread_cond_destroy(&mCond);
}


void SubstanceAir::Details::Sync::condition_variable::notify_one()
{
	pthread_cond_signal(&mCond);
}


void SubstanceAir::Details::Sync::condition_variable::wait(unique_lock& l)
{
	pthread_cond_wait(&mCond,&l.mMutex.mMutex);
}

#endif  // ifdef AIR_USE_PTHREAD_SYNC
