#ifndef _egal_type_h_
#define _egal_type_h_
#pragma once
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <list>
#include <set>
#include <algorithm>
#include <assert.h>

#include "common/config.h"
#include "common/platform.h"
#include "common/define.h"

#if PLATFORM_WINDOWS
	#include <SDKDDKVer.h>
	#include <windows.h>
	#include <stdint.h>
#endif

/**
 * B: Boolean, bEnabled
 * E:枚举, ESelectionMode 
 * I: 接口类，ITransaction 
 * T：模板，例如TArray,TMap,TQueue 
 * F: 其他的类和结构，例如FName, FVector
*/

namespace egal
{
	class egal_string;
//********************************************************************
	typedef void					e_void;

	typedef char					e_int8;
	typedef unsigned char			e_uint8;
	typedef short					e_int16;
	typedef unsigned short			e_uint16;

	typedef int						e_int32;  //如果报这个e_int32 类型出错 搜索 "unsigned e_int32" 替换为 "e_uint32"
	typedef unsigned int			e_uint32;

	typedef unsigned long long		e_uint64;
	typedef long long				e_int64;

	typedef unsigned short			e_word;

	typedef bool					e_bool;

	typedef unsigned char			e_byte;
	typedef char					e_char;

	typedef float					e_float;
	typedef double					e_double;

	typedef wchar_t					e_wchar;

	typedef egal_string				String;

	typedef e_uint64				ObjectGUID;
	typedef e_uint64				ObjectTypeGUID;
	typedef e_uint64				ObjectPropertyUID;
	typedef e_uint64				ObjectListUID;

#ifdef PLATFORM_64
	typedef e_uint64				uintptr;
#else
	typedef e_uint32				uintptr;
#endif
}

#endif
