#ifndef _material_manager_h_
#define _material_manager_h_

#include "common/resource/resource_manager.h"
#include "common/resource/resource_define.h"

namespace egal
{
	class Renderer;
	class Texture;
	struct ShaderInstance;

	class Material : public Resource
	{
		friend class MaterialManager;
	public:
		enum class DepthFunc
		{
			LEQUAL,
			LESS
		};

		struct Uniform
		{
			e_uint32 name_hash;
			union
			{
				e_int32 int_value;
				e_float float_value;
				e_float vec3[3];
				e_float vec2[2];
				e_float matrix[16];
			};
		};

	public:
		Material(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
		~Material();

		e_float getMetallic() const { return m_metallic; }
		e_void setMetallic(e_float value) { m_metallic = value; createCommandBuffer(); }
		e_float getRoughness() const { return m_roughness; }
		e_void setRoughness(e_float value) { m_roughness = value; createCommandBuffer(); }
		float4 getColor() const { return m_color; }
		e_void setColor(const float4& color) { m_color = color;  createCommandBuffer(); }
		e_float getAlphaRef() const { return m_alpha_ref; }
		e_void setAlphaRef(e_float value);
		e_uint64 getRenderStates() const { return m_render_states; }
		e_void enableBackfaceCulling(e_bool enable);
		e_bool isBackfaceCulling() const;

		e_void setShader(Shader* shader);
		e_void setShader(const ArchivePath& path);
		Shader* getShader() const { return m_shader; }

		e_int32 getTextureCount() const { return m_texture_count; }
		Texture* getTexture(e_int32 i) const { return i < m_texture_count ? m_textures[i] : nullptr; }
		const e_char* getTextureUniform(e_int32 i);
		Texture* getTextureByUniform(const e_char* uniform) const;
		e_bool isTextureDefine(e_uint8 define_idx) const;
		e_void setTexture(e_int32 i, Texture* texture);
		e_void setTexturePath(e_int32 i, const ArchivePath& path);
		e_bool save(JsonSerializer& serializer);
		e_int32 getUniformCount() const { return m_uniforms.size(); }
		Uniform& getUniform(e_int32 index) { return m_uniforms[index]; }
		const Uniform& getUniform(e_int32 index) const { return m_uniforms[index]; }
		ShaderInstance& getShaderInstance() { ASSERT(m_shader_instance); return *m_shader_instance; }
		const ShaderInstance& getShaderInstance() const { ASSERT(m_shader_instance); return *m_shader_instance; }
		const e_uint8* getCommandBuffer() const { return m_command_buffer; }
		e_void createCommandBuffer();
		e_int32 getRenderLayer() const { return m_render_layer; }
		e_void setRenderLayer(e_int32 layer);
		e_uint64 getRenderLayerMask() const { return m_render_layer_mask; }
		e_int32 getLayersCount() const { return m_layers_count; }
		e_void setLayersCount(e_int32 layers);

		e_void setDefine(e_uint8 define_idx, e_bool enabled);
		e_bool hasDefine(e_uint8 define_idx) const;
		e_bool isDefined(e_uint8 define_idx) const;

		e_void setCustomFlag(e_uint32 flag) { m_custom_flags |= flag; }
		e_void unsetCustomFlag(e_uint32 flag) { m_custom_flags &= ~flag; }
		e_bool isCustomFlag(e_uint32 flag) const { return (m_custom_flags & flag) == flag; }

		static e_uint32 getCustomFlag(const e_char* flag_name);
		static const e_char* getCustomFlagName(e_int32 index);
		static e_int32 getCustomFlagCount();

	private:
		e_void onBeforeReady() override;
		e_void unload() override;
		e_bool load(FS::IFile& file) override;

		e_bool deserializeTexture(JsonSerializer& serializer, const e_char* material_dir);
		e_void deserializeUniforms(JsonSerializer& serializer);
		e_void deserializeDefines(JsonSerializer& serializer);
		e_void deserializeCustomFlags(JsonSerializer& serializer);

	private:
		static const e_int32 MAX_TEXTURE_COUNT = 16;

		Shader* m_shader;
		ShaderInstance* m_shader_instance;
		Texture* m_textures[MAX_TEXTURE_COUNT];
		e_int32 m_texture_count;
		TVector<Uniform> m_uniforms;
		IAllocator& m_allocator;
		e_uint64 m_render_states;
		float4 m_color;
		e_float m_alpha_ref;
		e_float m_metallic;
		e_float m_roughness;
		e_uint32 m_define_mask;
		e_uint8* m_command_buffer;
		e_uint32 m_custom_flags;
		e_int32 m_render_layer;
		e_uint64 m_render_layer_mask;
		e_int32 m_layers_count;
	};
}

namespace egal
{
	class Renderer;
	class MaterialManager : public ResourceManagerBase
	{
	public:
		MaterialManager(Renderer& renderer, IAllocator& allocator)
			: ResourceManagerBase(allocator)
			, m_renderer(renderer)
			, m_allocator(allocator)
		{}
		~MaterialManager() {}

		Renderer& getRenderer() { return m_renderer; }

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
		Renderer& m_renderer;
	};
}
#endif
