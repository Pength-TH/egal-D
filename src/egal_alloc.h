
#ifndef _egal_alloc_h_
#define _egal_alloc_h_
#pragma once

namespace egal
{

#if _DEBUG

	#define new_t(t, ca) new
	#define delete_t(ptr, T, ca) delete
	#define array_t(t, count, ca) new t[count()]
	#define delete_array_t(ptr, t, count, ca) delete[] ptr; ptr = 0;

	#define alloc_t(t, count, ca)
	#define free_t(ptr, ca)

	#define _new new
	#define _delete delete

#else

	#define new_t(t, ca) new
	#define delete_t(ptr, T, ca) delete
	#define array_t(t, count, ca) new t[count()]
	#define delete_array_t(ptr, t, count, ca) delete[] ptr; ptr = 0;

	#define alloc_t(t, count, ca)
	#define free_t(ptr, ca)

	#define _new new
	#define _delete delete

#endif

}

#endif
