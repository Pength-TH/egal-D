#ifndef _egal_type_h_
#define _egal_type_h_
#pragma once

#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <assert.h>

namespace egal
{
//********************************************************************
	typedef void					e_void;
	typedef int						e_int32;
	typedef unsigned int			e_uint32;
	typedef short					e_int16;
	typedef unsigned short			e_uint16;
	typedef long					e_long;
	typedef unsigned long			e_ulong;
	typedef long long				e_int64;
	typedef unsigned short			e_word;
	typedef int						e_bool;
	typedef unsigned char			e_byte;
	typedef char					e_char;
	typedef float					e_float;
	typedef wchar_t					e_wchar;

//********************************************************************
	
	typedef unsigned long long  ObjectGUID;
	typedef unsigned long		ObjectTypeGUID;
	typedef unsigned long		ObjectPropertyUID;
	typedef unsigned long		ObjectListUID;

//*********************************************************************

	/** stl */
	// vector container
	template <class T> struct vector			: public std::vector<T> { };

	// list container
	template <class T> struct list				: public std::list<T> { };

	// map container
	template <class T, class M> struct map		: public std::map<T, M, std::less<T>> { };

	//multimap
	template <class T, class M> struct multimap	: public std::multimap<T, M, std::less<T>> { };

//*********************************************************************

	struct FrameTime
	{
		// Gets total simulation time in seconds.
		double TotalTime;

		// Gets elapsed time in seconds. since last update.
		float ElapsedTime;
	};

	namespace UpdateType
	{
		enum Enum
		{
			Editing,
			GamePlay,
			Paused,
		};
	}
	typedef UpdateType::Enum UpdateTypeEnum;

//*********************************************************************
	
}

#endif
