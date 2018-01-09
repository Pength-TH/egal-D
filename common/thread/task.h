#ifndef _task_h_
#define _task_h_

#include "common/type.h"
#include "common/allocator/egal_allocator.h"

namespace egal
{
	namespace MT
	{
		class Task
		{
		public:
			explicit Task(IAllocator& allocator);
			virtual ~Task();

			virtual e_int32 task() = 0;

			e_bool create(const e_char* name);
			e_bool destroy();

			e_void setAffinityMask(e_uint64 affinity_mask);

			e_uint64 getAffinityMask() const;

			e_bool isRunning() const;
			e_bool isFinished() const;
			e_bool isForceExit() const;

			e_void forceExit(e_bool wait);

		protected:
			IAllocator& getAllocator();

		private:
			struct TaskImpl* m_implementation;
		};
	}
}

#endif
