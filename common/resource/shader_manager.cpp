#include "common/resource/shader_manager.h"
#include "common/egal-d.h"

#include "common/lua/lua_shader.h"

namespace egal
{
	

	Shader::Shader(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
		: Resource(path, resource_manager, allocator)
		, m_allocator(allocator)
		, m_instances(allocator)
		, m_texture_slot_count(0)
		, m_uniforms(allocator)
		, m_render_states(0)
		, m_all_defines_mask(0)
	{
	}

	Shader::~Shader()
	{
		ASSERT(isEmpty());
	}

	e_bool Shader::hasDefine(e_uint8 define_idx) const
	{
		return (m_combintions.all_defines_mask & (1 << define_idx)) != 0;
	}

	ShaderInstance& Shader::getInstance(e_uint32 mask)
	{
		mask = mask & m_all_defines_mask;
		for (e_int32 i = 0; i < m_instances.size(); ++i)
		{
			if (m_instances[i].define_mask == mask)
			{
				return m_instances[i];
			}
		}

		log_error("Renderer Unknown shader combination requested: %s.", mask);
		return m_instances[0];
	}

	ShaderCombinations::ShaderCombinations()
	{
		StringUnitl::setMemory(this, 0, sizeof(*this));
	}

	Renderer& Shader::getRenderer()
	{
		return static_cast<ShaderManager&>(m_resource_manager).getRenderer();
	}

	static e_uint32 getDefineMaskFromDense(const Shader& shader, e_uint32 dense)
	{
		e_uint32 mask = 0;
		e_int32 defines_count = Math::minimum(TlengthOf(shader.m_combintions.defines), e_int32(sizeof(dense) * 8));

		for (e_int32 i = 0; i < defines_count; ++i)
		{
			if (dense & (1 << i))
			{
				mask |= 1 << shader.m_combintions.defines[i];
			}
		}
		return mask;
	}

	e_bool Shader::generateInstances()
	{
		e_bool is_opengl = bgfx::getRendererType() == bgfx::RendererType::OpenGL ||
			bgfx::getRendererType() == bgfx::RendererType::OpenGLES;
		m_instances.clear();

		e_uint32 count = 1 << m_combintions.define_count;

		auto* binary_manager = m_resource_manager.getOwner().get( RESOURCE_SHADER_BINARY_TYPE);
		e_char basename[MAX_PATH_LENGTH];
		StringUnitl::getBasename(basename, sizeof(basename), getPath().c_str());

		m_instances.reserve(count);
		for (e_uint32 mask = 0; mask < count; ++mask)
		{
			ShaderInstance& instance = m_instances.emplace(*this);

			instance.define_mask = getDefineMaskFromDense(*this, mask);
			m_all_defines_mask |= instance.define_mask;

			for (e_int32 pass_idx = 0; pass_idx < m_combintions.pass_count; ++pass_idx)
			{
				const e_char* pass = m_combintions.passes[pass_idx];
				StaticString<MAX_PATH_LENGTH> path("pipelines/compiled", is_opengl ? "_gl/" : "/");
				e_int32 actual_mask = mask & m_combintions.vs_local_mask[pass_idx];
				path << basename << "_" << pass << actual_mask << "_vs.shb";

				ArchivePath vs_path(path);
				auto* vs_binary = static_cast<ShaderBinary*>(binary_manager->load(vs_path));
				vs_binary->m_shader = this;
				addDependency(*vs_binary);
				instance.binaries[pass_idx * 2] = vs_binary;

				path.data[0] = '\0';
				actual_mask = mask & m_combintions.fs_local_mask[pass_idx];
				path << "pipelines/compiled" << (is_opengl ? "_gl/" : "/") << basename;
				path << "_" << pass << actual_mask << "_fs.shb";

				ArchivePath fs_path(path);
				auto* fs_binary = static_cast<ShaderBinary*>(binary_manager->load(fs_path));
				fs_binary->m_shader = this;
				addDependency(*fs_binary);
				instance.binaries[pass_idx * 2 + 1] = fs_binary;
			}
		}
		return true;
	}

	e_void Shader::unload()
	{
		m_combintions = {};

		for (auto& uniform : m_uniforms)
		{
			bgfx::destroy(uniform.handle);
		}
		m_uniforms.clear();

		for (e_int32 i = 0; i < m_texture_slot_count; ++i)
		{
			if (bgfx::isValid(m_texture_slots[i].uniform_handle))
			{
				bgfx::destroy(m_texture_slots[i].uniform_handle);
			}
			m_texture_slots[i].uniform_handle = BGFX_INVALID_HANDLE;
		}
		m_texture_slot_count = 0;

		m_instances.clear();

		m_all_defines_mask = 0;
		m_render_states = 0;
	}

	bgfx::ProgramHandle ShaderInstance::getProgramHandle(e_int32 pass_idx)
	{
		if (!bgfx::isValid(program_handles[pass_idx]))
		{
			for (e_int32 i = 0; i < TlengthOf(shader.m_combintions.passes); ++i)
			{
				auto& pass = shader.m_combintions.passes[i];
				e_int32 global_idx = shader.getRenderer().getPassIdx(pass);
				if (global_idx == pass_idx)
				{
					e_int32 binary_index = i * 2;
					if (!binaries[binary_index] || !binaries[binary_index + 1]) break;
					auto vs_handle = binaries[binary_index]->getHandle();
					auto fs_handle = binaries[binary_index + 1]->getHandle();
					auto program = bgfx::createProgram(vs_handle, fs_handle);
					program_handles[global_idx] = program;
					break;
				}
			}
		}

		return program_handles[pass_idx];
	}

	ShaderInstance::~ShaderInstance()
	{
		for (e_int32 i = 0; i < TlengthOf(program_handles); ++i)
		{
			if (bgfx::isValid(program_handles[i]))
			{
				bgfx::destroy(program_handles[i]);
			}
		}

		for (auto* binary : binaries)
		{
			if (!binary) continue;

			shader.removeDependency(*binary);
			binary->getResourceManager().unload(*binary);
		}
	}

	e_void Shader::onBeforeEmpty()
	{
		for (ShaderInstance& inst : m_instances)
		{
			for (e_int32 i = 0; i < TlengthOf(inst.program_handles); ++i)
			{
				if (bgfx::isValid(inst.program_handles[i]))
				{
					bgfx::destroy(inst.program_handles[i]);
					inst.program_handles[i] = BGFX_INVALID_HANDLE;
				}
			}
		}
	}

	ShaderBinary::ShaderBinary(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
		: Resource(path, resource_manager, allocator)
		, m_handle(INVALID_HANDLE)
	{
	}

	e_void ShaderBinary::unload()
	{
		if (bgfx::isValid(m_handle)) bgfx::destroy(m_handle);
		m_handle = BGFX_INVALID_HANDLE;
	}

	e_bool ShaderBinary::load(FS::IFile& file)
	{
		auto* mem = bgfx::alloc((e_uint32)file.size() + 1);
		file.read(mem->data, file.size());
		mem->data[file.size()] = '\0';
		m_handle = bgfx::createShader(mem);
		m_size = file.size();
		if (!bgfx::isValid(m_handle))
		{
			log_error("Renderer %s : Failed to create bgfx shader", getPath().c_str());
			return false;
		}
		bgfx::setName(m_handle, getPath().c_str());
		return true;
	}
}

/** manager */
namespace egal
{
	ShaderManager::ShaderManager(Renderer& renderer, IAllocator& allocator)
		: ResourceManagerBase(allocator)
		, m_allocator(allocator)
		, m_renderer(renderer)
	{
		m_buffer = nullptr;
		m_buffer_size = -1;
	}


	ShaderManager::~ShaderManager()
	{
		_delete(m_allocator, m_buffer);
	}


	Resource* ShaderManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Shader)(path, *this, m_allocator);
	}


