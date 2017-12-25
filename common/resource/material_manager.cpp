
#include "common/resource/material_manager.h"
#include "common/resource/texture_manager.h"
#include "common/resource/shader_manager.h"
#include "common/egal-d.h"

namespace egal
{
	static const ResourceType TEXTURE_TYPE("texture");
	static const ResourceType SHADER_TYPE("shader");
	static const e_float DEFAULT_ALPHA_REF_VALUE = 0.3f;
	
	static e_uint8 DEFAULT_COMMAND_BUFFER = 0;
	static struct CustomFlags
	{
		e_char flags[32][32];
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

		ResourceManagerBase* texture_manager = m_resource_manager.getOwner().get(TEXTURE_TYPE);
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

	e_bool Material::save(JsonSerializer& serializer)
	{
		if (!isReady()) return false;
		if (!m_shader) return false;

		auto& renderer = static_cast<MaterialManager&>(m_resource_manager).getRenderer();

		serializer.beginObject();
		serializer.serialize("render_layer", renderer.getLayerName(m_render_layer));
		serializer.serialize("layers_count", m_layers_count);
		serializer.serialize("shader", m_shader ? m_shader->getPath() : ArchivePath(""));
		serializer.serialize("backface_culling", isBackfaceCulling());
		for (e_int32 i = 0; i < m_texture_count; ++i)
		{
			e_char path[MAX_PATH_LENGTH];
			e_int32 flags = 0;
			if (m_textures[i])
			{
				flags = m_textures[i]->bgfx_flags;
				path[0] = '/';
				StringUnitl::copyString(path + 1, MAX_PATH_LENGTH - 1, m_textures[i]->getPath().c_str());
			}
			else
			{
				path[0] = '\0';
			}
			serializer.beginObject("texture");
			serializer.serialize("source", path);
			if (flags & BGFX_TEXTURE_SRGB) serializer.serialize("srgb", true);
			if (flags & BGFX_TEXTURE_U_CLAMP) serializer.serialize("u_clamp", true);
			if (flags & BGFX_TEXTURE_V_CLAMP) serializer.serialize("v_clamp", true);
			if (flags & BGFX_TEXTURE_W_CLAMP) serializer.serialize("w_clamp", true);
			if (flags & BGFX_TEXTURE_MIN_POINT) serializer.serialize("min_filter", "point");
			if (flags & BGFX_TEXTURE_MIN_ANISOTROPIC) serializer.serialize("min_filter", "anisotropic");
			if (flags & BGFX_TEXTURE_MAG_POINT) serializer.serialize("mag_filter", "point");
			if (flags & BGFX_TEXTURE_MAG_ANISOTROPIC) serializer.serialize("mag_filter", "anisotropic");
			if (m_textures[i] && m_textures[i]->getData()) serializer.serialize("keep_data", true);
			serializer.endObject();
		}

		if (m_custom_flags != 0)
		{
			serializer.beginArray("custom_flags");
			for (e_int32 i = 0; i < 32; ++i)
			{
				if (m_custom_flags & (1 << i)) serializer.serializeArrayItem(s_custom_flags.flags[i]);
			}
			serializer.endArray();
		}

		serializer.beginArray("defines");
		for (e_int32 i = 0; i < sizeof(m_define_mask) * 8; ++i)
		{
			if ((m_define_mask & (1 << i)) == 0) continue;
			const e_char* def = renderer.getShaderDefine(i);
			if (StringUnitl::equalStrings("SKINNED", def)) continue;
			serializer.serializeArrayItem(def);
		}
		serializer.endArray();

		serializer.beginArray("uniforms");
		for (e_int32 i = 0; i < m_shader->m_uniforms.size(); ++i)
		{
			serializer.beginObject();
			const auto& uniform = m_shader->m_uniforms[i];

			serializer.serialize("name", uniform.name);
			switch (uniform.type)
			{
			case Shader::Uniform::e_float:
				serializer.serialize("float_value", m_uniforms[i].float_value);
				break;
			case Shader::Uniform::COLOR:
				serializer.beginArray("color");
				serializer.serializeArrayItem(m_uniforms[i].vec3[0]);
				serializer.serializeArrayItem(m_uniforms[i].vec3[1]);
				serializer.serializeArrayItem(m_uniforms[i].vec3[2]);
				serializer.endArray();
				break;
			case Shader::Uniform::FLOAT3:
				serializer.beginArray("vec3");
				serializer.serializeArrayItem(m_uniforms[i].vec3[0]);
				serializer.serializeArrayItem(m_uniforms[i].vec3[1]);
				serializer.serializeArrayItem(m_uniforms[i].vec3[2]);
				serializer.endArray();
				break;
			case Shader::Uniform::FLOAT2:
				serializer.beginArray("vec2");
				serializer.serializeArrayItem(m_uniforms[i].vec2[0]);
				serializer.serializeArrayItem(m_uniforms[i].vec2[1]);
				serializer.endArray();
				break;
			case Shader::Uniform::TIME:
				serializer.serialize("time", 0);
				break;
			case Shader::Uniform::INT32:
				serializer.serialize("int_value", m_uniforms[i].int_value);
				break;
			case Shader::Uniform::MATRIX4:
				serializer.beginArray("matrix_value");
				for (e_int32 j = 0; j < 16; ++j)
				{
					serializer.serializeArrayItem(m_uniforms[i].matrix[j]);
				}
				serializer.endArray();
				break;
			default:
				ASSERT(false);
				break;
			}
			serializer.endObject();
		}
		serializer.endArray();
		serializer.serialize("metallic", m_metallic);
		serializer.serialize("roughness", m_roughness);
		serializer.serialize("alpha_ref", m_alpha_ref);
		serializer.beginArray("color");
		serializer.serializeArrayItem(m_color.x);
		serializer.serializeArrayItem(m_color.y);
		serializer.serializeArrayItem(m_color.z);
		serializer.serializeArrayItem(m_color.w);
		serializer.endArray();
		serializer.endObject();
		return true;
	}


