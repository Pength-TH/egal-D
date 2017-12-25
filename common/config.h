#ifndef _config_h_
#define _config_h_
#pragma once

#define MAX_LOG_LENGTH  2048
#define MAX_PATH_LENGTH 1024

#define USE_DEFAULT_ALLOCATOR 1

#define EGAL_COMPILER 0		// 0:MSVC 1:CLANG 2:GCC
#define EGAL_PLATFORM 0		// 0:WINDOWS 1:ANDROID 2:IOS 3:OSX 
#define EGAL_CPU	  0		// 0:64 1:32

#define EGAL_USER_STL 1		// 0:STD 1:egal stl

#define EGAL_MATH 0         // 0:default 1:mathfu

#endif