	egal::Resource* ShaderManager::createResource()
	{
		return _aligned_new(m_allocator, Shader)(ArchivePath(), *this, m_allocator);
	}

	e_void ShaderManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Shader*>(&resource));
	}

	Renderer& ShaderManager::getRenderer() 
	{ 
		return m_renderer; 
	}

	e_uint8* ShaderManager::getBuffer(e_int32 size)
	{
		if (m_buffer_size < size)
		{
			_delete(m_allocator, m_buffer);
			m_buffer = nullptr;
			m_buffer_size = -1;
		}
		if (m_buffer == nullptr)
		{
			m_buffer = (e_uint8*)m_allocator.allocate(sizeof(e_uint8) * size);
			m_buffer_size = size;
		}
		return m_buffer;
	}


	ShaderBinaryManager::ShaderBinaryManager(Renderer& renderer, IAllocator& allocator)
		: ResourceManagerBase(allocator)
		, m_allocator(allocator)
	{
	}


	ShaderBinaryManager::~ShaderBinaryManager() = default;


	Resource* ShaderBinaryManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, ShaderBinary)(path, *this, m_allocator);
	}

	egal::Resource* ShaderBinaryManager::createResource()
	{
		return _aligned_new(m_allocator, ShaderBinary)(ArchivePath(""), *this, m_allocator);
	}

	e_void ShaderBinaryManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<ShaderBinary*>(&resource));
	}
}

/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
namespace egal
{
	e_bool Shader::getShaderCombinations(const e_char* shd_path,
		Renderer& renderer,
		const e_char* shader_content,
		ShaderCombinations* output)
	{
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		//registerFunctions(nullptr, output, &renderer, L);

		e_bool errors = luaL_loadbuffer(L, shader_content, StringUnitl::stringLength(shader_content), "") != LUA_OK;
		errors = errors || lua_pcall(L, 0, 0, 0) != LUA_OK;
		if (errors)
		{
			log_error("Renderer %s - %s.", shd_path, lua_tostring(L, -1));
			lua_pop(L, 1);
			lua_close(L);
			return false;
		}
		lua_close(L);
		return true;
	}

	e_bool Shader::load(FS::IFile& file)
	{
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		registerFunctions(this, &m_combintions, &getRenderer(), L);
		m_render_states = BGFX_STATE_DEPTH_TEST_LEQUAL;

		e_bool errors = luaL_loadbuffer(L, (const e_char*)file.getBuffer(), file.size(), "") != LUA_OK;
		errors = errors || lua_pcall(L, 0, 0, 0) != LUA_OK;
		if (errors)
		{
			log_error("Renderer %s:%s.", getPath().c_str(), lua_tostring(L, -1));
			lua_pop(L, 1);
			return false;
		}

		if (!generateInstances())
		{
			log_error("Renderer Could not load instances of shader %s.", getPath().c_str());
			return false;
		}

		m_size = file.size();
		lua_close(L);
		return true;
	}

}