	void Material::deserializeCustomFlags(JsonSerializer& serializer)
	{
		m_custom_flags = 0;
		serializer.deserializeArrayBegin();
		while (!serializer.isArrayEnd())
		{
			e_char tmp[32];
			serializer.deserializeArrayItem(tmp, TlengthOf(tmp), "");
			setCustomFlag(getCustomFlag(tmp));
		}
		serializer.deserializeArrayEnd();
	}


	void Material::deserializeDefines(JsonSerializer& serializer)
	{
		auto& renderer = static_cast<MaterialManager&>(m_resource_manager).getRenderer();
		serializer.deserializeArrayBegin();
		m_define_mask = 0;
		while (!serializer.isArrayEnd())
		{
			e_char tmp[32];
			serializer.deserializeArrayItem(tmp, TlengthOf(tmp), "");
			m_define_mask |= 1 << renderer.getShaderDefineIdx(tmp);
		}
		serializer.deserializeArrayEnd();
	}


	void Material::deserializeUniforms(JsonSerializer& serializer)
	{
		serializer.deserializeArrayBegin();
		m_uniforms.clear();
		while (!serializer.isArrayEnd())
		{
			Uniform& uniform = m_uniforms.emplace();
			serializer.nextArrayItem();
			serializer.deserializeObjectBegin();
			e_char label[256];
			while (!serializer.isObjectEnd())
			{
				serializer.deserializeLabel(label, 255);
				if (StringUnitl::equalStrings(label, "name"))
				{
					e_char name[32];
					serializer.deserialize(name, TlengthOf(name), "");
					uniform.name_hash = crc32(name);
				}
				else if (StringUnitl::equalStrings(label, "int_value"))
				{
					serializer.deserialize(uniform.int_value, 0);
				}
				else if (StringUnitl::equalStrings(label, "float_value"))
				{
					serializer.deserialize(uniform.float_value, 0);
				}
				else if (StringUnitl::equalStrings(label, "matrix_value"))
				{
					serializer.deserializeArrayBegin();
					for (e_int32 i = 0; i < 16; ++i)
					{
						serializer.deserializeArrayItem(uniform.matrix[i], 0);
					}
					serializer.deserializeArrayEnd();
				}
				else if (StringUnitl::equalStrings(label, "time"))
				{
					serializer.deserialize(uniform.float_value, 0);
				}
				else if (StringUnitl::equalStrings(label, "color"))
				{
					serializer.deserializeArrayBegin();
					serializer.deserializeArrayItem(uniform.vec3[0], 0);
					serializer.deserializeArrayItem(uniform.vec3[1], 0);
					serializer.deserializeArrayItem(uniform.vec3[2], 0);
					serializer.deserializeArrayEnd();
				}
				else if (StringUnitl::equalStrings(label, "vec3"))
				{
					serializer.deserializeArrayBegin();
					serializer.deserializeArrayItem(uniform.vec3[0], 0);
					serializer.deserializeArrayItem(uniform.vec3[1], 0);
					serializer.deserializeArrayItem(uniform.vec3[2], 0);
					serializer.deserializeArrayEnd();
				}
				else if (StringUnitl::equalStrings(label, "vec2"))
				{
					serializer.deserializeArrayBegin();
					serializer.deserializeArrayItem(uniform.vec2[0], 0);
					serializer.deserializeArrayItem(uniform.vec2[1], 0);
					serializer.deserializeArrayEnd();
				}
				else
				{
					log_waring("Renderer Unknown label %s.",label);
				}
			}
			serializer.deserializeObjectEnd();
		}
		serializer.deserializeArrayEnd();
	}


