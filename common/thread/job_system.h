#ifndef _job_system_h_
#define _job_system_h_
#pragma once

#include "common/type.h"
#include "common/allocator/allocator.h"

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
			LambdaJob() { data = pool; }
			~LambdaJob() { if (data != pool) allocator->deallocate(data); }
			e_uint8 pool[64];
			IAllocator* allocator;
		};

		template <typename T> e_void lambdaInvoker(e_void* data)
		{
			(*(T*)data)();
		}

		template<typename T>
		e_void fromLambda(T lambda, LambdaJob* job, JobDecl* job_decl, IAllocator* allocator)
		{
			job->allocator = allocator;
			if (sizeof(lambda) <= sizeof(job->pool))
			{
				job->data = job->pool;
			}
			else
			{
				ASSERT(allocator);
				job->data = allocator->allocate(sizeof(T));
			}
			_new(job->data) T(lambda);
			job->task = &lambdaInvoker<T>;
			*job_decl = *job;
		}
	}
}

#endif
