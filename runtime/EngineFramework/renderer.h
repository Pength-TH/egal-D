#ifndef _render_h_
#define _render_h_
#pragma once
#include "common/type.h"
#include "common/egal_string.h"

#include "common/resource/resource_define.h"
#include "common/resource/shader_manager.h"
#include "common/resource/entity_manager.h"
#include "common/resource/material_manager.h"
#include "common/resource/texture_manager.h"

#include "common/allocator/bgfx_allocator.h"
#include "runtime/EngineFramework/plugin_manager.h"

namespace bgfx
{
	struct UniformHandle;
	struct VertexDecl;
}

namespace egal
{
	class EngineRoot;
	struct FontAtlas;
	class Pipeline;
	class ArchivePath;
	class Shader;
	class Material;
	
	struct bgfx_allocator;

	struct CallbackStub : public bgfx::CallbackI
	{
		explicit CallbackStub(Renderer& renderer)
			: m_renderer(renderer)
		{}

		e_void setThreadName();
		e_void profilerBegin(const e_char* _name, uint32_t _abgr, const e_char* _filePath, uint16_t _line);
		e_void profilerBeginLiteral(const e_char* _name, uint32_t _abgr, const e_char* _filePath, uint16_t _line);
		e_void profilerEnd();
		e_void captureBegin(e_uint32, e_uint32, e_uint32, bgfx::TextureFormat::Enum, e_bool);
		e_void fatal(bgfx::Fatal::Enum _code, const e_char* _str);
		e_void traceVargs(const e_char* _filePath, e_uint16 _line, const e_char* _format, va_list _argList);
		e_void screenShot(const e_char* filePath,
			uint32_t width,
			uint32_t height,
			uint32_t pitch,
			const e_void* data,
			uint32_t size,
			e_bool yflip);

		e_uint32 cacheReadSize(e_uint64) { return 0; }
		e_bool cacheRead(e_uint64, e_void*, e_uint32) { return false; }
		e_void cacheWrite(e_uint64, const e_void*, e_uint32) {}
		e_void captureEnd() { ASSERT(false); }
		e_void captureFrame(const e_void*, e_uint32) { ASSERT(false); }

		e_bool m_is_thread_name_set = false;
		Renderer& m_renderer;
	};

	class Renderer
	{
	public:
		/** ¥¥Ω®‰÷»æ */
		static Renderer* create(EngineRoot& engine);

		/** –∂‘ÿ‰÷»æ */
		static void destroy(Renderer* render, IAllocator& allocator);

	public:
		Renderer(EngineRoot& engine);
		~Renderer();

		const char* getName() const;
		void createScenes(ComponentManager&);
		void destroyScene(SceneManager*);

		void frame(e_bool capture);
		void resize(e_int32 width, e_int32 height);

		int getViewCounter() const;
		void viewCounterAdd();

		void makeScreenshot(const ArchivePath& filename);

		e_int32 getPassIdx(const e_char* pass);
		const e_char* getPassName(e_int32 idx);

		e_uint8 getShaderDefineIdx(const e_char* define);
		const e_char* getShaderDefine(e_int32 define_idx);
		int getShaderDefinesCount() const;

		const bgfx::VertexDecl& getBasicVertexDecl() const;
		const bgfx::VertexDecl& getBasic2DVertexDecl() const;

		MaterialManager& getMaterialManager();
		EntityManager& getEntityManager();
		TextureManager& getTextureManager();
		Shader* getDefaultShader();

		const UniformHandle& getMaterialColorUniform() const;
		const UniformHandle& getRoughnessMetallicUniform() const;

		int getLayersCount() const;
		int getLayer(const e_char* name);
		const e_char* getLayerName(e_int32 idx) const;

		void setMainPipeline(Pipeline* pipeline);
		Pipeline* getMainPipeline();

		EngineRoot& getEngine();

	private:
		using ShaderDefine = StaticString<32>;
		using Layer = StaticString<32>;

		EngineRoot&		m_engine;
		IAllocator&		m_allocator;
		
		Material*		m_draw2d_material;
		Pipeline*		m_main_pipeline;
		Shader*			m_default_shader;

		bgfx_allocator	m_bgfx_allocator;
		CallbackStub	m_callback_stub;

		TextureManager		m_texture_manager;
		MaterialManager		m_material_manager;
		ShaderManager		m_shader_manager;
		ShaderBinaryManager m_shader_binary_manager;
		EntityManager		m_entity_manager;

		TVector<ShaderCombinations::Pass>	m_passes;
		TVector<ShaderDefine>				m_shader_defines;
		TVector<Layer>						m_layers;

		e_uint32	m_current_pass_hash;
		e_int32		m_view_counter;
		e_bool		m_vsync;

		bgfx::VertexDecl	m_basic_vertex_decl;
		bgfx::VertexDecl	m_basic_2d_vertex_decl;

		bgfx::UniformHandle m_mat_color_uniform;
		bgfx::UniformHandle m_roughness_metallic_uniform;
	};
}

#endif
