#ifndef _job_system_h_
#define _job_system_h_
#pragma once

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/egal_string.h"

namespace egal
{
	namespace JobSystem
	{
		struct JobDecl
		{
			e_void(*task)(e_void*);
			e_void* data;
		};

		bool init(IAllocator& allocator);
		e_void shutdown();
		e_void runJobs(const JobDecl* jobs, e_int32 count, e_int32 volatile* counter);
		e_void wait(e_int32 volatile* counter);

		struct LambdaJob : JobDecl
		{
			LambdaJob()
			{ 
				StringUnitl::setMemory(pool, 0, 64);
				data		= pool;
				allocator	= NULL;
			}
			~LambdaJob() 
			{ 
				if (data != pool && allocator)
					allocator->deallocate(data); 
			}
			
			byte		pool[64];
			IAllocator* allocator;
		};

		template <typename T> 
		e_void lambdaInvoker(e_void* data)
		{
			(*(T*)data)();
		}

		/**ÄäÃûº¯Êý*/
		template<typename T>
		e_void fromLambda(T lambda, LambdaJob* job, JobDecl* job_decl, IAllocator* allocator)
		{
			job->allocator = allocator;
			if (sizeof(lambda) <= sizeof(job->pool))
			{
				job->data = job->pool;
			}
			else if(allocator)
			{
				ASSERT(allocator);
				job->data = allocator->allocate(sizeof(T));
			}
			else
			{
				job->data = nullptr;
			}

			_new(job->data) T(lambda);
			job->task = &lambdaInvoker<T>;
			*job_decl = *job;
		}
	}
}

#endif
