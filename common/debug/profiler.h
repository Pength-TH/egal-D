#ifndef _profiler_h_
#define _profiler_h_
#pragma once

#include "common/type.h"
#include "common/thread/thread.h"
#include "common/stl/delegate.h"

namespace egal
{
	template <typename T> class DelegateList;
	namespace Profiler
	{
		struct Block;
		enum class BlockType
		{
			TIME,
			INT
		};

		MT::ThreadID getThreadID(e_int32 index);
		e_void setThreadName(const e_char* name);
		const e_char* getThreadName(MT::ThreadID thread_id);
		e_int32 getThreadIndex(e_uint32 id);
		e_int32 getThreadCount();

		e_uint64 now();
		Block* getRootBlock(MT::ThreadID thread_id);
		Block* getCurrentBlock();
		e_int32 getBlockInt(Block* block);
		BlockType getBlockType(Block* block);
		Block* getBlockFirstChild(Block* block);
		Block* getBlockNext(Block* block);
		e_float getBlockLength(Block* block);
		e_int32 getBlockHitCount(Block* block);
		e_uint64 getBlockHitStart(Block* block, e_int32 hit_index);
		e_uint64 getBlockHitLength(Block* block, e_int32 hit_index);
		const e_char* getBlockName(Block* block);

		e_void record(const e_char* name, e_int32 value);
		e_void* beginBlock(const e_char* name);
		e_void* endBlock();
		e_void frame();
		TDelegateList<e_void()>& getFrameListeners();


#ifdef _DEBUG
		struct Scope
		{
			explicit Scope(const e_char* name) { ptr = beginBlock(name); }
			~Scope()
			{
				e_void* tmp = endBlock();
				ASSERT(tmp == ptr);
			}

			const e_void* ptr;
		};
#else
		struct Scope
		{
			explicit Scope(const e_char* name) { beginBlock(name); }
			~Scope() { endBlock(); }
		};
#endif
	}

#define PROFILE_INT(name, x) Profiler::record((name), (x));
#define PROFILE_FUNCTION() Profiler::Scope profile_scope(__FUNCTION__);
#define PROFILE_BLOCK(name) Profiler::Scope profile_scope(name);
}
#endif
