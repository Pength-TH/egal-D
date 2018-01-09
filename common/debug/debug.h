#ifndef _debug_h_
#define _debug_h_

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/egal_string.h"
#include "common/thread/sync.h"

namespace egal
{
	namespace Debug
	{
		void debugBreak();
		void debugOutput(const char* message);

		class StackNode;
		class StackTree
		{
		public:
			StackTree();
			~StackTree();

			StackNode* record();
			void printCallstack(StackNode* node);
			static bool getFunction(StackNode* node, char* out, int max_size, int* line);
			static StackNode* getParent(StackNode* node);
			static int getPath(StackNode* node, StackNode** output, int max_size);
			static void refreshModuleList();

		private:
			StackNode* insertChildren(StackNode* node, void** instruction, void** stack);

		private:
			StackNode* m_root;
			static e_int32 s_instances;
		};

		class Allocator : public IAllocator
		{
		public:
			struct AllocationInfo
			{
				AllocationInfo* previous;
				AllocationInfo* next;
				size_t size;
				StackNode* stack_leaf;
				e_uint16 align;
			};

		public:
			explicit Allocator(IAllocator& source);
			virtual ~Allocator();

			void* allocate(size_t size) override;
			void deallocate(void* ptr) override;
			void* reallocate(void* ptr, size_t size) override;
			void* allocate_aligned(size_t size, size_t align) override;
			void deallocate_aligned(void* ptr) override;
			void* reallocate_aligned(void* ptr, size_t size, size_t align) override;
			size_t getTotalSize() const { return m_total_size; }
			void checkGuards();

			IAllocator& getSourceAllocator() { return m_source; }
			AllocationInfo* getFirstAllocationInfo() const { return m_root; }
			void lock();
			void unlock();

		private:
			inline size_t getAllocationOffset();
			inline AllocationInfo* getAllocationInfoFromSystem(void* system_ptr);
			inline AllocationInfo* getAllocationInfoFromUser(void* user_ptr);
			inline e_uint8* getUserFromSystem(void* system_ptr, size_t align);
			inline e_uint8* getSystemFromUser(void* user_ptr);
			inline size_t getNeededMemory(size_t size);
			inline size_t getNeededMemory(size_t size, size_t align);
			inline void* getUserPtrFromAllocationInfo(AllocationInfo* info);

		private:
			IAllocator& m_source;
			StackTree m_stack_tree;
			MT::SpinMutex m_mutex;
			AllocationInfo* m_root;
			AllocationInfo m_sentinels[2];
			size_t m_total_size;
			bool m_is_fill_enabled;
			bool m_are_guards_enabled;
		};

	} 

	void  enableCrashReporting(bool enable);
	void  installUnhandledExceptionHandler();
}
#endif
