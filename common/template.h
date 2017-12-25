#ifndef _template_h_
#define _template_h_
#pragma once

#include "common/config.h"

#if EGAL_USE_STDSTL

#include "common/stl/delegate.h"
#include "common/stl/hash_map.h"
#include "common/stl/map.h"
#include "common/stl/queue.h"
#include "common/stl/vector.h"

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
	template <typename T> class TVector : public std::vector<T> { };
	template <typename T, template V> class TMap : public std::map<T, V> { };
	template <typename T> class TQueue : public std::queue<T> { };
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
