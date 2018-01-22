
#include "common/resource/material_manager.h"
#include "common/resource/texture_manager.h"
#include "common/resource/shader_manager.h"
#include "common/egal-d.h"

namespace egal
{
	static struct CustomFlags
	{
		e_char	flags[32][32];
		e_int32 count;
	} s_custom_flags = {};

	Material::Material(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
		: Resource(path, resource_manager, allocator)
		, m_shader(nullptr)
		, m_uniforms(allocator)
		, m_allocator(allocator)
		, m_texture_count(0)
		, m_render_states(BGFX_STATE_CULL_CW)
		, m_color(1, 1, 1, 1)
		, m_metallic(0)
		, m_roughness(1.0f)
		, m_shader_instance(nullptr)
		, m_define_mask(0)
		, m_command_buffer(&DEFAULT_COMMAND_BUFFER)
		, m_custom_flags(0)
		, m_render_layer(0)
		, m_render_layer_mask(1)
		, m_layers_count(0)
	{
		setAlphaRef(DEFAULT_ALPHA_REF_VALUE);
		for (e_int32 i = 0; i < MAX_TEXTURE_COUNT; ++i)
		{
			m_textures[i] = nullptr;
		}

		setShader(nullptr);
	}

	Material::~Material()
	{
		ASSERT(isEmpty());
	}

	const e_char* Material::getCustomFlagName(e_int32 index)
	{
		return s_custom_flags.flags[index];
	}

	e_int32 Material::getCustomFlagCount()
	{
		return s_custom_flags.count;
	}

	e_uint32 Material::getCustomFlag(const e_char* flag_name)
	{
		for (e_int32 i = 0; i < s_custom_flags.count; ++i)
		{
			if (StringUnitl::equalStrings(s_custom_flags.flags[i], flag_name)) return 1 << i;
		}
		if (s_custom_flags.count >= TlengthOf(s_custom_flags.flags))
		{
			ASSERT(false);
			return 0;
		}
		StringUnitl::copyString(s_custom_flags.flags[s_custom_flags.count], flag_name);
		++s_custom_flags.count;
		return 1 << (s_custom_flags.count - 1);
	}

	e_bool Material::isDefined(e_uint8 define_idx) const
	{
		return (m_define_mask & (1 << define_idx)) != 0;
	}

	e_bool Material::hasDefine(e_uint8 define_idx) const
	{
		return m_shader->hasDefine(define_idx) != 0;
	}

	void Material::setDefine(e_uint8 define_idx, e_bool enabled)
	{
		e_uint32 old_mask = m_define_mask;
		if (enabled)
		{
			m_define_mask |= 1 << define_idx;
		}
		else
		{
			m_define_mask &= ~(1 << define_idx);
		}

		if (!isReady()) 
			return;
		if (!m_shader) 
			return;

		if (old_mask != m_define_mask)
		{
			m_shader_instance = &m_shader->getInstance(m_define_mask);
		}
	}

	void Material::unload()
	{
		if (m_command_buffer != &DEFAULT_COMMAND_BUFFER) m_allocator.deallocate(m_command_buffer);
		m_command_buffer = &DEFAULT_COMMAND_BUFFER;
		m_uniforms.clear();
		setShader(nullptr);

		ResourceManagerBase* texture_manager = m_resource_manager.getOwner().get( RESOURCE_TEXTURE_TYPE);
		for (e_int32 i = 0; i < m_texture_count; i++)
		{
			if (m_textures[i])
			{
				removeDependency(*m_textures[i]);
				texture_manager->unload(*m_textures[i]);
			}
		}
		m_alpha_ref = 0.3f;
		m_color.x = 1;
		m_color.y = 1;
		m_color.z = 1;
		m_color.w = 1;
		m_custom_flags = 0;
		m_define_mask = 0;
		m_layers_count = 0;
		m_metallic = 0.0f;
		m_render_layer = 0;
		m_render_layer_mask = 1;
		m_roughness = 1.0f;
		m_texture_count = 0;
	}

	void Material::setTexturePath(e_int32 i, const ArchivePath& path)
	{
		if (path.length() == 0)
		{
			setTexture(i, nullptr);
		}
		else
		{
			Texture* texture = static_cast<Texture*>(m_resource_manager.getOwner().get( RESOURCE_TEXTURE_TYPE)->load(path));
			setTexture(i, texture);
		}
	}


	void Material::setLayersCount(e_int32 layers)
	{
		++m_empty_dep_count;
		checkState();
		m_layers_count = layers;
		--m_empty_dep_count;
		checkState();
	}


	void Material::setRenderLayer(e_int32 layer)
	{
		++m_empty_dep_count;
		checkState();
		m_render_layer = layer;
		m_render_layer_mask = 1ULL << (e_uint64)layer;
		--m_empty_dep_count;
		checkState();
	}


	void Material::setTexture(e_int32 i, Texture* texture)
	{
		Texture* old_texture = i < m_texture_count ? m_textures[i] : nullptr;

		if (texture) 
			addDependency(*texture);
		
		m_textures[i] = texture;
		if (i >= m_texture_count) 
			m_texture_count = i + 1;

		if (old_texture)
		{
			removeDependency(*old_texture);
			m_resource_manager.getOwner().get( RESOURCE_TEXTURE_TYPE)->unload(*old_texture);
		}
		if (isReady() && m_shader)
		{
			e_int32 define_idx = m_shader->m_texture_slots[i].define_idx;
			if (define_idx >= 0)
			{
				if (m_textures[i])
				{
					m_define_mask |= 1 << define_idx;
				}
				else
				{
					m_define_mask &= ~(1 << define_idx);
				}
			}

			createCommandBuffer();
			m_shader_instance = &m_shader->getInstance(m_define_mask);
		}
	}


	void Material::setShader(const ArchivePath& path)
	{
		Shader* shader = static_cast<Shader*>(m_resource_manager.getOwner().get( RESOURCE_SHADER_TYPE)->load(path));
		setShader(shader);
	}

	void Material::createCommandBuffer()
	{
		if (m_command_buffer != &DEFAULT_COMMAND_BUFFER) 
			m_allocator.deallocate(m_command_buffer);
		
		m_command_buffer = &DEFAULT_COMMAND_BUFFER;
		if (!m_shader) 
			return;

		CommandBufferGenerator generator;

		for (e_int32 i = 0; i < m_shader->m_uniforms.size(); ++i)
		{
			const Material::Uniform& uniform = m_uniforms[i];
			const Shader::Uniform& shader_uniform = m_shader->m_uniforms[i];

			switch (shader_uniform.type)
			{
			case Shader::Uniform::FLOAT:
				generator.setUniform(shader_uniform.handle, float4(uniform.float_value, 0, 0, 0));
				break;
			case Shader::Uniform::FLOAT2:
				generator.setUniform(shader_uniform.handle, float4(uniform.vec2[0], uniform.vec2[1], 0, 0));
				break;
			case Shader::Uniform::FLOAT3:
			case Shader::Uniform::COLOR:
				generator.setUniform(shader_uniform.handle, float4(uniform.vec3[0], uniform.vec3[1], uniform.vec3[2], 0));
				break;
			case Shader::Uniform::TIME: generator.setTimeUniform(shader_uniform.handle); break;
			default: ASSERT(false); break;
			}
		}

		for (e_int32 i = 0; i < m_shader->m_texture_slot_count; ++i)
		{
			if (i >= m_texture_count || !m_textures[i]) 
				continue;

			generator.setTexture(i, m_shader->m_texture_slots[i].uniform_handle, m_textures[i]->handle);
		}

		auto& renderer = static_cast<MaterialManager&>(m_resource_manager).getRenderer();
		auto& uniform = renderer.getMaterialColorUniform();
		generator.setUniform(uniform, m_color);

		float4 roughness_metallic(m_roughness, m_metallic, 0, 0);
		auto& rm_uniform = renderer.getRoughnessMetallicUniform();
		generator.setUniform(rm_uniform, roughness_metallic);
		generator.end();

		m_command_buffer = (e_uint8*)m_allocator.allocate(generator.getSize());
		generator.getData(m_command_buffer);
	}


	void Material::onBeforeReady()
	{
		if (!m_shader) return;

		for (e_int32 i = 0; i < m_shader->m_uniforms.size(); ++i)
		{
			auto& shader_uniform = m_shader->m_uniforms[i];
			e_bool found = false;
			for (e_int32 j = i; j < m_uniforms.size(); ++j)
			{
				if (m_uniforms[j].name_hash == shader_uniform.name_hash)
				{
					auto tmp = m_uniforms[i];
					m_uniforms[i] = m_uniforms[j];
					m_uniforms[j] = tmp;
					found = true;
					break;
				}
			}
			if (found) continue;
			if (i < m_uniforms.size())
			{
				m_uniforms.emplace(m_uniforms[i]);
			}
			else
			{
				m_uniforms.emplace();
			}
			m_uniforms[i].name_hash = shader_uniform.name_hash;
		}

		e_uint8 alpha_ref = e_uint8(m_alpha_ref * 255.0f);
		m_render_states = (m_render_states & ~BGFX_STATE_ALPHA_REF_MASK) | BGFX_STATE_ALPHA_REF(alpha_ref);
		m_render_states |= m_shader->m_render_states;

		for (e_int32 i = 0; i < m_shader->m_texture_slot_count; ++i)
		{
			e_int32 define_idx = m_shader->m_texture_slots[i].define_idx;
			if (define_idx >= 0)
			{
				if (m_textures[i])
				{
					m_define_mask |= 1 << define_idx;
				}
				else
				{
					m_define_mask &= ~(1 << define_idx);
				}
			}
		}

		createCommandBuffer();
		m_shader_instance = &m_shader->getInstance(m_define_mask);
	}


	void Material::setShader(Shader* shader)
	{
		auto& mat_manager = static_cast<MaterialManager&>(m_resource_manager);

		if (m_shader && m_shader != mat_manager.getRenderer().getDefaultShader())
		{
			Shader* shader = m_shader;
			m_shader = nullptr;
			removeDependency(*shader);
			m_resource_manager.getOwner().get( RESOURCE_SHADER_TYPE)->unload(*shader);
		}
		m_shader = shader;
		if (m_shader)
		{
			addDependency(*m_shader);
			if (m_shader->isReady()) onBeforeReady();
		}
		else
		{
			m_shader = mat_manager.getRenderer().getDefaultShader();
			m_shader_instance = m_shader->m_instances.empty() ? nullptr : &m_shader->m_instances[0];
		}
	}


	const e_char* Material::getTextureUniform(e_int32 i)
	{
		if (i < m_shader->m_texture_slot_count) return m_shader->m_texture_slots[i].uniform;
		return "";
	}


	Texture* Material::getTextureByUniform(const e_char* uniform) const
	{
		if (!m_shader) return nullptr;

		for (e_int32 i = 0, c = m_shader->m_texture_slot_count; i < c; ++i)
		{
			if (StringUnitl::equalStrings(m_shader->m_texture_slots[i].uniform, uniform))
			{
				return m_textures[i];
			}
		}
		return nullptr;
	}


	e_bool Material::isTextureDefine(e_uint8 define_idx) const
	{
		if (!m_shader) return false;

		for (e_int32 i = 0, c = m_shader->m_texture_slot_count; i < c; ++i)
		{
			if (m_shader->m_texture_slots[i].define_idx == define_idx)
			{
				return true;
			}
		}
		return false;
	}

	void Material::setAlphaRef(e_float value)
	{
		m_alpha_ref = value;
		e_uint8 val = e_uint8(value * 255.0f);
		m_render_states &= ~BGFX_STATE_ALPHA_REF_MASK;
		m_render_states |= BGFX_STATE_ALPHA_REF(val);
	}


	void Material::enableBackfaceCulling(e_bool enable)
	{
		if (enable)
		{
			m_render_states |= BGFX_STATE_CULL_CW;
		}
		else
		{
			m_render_states &= ~BGFX_STATE_CULL_MASK;
		}
	}


	e_bool Material::isBackfaceCulling() const
	{
		return (m_render_states & BGFX_STATE_CULL_MASK) != 0;
	}

}

namespace egal
{
	Resource* MaterialManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Material)(path, *this, m_allocator);
	}

	egal::Resource* MaterialManager::createResource()
	{
		return _aligned_new(m_allocator, Material)(ArchivePath(""), *this, m_allocator);
	}

	e_void MaterialManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Material*>(&resource));
	}
}