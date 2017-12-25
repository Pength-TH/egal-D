#ifndef _SINGLETON_H__
#define _SINGLETON_H__

#include "common/struct.h"
namespace egal
{
	template <typename T> class Singleton : public NoneCopyable
	{
	protected:
		static T* msSingleton;

	public:
		Singleton(void)
		{
			assert(!msSingleton);
#if defined( _MSC_VER ) && _MSC_VER < 1200   
			int offset = (int)(T*)1 - (int)(Singleton <T>*)(T*)1;
			msSingleton = (T*)((int)this + offset);
#else
			msSingleton = static_cast<T*>(this);
#endif
		}
		~Singleton(void)
		{
			assert(msSingleton);
			msSingleton = 0;
		}
		/// Get the singleton instance
		static T& getSingleton(void)
		{
			assert(msSingleton);
			return (*msSingleton);
		}
		/// @copydoc getSingleton
		static T* getSingletonPtr(void)
		{
			return msSingleton;
		}
	};
}

#endif
