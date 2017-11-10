#ifndef _egal_type_h_
#define _egal_type_h_
#pragma once

#include "egal_platform.h"
#include "egal_alloc.h"

namespace egal
{
	typedef unsigned int		uint32;
	typedef unsigned short		uint16;
	typedef unsigned char		uint8;
	typedef unsigned int		uint;
	typedef unsigned short		ushort;
	typedef int					int32;
	typedef short				int16;
	typedef unsigned char		uchar;

	typedef float				Real;

	typedef std::string         String;


	/** stl */
	// vector container
	template <class T> struct vector		: public std::vector<T> { };

	// list container
	template <class T> struct list			: public std::list<T> { };

	// map container
	template <class T, class M> struct map	: public std::map<T, M, std::less<T>> { };

	//multimap
	template <class T> struct multimap		: public std::multimap<T> { };
	
}

#endif
