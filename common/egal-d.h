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

/**math*/
#include "common/math/egal_math.h"

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

/**stl*/
#include "common/stl/delegate.h"
#include "common/stl/hash_map.h"
#include "common/stl/map.h"
#include "common/stl/queue.h"
#include "common/stl/vector.h"

/**debug*/
#include "common/debug/debug.h"
#include "common/debug/profiler.h"

/**serializer*/
#include "common/serializer/serializer.h"
#include "common/serializer/json_serializer.h"

/**lua*/
#include <lua.hpp>
#include <lauxlib.h>

/**framework*/
#include "common/framework/renderer.h"
#include "common/framework/buffer_generator.h"

#endif