	void Material::setTexturePath(e_int32 i, const ArchivePath& path)
	{
		if (path.length() == 0)
		{
			setTexture(i, nullptr);
		}
		else
		{
			Texture* texture = static_cast<Texture*>(m_resource_manager.getOwner().get(TEXTURE_TYPE)->load(path));
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

		if (texture) addDependency(*texture);
		m_textures[i] = texture;
		if (i >= m_texture_count) m_texture_count = i + 1;

		if (old_texture)
		{
			removeDependency(*old_texture);
			m_resource_manager.getOwner().get(TEXTURE_TYPE)->unload(*old_texture);
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
		Shader* shader = static_cast<Shader*>(m_resource_manager.getOwner().get(SHADER_TYPE)->load(path));
		setShader(shader);
	}

	void Material::createCommandBuffer()
	{
		if (m_command_buffer != &DEFAULT_COMMAND_BUFFER) m_allocator.deallocate(m_command_buffer);
		m_command_buffer = &DEFAULT_COMMAND_BUFFER;
		if (!m_shader) return;

		CommandBufferGenerator generator;

		for (e_int32 i = 0; i < m_shader->m_uniforms.size(); ++i)
		{
			const Material::Uniform& uniform = m_uniforms[i];
			const Shader::Uniform& shader_uniform = m_shader->m_uniforms[i];

			switch (shader_uniform.type)
			{
			case Shader::Uniform::e_float:
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
			if (i >= m_texture_count || !m_textures[i]) continue;

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
			m_resource_manager.getOwner().get(SHADER_TYPE)->unload(*shader);
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

	e_bool Material::deserializeTexture(JsonSerializer& serializer, const e_char* material_dir)
	{
		e_char path[MAX_PATH_LENGTH];
		serializer.deserializeObjectBegin();
		e_char label[256];
		e_bool keep_data = false;
		e_uint32 flags = 0;

		while (!serializer.isObjectEnd())
		{
			serializer.deserializeLabel(label, sizeof(label));
			if (StringUnitl::equalStrings(label, "source"))
			{
				serializer.deserialize(path, MAX_PATH_LENGTH, "");
				if (path[0] != '\0')
				{
					e_char texture_path[MAX_PATH_LENGTH];
					if (path[0] != '/' && path[0] != '\\')
					{
						StringUnitl::copyString(texture_path, material_dir);
						StringUnitl::catString(texture_path, path);
					}
					else
					{
						StringUnitl::copyString(texture_path, path);
					}
					auto* mng = m_resource_manager.getOwner().get(TEXTURE_TYPE);
					m_textures[m_texture_count] = static_cast<Texture*>(mng->load(ArchivePath(texture_path)));
					addDependency(*m_textures[m_texture_count]);
				}
			}
			else if (StringUnitl::equalStrings(label, "min_filter"))
			{
				serializer.deserialize(label, sizeof(label), "");
				if (StringUnitl::equalStrings(label, "point"))
				{
					flags |= BGFX_TEXTURE_MIN_POINT;
				}
				else if (StringUnitl::equalStrings(label, "anisotropic"))
				{
					flags |= BGFX_TEXTURE_MIN_ANISOTROPIC;
				}
				else
				{
					log_error("Renderer Unknown texture filter %s in material %s.", label, getPath());
				}
			}
			else if (StringUnitl::equalStrings(label, "mag_filter"))
			{
				serializer.deserialize(label, sizeof(label), "");
				if (StringUnitl::equalStrings(label, "point"))
				{
					flags |= BGFX_TEXTURE_MAG_POINT;
				}
				else if (StringUnitl::equalStrings(label, "anisotropic"))
				{
					flags |= BGFX_TEXTURE_MAG_ANISOTROPIC;
				}
				else
				{
					log_error("Renderer Unknown texture filter %s in material %s .", label, getPath());
				}
			}
			else if (StringUnitl::equalStrings(label, "u_clamp"))
			{
				e_bool b;
				serializer.deserialize(b, false);
				if (b) flags |= BGFX_TEXTURE_U_CLAMP;
			}
			else if (StringUnitl::equalStrings(label, "v_clamp"))
			{
				e_bool b;
				serializer.deserialize(b, false);
				if (b) flags |= BGFX_TEXTURE_V_CLAMP;
			}
			else if (StringUnitl::equalStrings(label, "w_clamp"))
			{
				e_bool b;
				serializer.deserialize(b, false);
				if (b) flags |= BGFX_TEXTURE_W_CLAMP;
			}
			else if (StringUnitl::equalStrings(label, "keep_data"))
			{
				serializer.deserialize(keep_data, false);
			}
			else if (StringUnitl::equalStrings(label, "srgb"))
			{
				e_bool is_srgb;
				serializer.deserialize(is_srgb, false);
				if (is_srgb) flags |= BGFX_TEXTURE_SRGB;
			}
			else
			{
				log_error("Rendere Unknown data %s in material %s",label, getPath());
				return false;
			}
		}
		if (m_textures[m_texture_count])
		{
			m_textures[m_texture_count]->setFlags(flags);

			if (keep_data)
			{
				m_textures[m_texture_count]->addDataReference();
			}
		}
		serializer.deserializeObjectEnd();
		++m_texture_count;
		return true;
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


	e_bool Material::load(FS::IFile& file)
	{
		//PROFILE_FUNCTION();

		m_render_states = BGFX_STATE_CULL_CW;
		setAlphaRef(DEFAULT_ALPHA_REF_VALUE);
		m_uniforms.clear();
		JsonSerializer serializer(file, JsonSerializer::READ, getPath(), m_allocator);
		serializer.deserializeObjectBegin();
		e_char label[256];
		e_char material_dir[MAX_PATH_LENGTH];
		StringUnitl::getDir(material_dir, MAX_PATH_LENGTH, getPath().c_str());
		while (!serializer.isObjectEnd())
		{
			serializer.deserializeLabel(label, 255);
			if (StringUnitl::equalStrings(label, "defines"))
			{
				deserializeDefines(serializer);
			}
			else if (StringUnitl::equalStrings(label, "custom_flags"))
			{
				deserializeCustomFlags(serializer);
			}
			else if (StringUnitl::equalStrings(label, "layers_count"))
			{
				serializer.deserialize(m_layers_count, 0);
			}
			else if (StringUnitl::equalStrings(label, "render_layer"))
			{
				e_char tmp[32];
				auto& renderer = static_cast<MaterialManager&>(m_resource_manager).getRenderer();
				serializer.deserialize(tmp, TlengthOf(tmp), "default");
				m_render_layer = renderer.getLayer(tmp);
				m_render_layer_mask = 1ULL << (e_uint64)m_render_layer;
			}
			else if (StringUnitl::equalStrings(label, "uniforms"))
			{
				deserializeUniforms(serializer);
			}
			else if (StringUnitl::equalStrings(label, "texture"))
			{
				if (!deserializeTexture(serializer, material_dir))
				{
					return false;
				}
			}
			else if (StringUnitl::equalStrings(label, "alpha_ref"))
			{
				serializer.deserialize(m_alpha_ref, 0.3f);
			}
			else if (StringUnitl::equalStrings(label, "backface_culling"))
			{
				e_bool b = true;
				serializer.deserialize(b, true);
				if (b)
				{
					m_render_states |= BGFX_STATE_CULL_CW;
				}
				else
				{
					m_render_states &= ~BGFX_STATE_CULL_MASK;
				}
			}
			else if (StringUnitl::equalStrings(label, "color"))
			{
				serializer.deserializeArrayBegin();
				serializer.deserializeArrayItem(m_color.x, 1.0f);
				serializer.deserializeArrayItem(m_color.y, 1.0f);
				serializer.deserializeArrayItem(m_color.z, 1.0f);
				if (!serializer.isArrayEnd())
				{
					serializer.deserializeArrayItem(m_color.w, 1.0f);
				}
				else
				{
					m_color.w = 1;
				}
				serializer.deserializeArrayEnd();
			}
			else if (StringUnitl::equalStrings(label, "metallic"))
			{
				serializer.deserialize(m_metallic, 0.0f);
			}
			else if (StringUnitl::equalStrings(label, "roughness"))
			{
				serializer.deserialize(m_roughness, 1.0f);
			}
			else if (StringUnitl::equalStrings(label, "shader"))
			{
				ArchivePath path;
				serializer.deserialize(path, ArchivePath(""));
				auto* manager = m_resource_manager.getOwner().get(SHADER_TYPE);
				setShader(static_cast<Shader*>(manager->load(ArchivePath(path))));
			}
			else
			{
				log_error("Renderer Unknown parameter %s in material %s", label, getPath());
			}
		}
		serializer.deserializeObjectEnd();

		if (!m_shader)
		{
			log_error("Renderer Material %s without a shader.", getPath());
			return false;
		}

		m_size = file.size();
		return true;
	}
}

namespace egal
{
	Resource* MaterialManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Material)(path, *this, m_allocator);
	}

	e_void MaterialManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Material*>(&resource));
	}
}