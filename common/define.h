#ifndef _define_h_
#define _define_h_
#pragma once

#ifdef _WIN32

#if defined(min) || defined(max)
#pragma message ( "set NOMINMAX as preprocessor macro, undefining min/max " )
#undef min
#undef max
#endif

#define ALIGN_BEGIN(_align) __declspec(align(_align))
#define ALIGN_END(_align)

#else
#define ALIGN_BEGIN(_align)
#define ALIGN_END(_align) __attribute__( (aligned(_align) ) )
#endif



#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef SAFE_ADDREF
#define SAFE_ADDREF(p)      { if (p) { (p)->addRef(); } }
#endif

#ifndef ASSERT
	#ifdef NDEBUG
		#define ASSERT(x) { false ? (void)(x) : (void)0; }
	#elif  _WIN32
		#define ASSERT(s) assert(s)
	#endif
#endif

#ifndef INLINE
	#define INLINE inline
#endif //INLINE

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_arr_) (sizeof(_arr_)/sizeof((_arr_)[0]))
#endif


#endif