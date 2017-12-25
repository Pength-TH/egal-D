#include "common/debug/profiler.h"
#include "common/thread/sync.h"
#include "common/egal-d.h"

namespace egal
{
	namespace Profiler
	{
		struct Block
		{
			struct Hit
			{
				e_uint64 m_length;
				e_uint64 m_start;
			};


			explicit Block(IAllocator& allocator)
				: allocator(allocator)
				, m_hits(allocator)
				, m_type(BlockType::TIME)
			{
			}


			~Block()
			{
				while (m_first_child)
				{
					Block* child = m_first_child->m_next;
					_delete(allocator, m_first_child);
					m_first_child = child;
				}
			}


			e_void frame()
			{
				m_values.int_value = 0;
				m_hits.clear();
				if (m_first_child)
				{
					m_first_child->frame();
				}
				if (m_next)
				{
					m_next->frame();
				}
			}


			IAllocator& allocator;
			Block* m_parent;
			Block* m_next;
			Block* m_first_child;
			const e_char* m_name;
			TVector<Hit> m_hits;
			BlockType m_type;
			union
			{
				e_float float_value;
				e_int32 int_value;
			} m_values;
		};


		const e_char* getBlockName(Block* block)
		{
			return block->m_name;
		}


		e_int32 getBlockInt(Block* block)
		{
			return block->m_values.int_value;
		}


		BlockType getBlockType(Block* block)
		{
			return block->m_type;
		}


		Block* getBlockFirstChild(Block* block)
		{
			return block->m_first_child;
		}


		Block* getBlockNext(Block* block)
		{
			return block->m_next;
		}


		e_uint64 getBlockHitStart(Block* block, e_int32 hit_index)
		{
			return block->m_hits[hit_index].m_start;
		}


		e_uint64 getBlockHitLength(Block* block, e_int32 hit_index)
		{
			return block->m_hits[hit_index].m_length;
		}


		e_int32 getBlockHitCount(Block* block)
		{
			return block->m_hits.size();
		}


		struct ThreadData
		{
			ThreadData()
			{
				root_block = current_block = nullptr;
				name[0] = '\0';
			}

			Block* root_block;
			Block* current_block;
			e_char name[30];
		};


		struct Instance
		{
			Instance()
				: threads(allocator)
				, frame_listeners(allocator)
				, m_mutex(false)
			{
				threads.insert(MT::getCurrentThreadID(), &main_thread);
				timer = _aligned_new(allocator, Timer);
			}

			~Instance()
			{
				delete timer;
				for (auto* i : threads)
				{
					if (i != &main_thread) _delete(allocator, i);
				}
			}

			DefaultAllocator allocator;
			TDelegateList<e_void()> frame_listeners;
			THashMap<MT::ThreadID, ThreadData*> threads;
			ThreadData main_thread;
			Timer* timer;
			MT::SpinMutex m_mutex;
		};


		Instance g_instance;


		e_float getBlockLength(Block* block)
		{
			e_uint64 ret = 0;
			for (e_int32 i = 0, c = block->m_hits.size(); i < c; ++i)
			{
				ret += block->m_hits[i].m_length;
			}
			return e_float(ret / (double)g_instance.timer->getFrequency());
		}


		struct BlockInfo
		{
			Block* block;
			ThreadData* thread_data;
		};

		static BlockInfo getBlock(const e_char* name)
		{
			auto thread_id = MT::getCurrentThreadID();

			ThreadData* thread_data;
			{
				MT::SpinLock lock(g_instance.m_mutex);
				auto iter = g_instance.threads.find(thread_id);
				if (iter == g_instance.threads.end())
				{
					g_instance.threads.insert(thread_id, _aligned_new(g_instance.allocator, ThreadData));
					iter = g_instance.threads.find(thread_id);
				}

				thread_data = iter.value();
			}

			if (!thread_data->current_block)
			{
				Block* root = thread_data->root_block;
				while (root && root->m_name != name)
				{
					root = root->m_next;
				}
				if (root)
				{
					thread_data->current_block = root;
				}
				else
				{
					Block* root = _aligned_new(g_instance.allocator, Block)(g_instance.allocator);
					root->m_parent = nullptr;
					root->m_next = thread_data->root_block;
					root->m_first_child = nullptr;
					root->m_name = name;
					thread_data->root_block = thread_data->current_block = root;
				}
			}
			else
			{
				Block* child = thread_data->current_block->m_first_child;
				while (child && child->m_name != name)
				{
					child = child->m_next;
				}
				if (!child)
				{
					child = _aligned_new(g_instance.allocator, Block)(g_instance.allocator);
					child->m_parent = thread_data->current_block;
					child->m_first_child = nullptr;
					child->m_name = name;
					child->m_next = thread_data->current_block->m_first_child;
					thread_data->current_block->m_first_child = child;
				}

				thread_data->current_block = child;
			}

			return { thread_data->current_block, thread_data };
		}


