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

//! Use C++11 sync if available (on MSVC compilers, set AIR_USE_CPP11_SYNC
//! on GCC when available), otherwise use Win32 thread or Pthread,
//! use boost::thread as fallback.
//!
//! By default Win32 implementation uses Condition Variable that are not
//! supported on WindowsXP. To force support of this OS, use 
//!	AIR_USE_WIN32_SEMAPHORE define (not recommended, may introduce 
//! synchronization issues).

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSYNC_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSYNC_H

#include <substance/framework/typedefs.h>

// If not already defined, select thread impl.
#if !defined(AIR_USE_WIN32_SYNC)   && \
    !defined(AIR_USE_PTHREAD_SYNC) && \
    !defined(AIR_USE_BOOST_SYNC)   && \
    !defined(AIR_USE_CPP11_SYNC)   && \
    !defined(AIR_SYNC_DISABLED)

	#if defined(__has_include)
		// Detect C++11 sync if __has_include is available (Clang)
		#if __has_include(<thread>) && \
				__has_include(<mutex>) && \
				__has_include(<condition_variable>)
			#define AIR_HAS_STD_SYNC_HEADERS 1
		#endif
	#endif

	#if defined(_MSC_VER)
		#if _MSC_VER>=1700 // At least MSVC 2012
			#define AIR_USE_CPP11_SYNC 1
		#else
			#define AIR_USE_WIN32_SYNC 1
		#endif
	//force the use of pthreads over C++ threads on linux
	#elif defined(__linux__) && !defined(__ANDROID__)
		#define AIR_USE_PTHREAD_SYNC 1
	#elif defined(AIR_HAS_STD_SYNC_HEADERS)
		#define AIR_USE_CPP11_SYNC 1
	#elif defined(__GNUC__)
		#define AIR_USE_PTHREAD_SYNC 1
	#else
		#define AIR_USE_BOOST_SYNC 1
	#endif
#endif

//force setting stack size on certain platforms
#if !defined(AIR_SYNC_USE_STACK_SIZE)
	#if defined(__ORBIS__)
		#define AIR_SYNC_USE_STACK_SIZE
	#endif
#endif

//assign a default worker thread stack size if needed (win32/pthread implementations only)
#if defined(AIR_SYNC_USE_STACK_SIZE)
	#if !defined(AIR_SYNC_STACK_SIZE)
		#define AIR_SYNC_STACK_SIZE 1024*1024
	#endif
#endif

#if defined(AIR_USE_CPP11_SYNC) || defined(ALG_USE_BOOST_SYNC)
	
	#if defined(AIR_USE_CPP11_SYNC)
		#include <thread>
		#include <mutex>
		#include <condition_variable>
		#define AIR_SYNC_NAMESPACE std
	#elif defined(ALG_USE_BOOST_SYNC)
		#include <boost/thread.hpp>
		#define AIR_SYNC_NAMESPACE boost
	#endif

	namespace SubstanceAir
	{
	namespace Details
	{
	namespace Sync
	{

		// Use boost::thread
		typedef AIR_SYNC_NAMESPACE::mutex mutex;
		typedef AIR_SYNC_NAMESPACE::unique_lock<mutex> unique_lock;
		typedef AIR_SYNC_NAMESPACE::thread thread;
		typedef AIR_SYNC_NAMESPACE::condition_variable condition_variable;

	} // namespace Sync
	} // namespace Details
	} // namespace SubstanceAir

