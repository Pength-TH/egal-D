#include "common/thread/job_system.h"

#include "common/thread/thread.h"
#include "common/thread/sync.h"
#include "common/thread/task.h"
#include "common/thread/atomic.h"
#include "common/thread/fibers.h"

#include "common/egal-d.h"

namespace egal
{
	namespace JobSystem
	{
		struct Job
		{
			JobDecl decl;
			volatile e_int32* counter;
		};


		struct FiberDecl
		{
			e_int32 idx;
			Fiber::Handle fiber;
			Job current_job;
			struct WorkerTask* worker_task;
			e_void* switch_state;
		};


		struct SleepingFiber
		{
			volatile e_int32* waiting_condition;
			FiberDecl* fiber;
		};

		struct System
		{
			System(IAllocator& allocator)
				: m_allocator(allocator)
				, m_workers(allocator)
				, m_job_queue(allocator)
				, m_sleeping_fibers(allocator)
				, m_sync(false)
				, m_work_signal(true)
				, m_event_outside_job(true)
			{
				m_event_outside_job.trigger();
				m_work_signal.reset();
			}


			MT::SpinMutex m_sync;
			MT::Event m_event_outside_job;
			MT::Event m_work_signal;
			TVector<MT::Task*> m_workers;
			TVector<Job> m_job_queue;
			FiberDecl m_fiber_pool[256];
			e_int32 m_free_fibers_indices[256];
			e_int32 m_num_free_fibers;
			TVector<SleepingFiber> m_sleeping_fibers;
			IAllocator& m_allocator;
		};

		static System* g_system = nullptr;

		static bool getReadySleepingFiber(System& system, SleepingFiber* out)
		{
			MT::SpinLock lock(system.m_sync);

			e_int32 count = system.m_sleeping_fibers.size();
			for (e_int32 i = 0; i < count; ++i)
			{
				SleepingFiber job = system.m_sleeping_fibers[i];
				if (*job.waiting_condition <= 0)
				{
					system.m_sleeping_fibers.eraseFast(i);
					*out = job;
					return true;
				}
			}
			return false;
		}

		static bool getReadyJob(System& system, Job* out)
		{
			MT::SpinLock lock(system.m_sync);

			if (system.m_job_queue.empty()) 
				return false;

			Job job = system.m_job_queue.back();
			system.m_job_queue.pop();
			if (system.m_job_queue.empty()) 
				system.m_work_signal.reset();

			*out = job;

			return true;
		}

		static thread_local MT::Task* g_worker = nullptr;

		struct WorkerTask : MT::Task
		{
			WorkerTask(System& system)
				: Task(system.m_allocator)
				, m_system(system)
			{}

			static FiberDecl& getFreeFiber()
			{
				MT::SpinLock lock(g_system->m_sync);

				ASSERT(g_system->m_num_free_fibers > 0);
				--g_system->m_num_free_fibers;
				e_int32 free_fiber_idx = g_system->m_free_fibers_indices[g_system->m_num_free_fibers];

				return g_system->m_fiber_pool[free_fiber_idx];
			}

			static e_void handleSwitch(FiberDecl& fiber)
			{
				MT::SpinLock lock(g_system->m_sync);

				if (!fiber.switch_state)
				{
					g_system->m_free_fibers_indices[g_system->m_num_free_fibers] = fiber.idx;
					++g_system->m_num_free_fibers;
					return;
				}

				volatile e_int32* counter = (volatile e_int32*)fiber.switch_state;
				SleepingFiber sleeping_fiber;
				sleeping_fiber.fiber = &fiber;
				sleeping_fiber.waiting_condition = counter;

				g_system->m_sleeping_fibers.push(sleeping_fiber);
			}

			e_int32 task() override
			{
				g_worker = this;
				Fiber::initThread(manage, &m_primary_fiber);
				return 0;
			}

#ifdef _WIN32
			static e_void __stdcall manage(e_void* data)
#else
			static e_void manage(e_void* data)
#endif
			{
				WorkerTask* that = (WorkerTask*)g_worker;
				that->m_finished = false;
				while (!that->m_finished)
				{
					SleepingFiber ready_sleeping_fiber;
					if (getReadySleepingFiber(*g_system, &ready_sleeping_fiber))
					{
						ready_sleeping_fiber.fiber->worker_task = that;
						ready_sleeping_fiber.fiber->switch_state = nullptr;
						//PROFILE_BLOCK("work");
						that->m_current_fiber = ready_sleeping_fiber.fiber;
						Fiber::switchTo(&that->m_primary_fiber, ready_sleeping_fiber.fiber->fiber);
						that->m_current_fiber = nullptr;
						ASSERT(Profiler::getCurrentBlock() == Profiler::getRootBlock(MT::getCurrentThreadID()));
						handleSwitch(*ready_sleeping_fiber.fiber);
						continue;
					}

					Job job;
					if (getReadyJob(*g_system, &job))
					{
						FiberDecl& fiber_decl = getFreeFiber();
						fiber_decl.worker_task = that;
						fiber_decl.current_job = job;
						fiber_decl.switch_state = nullptr;
						//PROFILE_BLOCK("work");
						that->m_current_fiber = &fiber_decl;
						Fiber::switchTo(&that->m_primary_fiber, fiber_decl.fiber);
						that->m_current_fiber = nullptr;
						ASSERT(Profiler::getCurrentBlock() == Profiler::getRootBlock(MT::getCurrentThreadID()));
						handleSwitch(fiber_decl);
					}
					else
					{
						//PROFILE_BLOCK("wait");
						g_system->m_work_signal.waitTimeout(1);
					}
				}
			}