		e_void record(const e_char* name, e_int32 value)
		{
			auto data = getBlock(name);
			if (data.block->m_type != BlockType::INT)
			{
				data.block->m_values.int_value = 0;
				data.block->m_type = BlockType::INT;
			}
			data.block->m_values.int_value += value;
			data.thread_data->current_block = data.block->m_parent;
		}


		e_void* beginBlock(const e_char* name)
		{
			BlockInfo data = getBlock(name);

			Block::Hit& hit = data.block->m_hits.emplace();
			hit.m_start = g_instance.timer->SinceStart();
			hit.m_length = 0;

			return data.block;
		}

		const e_char* getThreadName(MT::ThreadID thread_id)
		{
			auto iter = g_instance.threads.find(thread_id);
			if (iter == g_instance.threads.end()) return "N/A";
			return iter.value()->name;
		}


		e_void setThreadName(const e_char* name)
		{
			MT::SpinLock lock(g_instance.m_mutex);
			MT::ThreadID thread_id = MT::getCurrentThreadID();
			auto iter = g_instance.threads.find(thread_id);
			if (iter == g_instance.threads.end())
			{
				g_instance.threads.insert(thread_id, _aligned_new(g_instance.allocator, ThreadData));
				iter = g_instance.threads.find(thread_id);
			}
			StringUnitl::copyString(iter.value()->name, name);
		}

		MT::ThreadID getThreadID(e_int32 index)
		{
			auto iter = g_instance.threads.begin();
			auto end = g_instance.threads.end();
			for (e_int32 i = 0; i < index; ++i)
			{
				++iter;
				if (iter == end) return 0;
			}
			return iter.key();
		}

		e_int32 getThreadIndex(e_uint32 id)
		{
			auto iter = g_instance.threads.begin();
			auto end = g_instance.threads.end();
			e_int32 idx = 0;
			while (iter != end)
			{
				if (iter.key() == id) return idx;
				++idx;
				++iter;
			}
			return -1;
		}

		e_int32 getThreadCount()
		{
			return g_instance.threads.size();
		}

		e_uint64 now()
		{
			return g_instance.timer->SinceStart();
		}

		Block* getRootBlock(MT::ThreadID thread_id)
		{
			auto iter = g_instance.threads.find(thread_id);
			if (!iter.isValid()) return nullptr;

			return iter.value()->root_block;
		}

		Block* getCurrentBlock()
		{
			auto thread_id = MT::getCurrentThreadID();

			ThreadData* thread_data;
			{
				MT::SpinLock lock(g_instance.m_mutex);
				auto iter = g_instance.threads.find(thread_id);
				ASSERT(iter.isValid());
				thread_data = iter.value();
			}

			return thread_data->current_block;
		}

		e_void* endBlock()
		{
			auto thread_id = MT::getCurrentThreadID();

			ThreadData* thread_data;
			{
				MT::SpinLock lock(g_instance.m_mutex);
				auto iter = g_instance.threads.find(thread_id);
				ASSERT(iter.isValid());
				thread_data = iter.value();
			}

			ASSERT(thread_data->current_block);
			auto* block = thread_data->current_block;
			e_uint64 now = g_instance.timer->SinceStart();
			thread_data->current_block->m_hits.back().m_length = now - thread_data->current_block->m_hits.back().m_start;
			thread_data->current_block = thread_data->current_block->m_parent;
			return block;
		}

		e_void frame()
		{
			PROFILE_FUNCTION();

			MT::SpinLock lock(g_instance.m_mutex);
			g_instance.frame_listeners.invoke();
			e_uint64 now = g_instance.timer->SinceStart();

			for (auto* i : g_instance.threads)
			{
				if (!i->root_block) continue;
				i->root_block->frame();
				auto* block = i->current_block;
				while (block)
				{
					auto& hit = block->m_hits.emplace();
					hit.m_start = now;
					hit.m_length = 0;
					block = block->m_parent;
				}
			}
		}

		TDelegateList<e_void()>& getFrameListeners()
		{
			return g_instance.frame_listeners;
		}

	}
}