#else

	// PTHREAD & WIN32 & Disabled specific implementation

	#if defined(AIR_USE_WIN32_SYNC)

		// Win32 threads implementation
		#include <windows.h>
		#define AIR_SYNC_THREADQUALIFIER DWORD WINAPI

		typedef HANDLE AirSyncThread;      //!< Thread type
		#if defined(AIR_USE_WIN32_SEMAPHORE)
			typedef HANDLE AirSyncMutex;       //!< Mutex type
			typedef HANDLE AirSyncCond;        //!< Conditional var. type
		#else //if defined(AIR_USE_WIN32_SEMAPHORE)
			typedef CRITICAL_SECTION AirSyncMutex;    //!< Mutex type
			typedef CONDITION_VARIABLE AirSyncCond;    //!< Conditional var. type
		#endif //if defined(AIR_USE_WIN32_SEMAPHORE)

	#elif defined(AIR_USE_PTHREAD_SYNC)

		// PThread implementation
		#include <sys/types.h>
		#include <pthread.h>
	
		#define AIR_SYNC_THREADQUALIFIER void*

		typedef pthread_mutex_t AirSyncMutex; //!< Mutex type
		typedef pthread_t AirSyncThread;      //!< Thread type
		typedef pthread_cond_t AirSyncCond;   //!< Conditional var. type
	
	#else 
	
		// Disabled
		#define AIR_SYNC_THREADQUALIFIER void*

		struct AirSyncMutex {};
		struct AirSyncThread {};
		struct AirSyncCond {};
	
	#endif

	namespace SubstanceAir
	{
	namespace Details
	{
	namespace Sync
	{

	//! @brief See std|boost::thread for documentation
	class thread
	{
	public:
		thread();
		~thread();
		thread(thread&&);
		thread(const thread&) = delete;

		//! @brief Start a thread executing the given functor
		//! @param f - Function to execute
		//! @param args - Arguments to pass to function
		template<class Function, class... Args>
		explicit thread(Function&& f, Args&&... args)
		{
			auto threadInstance = make_unique<ThreadFunctor<Function,Args...>>(std::forward<Function>(f), std::forward<Args>(args)...);

			mThreadFunctor = static_unique_ptr_cast<IThreadFunctor>(std::move(threadInstance));
			start();
		}
	
		//! @brief Move thread internals from one thread object to another
		thread& operator=(thread&&);

		//! @brief Wait for thread to complete execution and then continue execution on calling thread
		void join();

		//! @brief Determine if thread is active and therefore can be joined
		//! @returns bool - True if thread is joinable, false otherwise
		bool joinable();

	protected:
		//! @brief Begin Thread Execution
		void start();

	protected:
		//! @brief Thread Functor Interface Class
		class IThreadFunctor
		{
		public:
			virtual ~IThreadFunctor() {}

			//! @brief Execute thread functor (implemented in specialized class)
			virtual void run() = 0;
		};

		//! @brief Thread Functor Template Implementation
		template<class Function, class... Args>
		class ThreadFunctor : public IThreadFunctor
		{
		public:
			//! @brief Construct thread instance with a given function and argument list
			ThreadFunctor(Function&& func, Args&&... args)
				: mFunction(func)
				, mArgs(std::make_tuple(std::forward<Args>(args)...))
			{
			}

			//! @brief Execute thread functor with appropriate arguments
			virtual void run() override
			{
				SubstanceAir::apply(mFunction, std::move(mArgs));
			}

		private:
			Function			mFunction;
			tuple<Args...>		mArgs;
		};

	protected:
		AirSyncThread				mThread;
		unique_ptr<IThreadFunctor>	mThreadFunctor;
	};


	//! @brief See std|boost::thread for documentation
	class mutex
	{
	public:
		mutex();
		mutex(const mutex&) = delete;
		const mutex& operator=(const mutex&) = delete;

		~mutex();

		void lock();
		bool try_lock();
		void unlock();
	protected:
		AirSyncMutex mMutex;
		friend class condition_variable;
	};


	//! @brief See std|boost::unique_lock for documentation
	class unique_lock
	{
	public:
		unique_lock(mutex& m) : mMutex(m) { mMutex.lock(); }
		unique_lock(const unique_lock&) = delete;
		const unique_lock& operator=(const unique_lock&) = delete;

		~unique_lock() { mMutex.unlock(); }
	protected:
		mutex &mMutex;
		friend class condition_variable;
	};


	//! @brief See std|boost::thread for documentation
	class condition_variable
	{
	public:
		condition_variable();
		~condition_variable();

		void notify_one();

		void wait(unique_lock&);
	
	protected:
		AirSyncCond mCond;
	};


	} // namespace Sync
	} // namespace Details
	} // namespace SubstanceAir

#endif

#endif // _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSYNC_H
