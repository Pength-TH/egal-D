#ifndef _egal_h_
#define _egal_h_
#pragma once

/**
* egal-d engine core include
*/
#include "common/config.h"

/**base*/
#include "common/type.h"
#include "common/define.h"
#include "common/platform.h"
#include "common/struct.h"
#include "common/template.h"
#include "common/egal_string.h"

/**allocal*/
#include "common/allocator/egal_allocator.h"

/**math*/
#include "common/math/egal_math.h"

/**stl*/
#if EGAL_USER_STL == 1

#include "common/stl/tmap.h"
#include "common/stl/tqueue.h"
#include "common/stl/tvector.h"

#endif

#include "common/stl/delegate.h"
#include "common/stl/delegate_list.h"
#include "common/stl/thash_map.h"

/**utils*/
#include "common/utils/crc32.h"
#include "common/utils/geometry.h"
#include "common/utils/logger.h"
#include "common/utils/perf_timer.h"
#include "common/utils/sharedptr.h"
#include "common/utils/singleton.h"

/**file_system*/
#include "common/filesystem/ifile_device.h"
#include "common/filesystem/binary.h"

/**thread*/
#include "common/thread/job_system.h"

/**debug*/
#include "common/debug/debug.h"
#include "common/debug/profiler.h"

/**serializer*/
#include "common/serializer/serializer.h"
#include "common/serializer/json_serializer.h"

/**input*/
#include "common/input/reflection.h"
#include "common/input/input_system.h"

/**framework*/
#include "runtime/EngineFramework/renderer.h"
#include "runtime/EngineFramework/buffer.h"

/**const*/
#include "common/egal_const.h"

/**lua*/
#include <lua.hpp>
#include <lauxlib.h>



#endif
