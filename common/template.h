#ifndef _template_h_
#define _template_h_
#pragma once

#include "common/config.h"
#include "common/allocator/egal_allocator.h"
#include "common/stl/tmap.h"

#if EGAL_USER_STL == 1
 
 #include "common/stl/tqueue.h"
 #include "common/stl/tvector.h"

#else

 #include <queue>
 #include <map>
 #include <vector>

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
	/** stl */
	// TVector container
#if EGAL_USER_STL == 0
	template <typename T> class TVector : public std::vector<T>
	{
	public:
		TVector(IAllocator& alloc)
		{

		}
	};

	//template <typename _K, template _V> class TMap : public std::map<_K, _V>
	//{
	//	TMap(IAllocator& alloc)
	//	{

	//	}
	//};

	template <typename T> class TQueue : public std::queue<T>
	{
	public:
		TQueue(IAllocator& alloc)
		{

		}
	};

#endif

	// list container
	template <typename T>
	class TList	: public std::list<T> { };

	//---------------------------------------------------------------------------
	//---------------------------------------------------------------------------
	//---------------------------------------------------------------------------
	template <typename T, e_int32 count> e_int32 TlengthOf(const T(&)[count])
	{
		return count;
	};








}

#endif
