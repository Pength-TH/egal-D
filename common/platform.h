#ifndef _platform_h_
#define _platform_h_
#pragma once

/**  egal-d engine
* support system:windows£¬ios£¬OSX, Android£¬html5
* support script: LUA
* support complier: MSVC£¬ GCC£¬ CLANG
* support CPU: x86 64
*/
#include "common/config.h"

// 0:MSVC 1:CLANG 2:GCC
#if EGAL_COMPILER == 0
	#define EGAL_COMPILER_MSVC 1
#elif EGAL_COMPILER == 0
	#define EGAL_COMPILER_CLANG 1
#elif EGAL_COMPILER == 0
	#define EGAL_COMPILER_GCC 1
#endif

// 0:WINDOWS 1:ANDROID 2:IOS 3:OSX  4:Html5
#if EGAL_PLATFORM == 0
	#define EGAL_PLATFORM_WINDOWS 1
#elif EGAL_PLATFORM == 1
	#define EGAL_PLATFORM_ANDROID 1
#elif EGAL_PLATFORM == 2
	#define EGAL_PLATFORM_IOS 1
#elif EGAL_PLATFORM == 3
	#define EGAL_PLATFORM_OSX 1
#elif EGAL_PLATFORM == 4
	#define EGAL_PLATFORM_HTML5 1
#endif

// 0:64 1:32
#if EGAL_CPU == 0
	#define EGAL_CPU_X64  1
#elif EGAL_CPU == 1
	#define EGAL_CPU_X86  1
#endif

#if EGAL_PLATFORM_ANDROID
	#define EGAL_PLATFORM_NAME "android"
#elif EGAL_PLATFORM_IOS
	#define EGAL_PLATFORM_NAME "ios"
#elif EGAL_PLATFORM_OSX
	#define EGAL_PLATFORM_NAME "osx"
#elif EGAL_PLATFORM_WINDOWS
	#define EGAL_PLATFORM_NAME "windows"
#endif // EGAL_PLATFORM_

#if EGAL_CPU_X86
	#define EGAL_CPU_NAME "x86"
#elif EGAL_CPU_X64
	#define EGAL_CPU_NAME "x64"
#endif // EGAL_CPU_

#if EGAL_PLATFORM_WINDOWS
	#define PLATFORM_WINDOWS 1
#elif EGAL_PLATFORM_IOS
	#define PLATFORM_APPLE 1
#elif EGAL_PLATFORM_ANDROID
	#define PLATFORM_ANDROID 1
#endif

#if EGAL_CPU_X64
	#define PLATFORM_64
#elif EGAL_CPU_X86
	#define PLATFORM_86
#endif

#endif