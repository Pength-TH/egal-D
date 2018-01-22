
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
			case Shader::Uniform::FLOAT:
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
					auto* mng = m_resource_manager.getOwner().get( RESOURCE_TEXTURE_TYPE);
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
				auto* manager = m_resource_manager.getOwner().get( RESOURCE_SHADER_TYPE);
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