			bool m_finished = false;
			FiberDecl* m_current_fiber = nullptr;
			Fiber::Handle m_primary_fiber;
			System& m_system;
		};

#ifdef _WIN32
		static e_void __stdcall fiberProc(e_void* data)
#else
		static e_void fiberProc(e_void* data)
#endif
		{
			FiberDecl* fiber_decl = (FiberDecl*)data;
			for (;;)
			{
				Job job = fiber_decl->current_job;
				job.decl.task(job.decl.data);
				if (job.counter) MT::atomicDecrement(job.counter);

				fiber_decl->switch_state = nullptr;
				Fiber::switchTo(&fiber_decl->fiber, fiber_decl->worker_task->m_primary_fiber);
			}
		}

		bool init(IAllocator& allocator)
		{
			ASSERT(!g_system);

			g_system = _aligned_new(allocator, System)(allocator);
			g_system->m_work_signal.reset();

			e_int32 count = Math::maximum(1, e_int32(MT::getCPUsCount() - 1));
			for (e_int32 i = 0; i < count; ++i)
			{
				WorkerTask* task = _aligned_new(allocator, WorkerTask)(*g_system);
				if (task->create("Job system worker"))
				{
					g_system->m_workers.push(task);
					task->setAffinityMask((e_uint64)1 << i);
				}
				else
				{
					log_error("Engine Job system worker failed to initialize.");
					_delete(allocator, task);
				}
			}

			e_int32 fiber_num = TlengthOf(g_system->m_fiber_pool);
			g_system->m_num_free_fibers = fiber_num;
			for (e_int32 i = 0; i < fiber_num; ++i)
			{
				FiberDecl& decl = g_system->m_fiber_pool[i];
				decl.fiber = Fiber::create(64 * 1024, fiberProc, &g_system->m_fiber_pool[i]);
				decl.idx = i;
				decl.worker_task = nullptr;
				g_system->m_free_fibers_indices[i] = i;
			}

			return !g_system->m_workers.empty();
		}

		e_void shutdown()
		{
			if (!g_system) return;

			IAllocator& allocator = g_system->m_allocator;
			for (MT::Task* task : g_system->m_workers)
			{
				WorkerTask* wt = (WorkerTask*)task;
				wt->m_finished = true;
			}

			for (MT::Task* task : g_system->m_workers)
			{
				while (!task->isFinished()) g_system->m_work_signal.trigger();
				task->destroy();
				_delete(allocator, task);
			}

			for (FiberDecl& fiber : g_system->m_fiber_pool)
			{
				Fiber::destroy(fiber.fiber);
			}

			_delete(allocator, g_system);
			g_system = nullptr;
		}


		e_void runJobs(const JobDecl* jobs, e_int32 count, e_int32 volatile* counter)
		{
			ASSERT(g_system);
			ASSERT(count > 0);

			MT::SpinLock lock(g_system->m_sync);
			g_system->m_work_signal.trigger();
			if (counter) MT::atomicAdd(counter, count);
			for (e_int32 i = 0; i < count; ++i)
			{
				Job job;
				job.decl = jobs[i];
				job.counter = counter;
				g_system->m_job_queue.push(job);
			}
		}

		e_void wait(e_int32 volatile* counter)
		{
			if (*counter <= 0) return;
			if (g_worker)
			{
				//ASSERT(Profiler::getCurrentBlock() == Profiler::getRootBlock(MT::getCurrentThreadID()));
				FiberDecl* fiber_decl = ((WorkerTask*)g_worker)->m_current_fiber;
				fiber_decl->switch_state = (e_void*)counter;
				Fiber::switchTo(&fiber_decl->fiber, fiber_decl->worker_task->m_primary_fiber);
			}
			else
			{
				//PROFILE_BLOCK("not a job waiting");

				//ASSERT(g_system->m_event_outside_job.poll());
				g_system->m_event_outside_job.reset();

				JobDecl job;
				job.data = (e_void*)counter;
				job.task = [](e_void* data) {
					JobSystem::wait((volatile e_int32*)data);
					g_system->m_event_outside_job.trigger();
				};
				runJobs(&job, 1, nullptr);
				MT::yield();
				while (*counter > 0)
				{
					g_system->m_event_outside_job.waitTimeout(1);
				}
			}
		}
	}
}
