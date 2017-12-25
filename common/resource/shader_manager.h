#ifndef _shader_manager_h_
#define _shader_manager_h_

#include "common/resource/resource_public.h"
#include "common/resource/resource_manager.h"
#include "common/resource/resource_define.h"

namespace egal
{
	class Renderer;
	class Shader;
	class ShaderBinary;

	struct ShaderInstance
	{
		explicit ShaderInstance(Shader& _shader)
			: shader(_shader)
		{
			for (e_int32 i = 0; i < TlengthOf(program_handles); ++i)
			{
				program_handles[i] = INVALID_HANDLE;
				binaries[i * 2] = nullptr;
				binaries[i * 2 + 1] = nullptr;
			}
		}
		~ShaderInstance();
		egal::ProgramHandle getProgramHandle(e_int32 pass_idx);

		egal::ProgramHandle program_handles[32];
		ShaderBinary*		binaries[64];
		e_uint32			define_mask;
		Shader&				shader;
	};

	struct ShaderCombinations
	{
		ShaderCombinations();

		typedef e_char Define[40];
		typedef StaticString<20> Pass;
		typedef e_uint8 Defines[16];
		typedef Pass Passes[32];

		e_int32		pass_count;
		e_int32		define_count;
		e_int32		vs_local_mask[32];
		e_int32		fs_local_mask[32];
		Defines		defines;
		Passes		passes;
		e_uint32	all_defines_mask;
	};

	class ShaderBinary : public Resource
	{
	public:
		ShaderBinary(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
		egal::ShaderHandle getHandle() { return m_handle; }

		Shader* m_shader;

	private:
		e_void unload() override;
		e_bool load(FS::IFile& file) override;

	private:
		egal::ShaderHandle m_handle;
	};

	class Shader : public Resource
	{
		friend struct ShaderInstance;
	public:
		struct TextureSlot
		{
			TextureSlot()
			{
				name[0] = uniform[0] = '\0';
				define_idx = -1;
				uniform_handle = INVALID_HANDLE;
			}
			e_char				name[30];
			e_char				uniform[30];
			e_int32				define_idx;
			egal::UniformHandle uniform_handle;
		};
	
		struct Uniform
		{
			enum Type
			{
				INT32,
				e_float,
				MATRIX4,
				TIME,
				COLOR,
				FLOAT2,
				FLOAT3
			};

			e_char name[32];
			e_uint32 name_hash;
			Type type;
			egal::UniformHandle handle;
		};
		
		static const e_int32 MAX_TEXTURE_SLOT_COUNT = 16;

	public:
		Shader(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
		~Shader();

		e_bool hasDefine(e_uint8 define_idx) const;
		ShaderInstance& getInstance(e_uint32 mask);
		Renderer& getRenderer();

		static e_bool getShaderCombinations(const e_char* shd_path,
			Renderer& renderer,
			const e_char* shader_content,
			ShaderCombinations* output);

		e_void onBeforeEmpty() override;

		IAllocator& m_allocator;
		TVector<ShaderInstance> m_instances;
		e_uint32 m_all_defines_mask;
		ShaderCombinations m_combintions;
		e_uint64 m_render_states;
		TextureSlot m_texture_slots[MAX_TEXTURE_SLOT_COUNT];
		e_int32 m_texture_slot_count;
		TVector<Uniform> m_uniforms;

	private:
		e_bool generateInstances();

		e_void unload() override;
		e_bool load(FS::IFile& file) override;
	};
}

namespace egal
{
	class ShaderBinaryManager : public ResourceManagerBase
	{
	public:
		ShaderBinaryManager(Renderer& renderer, IAllocator& allocator);
		~ShaderBinaryManager();

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
	};

	class ShaderManager : public ResourceManagerBase
	{
	public:
		ShaderManager(Renderer& renderer, IAllocator& allocator);
		~ShaderManager();

		Renderer& getRenderer();
		e_uint8* getBuffer(e_int32 size);

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
		e_uint8* m_buffer;
		e_int32 m_buffer_size;
		Renderer& m_renderer;
	};
}

#endif
