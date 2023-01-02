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

#ifdef AIR_USE_WIN32_SYNC

SubstanceAir::Details::Sync::thread::thread() 
	: mThread(INVALID_HANDLE_VALUE)
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
	if (mThread != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(mThread,INFINITE);
		CloseHandle(mThread);
		mThread = INVALID_HANDLE_VALUE;
		mThreadFunctor.reset(nullptr);
	}
}

SubstanceAir::Details::Sync::thread& 
SubstanceAir::Details::Sync::thread::operator=(thread&& b)
{
	join();

	mThread = b.mThread;
	b.mThread = INVALID_HANDLE_VALUE;

	mThreadFunctor = std::move(b.mThreadFunctor);

	return *this;
}


void SubstanceAir::Details::Sync::thread::start()
{
#if defined (AIR_SYNC_USE_STACK_SIZE)
	SIZE_T stackSize = AIR_SYNC_STACK_SIZE;
#else
	SIZE_T stackSize = 0;
#endif

	auto routine = [](LPVOID lpThreadParameter) -> DWORD {
		IThreadFunctor* functor = reinterpret_cast<IThreadFunctor*>(lpThreadParameter);
		functor->run();
		return 0;
	};

	mThread = CreateThread(
		nullptr,
		stackSize,
		static_cast<LPTHREAD_START_ROUTINE>(routine),
		mThreadFunctor.get(),
		0,
		nullptr);
}

bool SubstanceAir::Details::Sync::thread::joinable()
{
	return mThread != INVALID_HANDLE_VALUE;
}



#if defined(AIR_USE_WIN32_SEMAPHORE)

SubstanceAir::Details::Sync::mutex::mutex() :
	mMutex(CreateMutex(nullptr,0,nullptr))
{
}


SubstanceAir::Details::Sync::mutex::~mutex()
{
	CloseHandle(mMutex);
}


void SubstanceAir::Details::Sync::mutex::lock()
{
	WaitForSingleObject(mMutex,INFINITE);
}


bool SubstanceAir::Details::Sync::mutex::try_lock()
{
	return WaitForSingleObject(mMutex,0)!=WAIT_TIMEOUT;
}


void SubstanceAir::Details::Sync::mutex::unlock()
{
	ReleaseMutex(mMutex);
}


SubstanceAir::Details::Sync::condition_variable::condition_variable() :
	mCond(CreateSemaphore(nullptr,0,1,nullptr))
{
}


SubstanceAir::Details::Sync::condition_variable::~condition_variable()
{
	CloseHandle(mCond);
}


void SubstanceAir::Details::Sync::condition_variable::notify_one()
{
	ReleaseSemaphore(mCond,1,nullptr);
}


void SubstanceAir::Details::Sync::condition_variable::wait(unique_lock& l)
{
	SignalObjectAndWait(l.mMutex.mMutex,mCond,INFINITE,FALSE);
}


#else //if defined(AIR_USE_WIN32_SEMAPHORE)

SubstanceAir::Details::Sync::mutex::mutex()
{
	InitializeCriticalSection(&mMutex);
}


SubstanceAir::Details::Sync::mutex::~mutex()
{
	DeleteCriticalSection(&mMutex);
}


void SubstanceAir::Details::Sync::mutex::lock()
{
	EnterCriticalSection(&mMutex);
}


bool SubstanceAir::Details::Sync::mutex::try_lock()
{
	return TryEnterCriticalSection(&mMutex)!=0;
}


void SubstanceAir::Details::Sync::mutex::unlock()
{
	LeaveCriticalSection(&mMutex);
}


SubstanceAir::Details::Sync::condition_variable::condition_variable()
{
	InitializeConditionVariable(&mCond);
}


SubstanceAir::Details::Sync::condition_variable::~condition_variable()
{
	// do nothing
}


void SubstanceAir::Details::Sync::condition_variable::notify_one()
{
	WakeConditionVariable(&mCond);
}


void SubstanceAir::Details::Sync::condition_variable::wait(unique_lock& l)
{
	SleepConditionVariableCS(&mCond,&l.mMutex.mMutex,INFINITE);
}

#endif //if defined(AIR_USE_WIN32_SEMAPHORE)

#endif  // ifdef AIR_USE_WIN32_SYNC
