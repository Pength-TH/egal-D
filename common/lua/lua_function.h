#ifndef _lua_function_h_
#define _lua_function_h_
#pragma once

namespace egal
{
	class LuaManager
	{
	public:
		static void* luaAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
		{
			auto& allocator = *static_cast<IAllocator*>(ud);
			if (nsize == 0)
			{
				allocator.deallocate(ptr);
				return nullptr;
			}
			if (nsize > 0 && ptr == nullptr) return allocator.allocate(nsize);

			void* new_mem = allocator.allocate(nsize);
			StringUnitl::copyMemory(new_mem, ptr, Math::minimum(osize, nsize));
			allocator.deallocate(ptr);
			return new_mem;
		}

		void init(IAllocator& allocator)
		{
			m_state = lua_newstate(luaAllocator, &allocator);
			luaL_openlibs(m_state);



		}

		void uninit(){}

		void runScript(const char* src, int src_length, const char* path){}

		lua_State* getState() { return m_state; }
	private:
		lua_State* m_state;
	};

}



#endif
