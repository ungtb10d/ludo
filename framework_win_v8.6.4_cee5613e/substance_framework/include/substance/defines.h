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

/*
	Defines utilities macros.
*/

#ifndef _SUBSTANCE_DEFINES_H
#define _SUBSTANCE_DEFINES_H


/* Base types defines */
#include <stddef.h>

/* C/C++ macros */
#ifndef SUBSTANCE_EXTERNC
	#ifdef __cplusplus
		#define SUBSTANCE_EXTERNC extern "C"
	#else
		#define SUBSTANCE_EXTERNC
	#endif
#endif

/* SUBSTANCE_EXPORT macros */
#ifndef SUBSTANCE_EXPORT
	#ifdef SUBSTANCE_DLL
		#if defined(_WIN32)
			#define SUBSTANCE_EXPORT SUBSTANCE_EXTERNC __declspec(dllexport)
		#elif defined(__GNUC__)
			#if __GNUC__<4
				#define SUBSTANCE_EXPORT SUBSTANCE_EXTERNC
			#else
				#define SUBSTANCE_EXPORT SUBSTANCE_EXTERNC __attribute__((visibility("default")))
			#endif
		#else
			#error Unknown compiler
		#endif
	#else
		#define SUBSTANCE_EXPORT SUBSTANCE_EXTERNC
	#endif
#endif

/* Callback functions */
#ifndef SUBSTANCE_CALLBACK
	#if defined(_WIN32)
		#define SUBSTANCE_CALLBACK __stdcall
	#else
		#define SUBSTANCE_CALLBACK
	#endif
#endif


#endif /* ifndef _SUBSTANCE_DEFINES_H */
