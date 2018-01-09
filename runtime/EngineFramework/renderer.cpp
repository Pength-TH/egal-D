#include "runtime/EngineFramework/renderer.h"
#include "runtime/EngineFramework/plugin_manager.h"

#include "common/resource/resource_public.h"
#include "common/resource/resource_manager.h"
#include "common/egal-d.h"

#include "runtime/EngineFramework/buffer.h"
#include "runtime/EngineFramework/component_manager.h"
#include "runtime/EngineFramework/engine_root.h"
#include "runtime/EngineFramework/scene_manager.h"
#include "runtime/EngineFramework/renderer.h"
#include "runtime/EngineFramework/culling_system.h"
#include "runtime/EngineFramework/pipeline.h"

#include "common/lua/lua_function.h"
#include "common/lua/lua_manager.h"

#include "common/filesystem/os_file.h"

#include "common/resource/resource_define.h"
#include <cstdio>
#include <bgfx/platform.h>

namespace egal
{
	struct BoneProperty : Reflection::IEnumProperty
	{
		BoneProperty() { name = "Bone"; }

		e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const
		{
			SceneManager* scene = static_cast<SceneManager*>(cmp.scene);
			e_int32 value = scene->getBoneAttachmentBone(cmp.handle);
			stream.write(value);
		}


		e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const
		{
			SceneManager* scene = static_cast<SceneManager*>(cmp.scene);
			e_int32 value = stream.read<e_int32>();
			scene->setBoneAttachmentBone(cmp.handle, value);
		}


		ComponentHandle getModelInstance(SceneManager* render_scene, ComponentHandle bone_attachment_cmp) const
		{
			GameObject parent_entity = render_scene->getBoneAttachmentParent(bone_attachment_cmp);
			if (parent_entity == INVALID_GAME_OBJECT) return INVALID_COMPONENT;
			ComponentHandle model_instance = render_scene->getEntityInstanceComponent(parent_entity);
			return model_instance;
		}


		e_int32 getEnumCount(ComponentUID cmp) const
		{
			SceneManager* render_scene = static_cast<SceneManager*>(cmp.scene);
			ComponentHandle model_instance = getModelInstance(render_scene, cmp.handle);
			if (model_instance == INVALID_COMPONENT) return 0;
			auto* model = render_scene->getEntityInstanceEntity(model_instance);
			if (!model || !model->isReady()) return 0;
			return model->getBoneCount();
		}


		const e_char* getEnumName(ComponentUID cmp, e_int32 index) const
		{
			SceneManager* render_scene = static_cast<SceneManager*>(cmp.scene);
			ComponentHandle model_instance = getModelInstance(render_scene, cmp.handle);
			if (model_instance == INVALID_COMPONENT) return "";
			auto* model = render_scene->getEntityInstanceEntity(model_instance);
			if (!model) return "";
			return model->getBone(index).name.c_str();
		}
	};

	e_void CallbackStub::fatal(bgfx::Fatal::Enum _code, const e_char* _str)
	{
		switch (_code)
		{
		case bgfx::Fatal::DebugCheck:
			log_error("Renderer Check %s", _str);
			break;
		case bgfx::Fatal::InvalidShader:
			log_error("Shader %s", _str);
			break;
		case bgfx::Fatal::UnableToInitialize:
			log_error("Initialize %s", _str);
			break;
		case bgfx::Fatal::UnableToCreateTexture:
			log_error("Create Texture: %s", _str);
			break;
		case bgfx::Fatal::DeviceLost:
			log_error("Device Lost: %s", _str);
			break;
		case bgfx::Fatal::Count:
			break;
		default:
			break;
		}

		if (bgfx::Fatal::DebugCheck == _code || bgfx::Fatal::InvalidShader == _code)
		{
			Debug::debugBreak();
		}
		else
		{
			abort();
		}
	}


	e_void CallbackStub::traceVargs(const e_char* _filePath,
		e_uint16 _line,
		const e_char* _format,
		va_list _argList)
	{
	}


	e_void CallbackStub::screenShot(const e_char* filePath,
		uint32_t width,
		uint32_t height,
		uint32_t pitch,
		const e_void* data,
		uint32_t size,
		e_bool yflip)
	{
		TGAHeader header;
		StringUnitl::setMemory(&header, 0, sizeof(header));
		e_int32 bytes_per_pixel = 4;
		header.bitsPerPixel = (e_char)(bytes_per_pixel * 8);
		header.height = (short)height;
		header.width = (short)width;
		header.dataType = 2;

		FS::OsFile file;
		if (!file.open(filePath, FS::Mode::CREATE_AND_WRITE))
		{
			log_error("Renderer Failed to save screenshot to %s.", filePath);
			return;
		}
		file.write(&header, sizeof(header));

		for (e_uint32 i = 0; i < height; ++i)
			file.write((const e_uint8*)data + pitch * i, width * 4);
		file.close();
	}


	e_void CallbackStub::captureBegin(e_uint32,
		e_uint32,
		e_uint32,
		bgfx::TextureFormat::Enum,
		e_bool)
	{
		ASSERT(false);
	}

	e_void CallbackStub::setThreadName()
	{
		if (m_is_thread_name_set) return;
		m_is_thread_name_set = true;
		Profiler::setThreadName("bgfx thread");
	}

	e_void CallbackStub::profilerBegin(
		const e_char* _name
		, uint32_t _abgr
		, const e_char* _filePath
		, uint16_t _line
	)
	{
		setThreadName();
		Profiler::beginBlock("bgfx_dynamic");
	}

	e_void CallbackStub::profilerBeginLiteral(
		const e_char* _name
		, uint32_t _abgr
		, const e_char* _filePath
		, uint16_t _line
	)
	{
		setThreadName();
		Profiler::beginBlock(_name);
	}


	e_void CallbackStub::profilerEnd()
	{
		Profiler::endBlock();
	}

	static e_void registerProperties(IAllocator& allocator)
	{
		using namespace Reflection;

		static auto render_scene = scene("renderer",
			component("bone_attachment",
				property("Parent", PROPERITY(SceneManager, getBoneAttachmentParent, setBoneAttachmentParent)),
				property("Relative position", PROPERITY(SceneManager, getBoneAttachmentPosition, setBoneAttachmentPosition)),
				property("Relative rotation", PROPERITY(SceneManager, getBoneAttachmentRotation, setBoneAttachmentRotation),
					RadiansAttribute()),
				BoneProperty()
			),
			component("camera",
				property("Slot", PROPERITY(SceneManager, getCameraSlot, setCameraSlot)),
				property("Orthographic size", PROPERITY(SceneManager, getCameraOrthoSize, setCameraOrthoSize),
					MinAttribute(0)),
				property("Orthographic", PROPERITY(SceneManager, isCameraOrtho, setCameraOrtho)),
				property("FOV", PROPERITY(SceneManager, getCameraFOV, setCameraFOV),
					RadiansAttribute()),
				property("Near", PROPERITY(SceneManager, getCameraNearPlane, setCameraNearPlane),
					MinAttribute(0)),
				property("Far", PROPERITY(SceneManager, getCameraFarPlane, setCameraFarPlane),
					MinAttribute(0))
			),
			component("render_able",
				property("Source", PROPERITY(SceneManager, getEntityInstancePath, setEntityInstancePath),
					ResourceAttribute("Mesh (*.msh)",  RESOURCE_ENTITY_TYPE)),
				property("Keep skin", PROPERITY(SceneManager, getEntityInstanceKeepSkin, setEntityInstanceKeepSkin)),
				const_array("Materials", &SceneManager::getEntityInstanceMaterialsCount,
					property("Source", PROPERITY(SceneManager, getEntityInstanceMaterial, setEntityInstanceMaterial),
						ResourceAttribute("Material (*.mat)", RESOURCE_MATERIAL_TYPE))
				)
			),
			component("global_light",
				property("Color", PROPERITY(SceneManager, getGlobalLightColor, setGlobalLightColor),
					ColorAttribute()),
				property("Intensity", PROPERITY(SceneManager, getGlobalLightIntensity, setGlobalLightIntensity),
					MinAttribute(0)),
				property("Indirect intensity", PROPERITY(SceneManager, getGlobalLightIndirectIntensity, setGlobalLightIndirectIntensity), MinAttribute(0)),
				property("Fog density", PROPERITY(SceneManager, getFogDensity, setFogDensity),
					ClampAttribute(0, 1)),
				property("Fog bottom", PROPERITY(SceneManager, getFogBottom, setFogBottom)),
				property("Fog height", PROPERITY(SceneManager, getFogHeight, setFogHeight),
					MinAttribute(0)),
				property("Fog color", PROPERITY(SceneManager, getFogColor, setFogColor),
					ColorAttribute()),
				property("Shadow cascades", PROPERITY(SceneManager, getShadowmapCascades, setShadowmapCascades))
			),
			component("point_light",
				property("Diffuse color", PROPERITY(SceneManager, getPointLightColor, setPointLightColor),
					ColorAttribute()),
				property("Specular color", PROPERITY(SceneManager, getPointLightSpecularColor, setPointLightSpecularColor),
					ColorAttribute()),
				property("Diffuse intensity", PROPERITY(SceneManager, getPointLightIntensity, setPointLightIntensity),
					MinAttribute(0)),
				property("Specular intensity", PROPERITY(SceneManager, getPointLightSpecularIntensity, setPointLightSpecularIntensity), MinAttribute(0)),
				property("FOV", PROPERITY(SceneManager, getLightFOV, setLightFOV),
					ClampAttribute(0, 360),
					RadiansAttribute()),
				property("Attenuation", PROPERITY(SceneManager, getLightAttenuation, setLightAttenuation),
					ClampAttribute(0, 1000)),
				property("Range", PROPERITY(SceneManager, getLightRange, setLightRange),
					MinAttribute(0)),
				property("Cast shadows", PROPERITY(SceneManager, getLightCastShadows, setLightCastShadows),
					MinAttribute(0))
			),
			component("decal",
				property("Material", PROPERITY(SceneManager, getDecalMaterialPath, setDecalMaterialPath),
					ResourceAttribute("Material (*.mat)", RESOURCE_MATERIAL_TYPE)),
				property("Scale", PROPERITY(SceneManager, getDecalScale, setDecalScale),
					MinAttribute(0))
			)
		);
		registerScene(render_scene);
	}

	Renderer::Renderer(EngineRoot& engine)
		: m_engine(engine)
		, m_allocator(engine.getAllocator())
		, m_texture_manager(m_allocator)
		, m_entity_manager(*this, m_allocator)
		, m_material_manager(*this, m_allocator)
		, m_shader_manager(*this, m_allocator)
		, m_shader_binary_manager(*this, m_allocator)
		, m_passes(m_allocator)
		, m_shader_defines(m_allocator)
		, m_layers(m_allocator)
		, m_bgfx_allocator(m_allocator)
		, m_callback_stub(*this)
		, m_vsync(true)
		, m_main_pipeline(nullptr)
	{
		registerProperties(engine.getAllocator());

		bgfx::PlatformData d;
		e_void* window_handle = engine.getPlatformData().window_handle;
		e_void* display = engine.getPlatformData().display;
		if (window_handle)
		{
			StringUnitl::setMemory(&d, 0, sizeof(d));
			d.nwh = window_handle;
			d.ndt = display;
			bgfx::setPlatformData(d);
		}
		e_char cmd_line[4096];
		bgfx::RendererType::Enum renderer_type = bgfx::RendererType::Direct3D11;
		getCommandLine(cmd_line, TlengthOf(cmd_line));

		m_vsync = false;

		e_bool res = bgfx::init(renderer_type, 0, 0, &m_callback_stub, &m_bgfx_allocator);
		if (!res)
		{
			log_error("Engine init failed.");
			return;
		}
		ASSERT(res);
		bgfx::reset(800, 600, m_vsync ? BGFX_RESET_VSYNC : 0);
		bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_PROFILER);
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

		ResourceManager& manager = engine.getResourceManager();
		m_texture_manager.create( RESOURCE_TEXTURE_TYPE, manager);
		m_entity_manager.create( RESOURCE_ENTITY_TYPE, manager);
		m_material_manager.create(RESOURCE_MATERIAL_TYPE, manager);
		m_shader_manager.create( RESOURCE_SHADER_TYPE, manager);
		m_shader_binary_manager.create( RESOURCE_SHADER_BINARY_TYPE, manager);

		m_current_pass_hash = crc32("MAIN");
		m_view_counter = 0;
		m_mat_color_uniform =
			bgfx::createUniform("u_materialColor", bgfx::UniformType::Vec4);
		m_roughness_metallic_uniform =
			bgfx::createUniform("u_roughnessMetallic", bgfx::UniformType::Vec4);

		m_basic_vertex_decl.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
		m_basic_2d_vertex_decl.begin()
			.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();

		m_default_shader = static_cast<Shader*>(m_shader_manager.load(ArchivePath("pipelines/common/default.shd")));
		//RenderScene::registerLuaAPI(m_engine.getState());

		m_layers.emplace("default");
		m_layers.emplace("transparent");
		m_layers.emplace("water");
		m_layers.emplace("fur");
	}


	Renderer::~Renderer()
	{
		Texture* draw2d_texture = m_draw2d_material->getTexture(0);
		m_draw2d_material->setTexture(0, nullptr);
		if (draw2d_texture)
		{
			draw2d_texture->destroy();
			_delete(m_engine.getAllocator(), draw2d_texture);
		}

		m_draw2d_material->getResourceManager().unload(*m_draw2d_material);

		m_shader_manager.unload(*m_default_shader);
		m_texture_manager.destroy();
		m_entity_manager.destroy();
		m_material_manager.destroy();
		m_shader_manager.destroy();
		m_shader_binary_manager.destroy();

		bgfx::destroy(m_mat_color_uniform);
		bgfx::destroy(m_roughness_metallic_uniform);
		bgfx::frame();
		bgfx::frame();
		bgfx::shutdown();
	}

	e_void Renderer::setMainPipeline(Pipeline* pipeline)
	{
		m_main_pipeline = pipeline;
	}


	Pipeline* Renderer::getMainPipeline()
	{
		return m_main_pipeline;
	}

	e_int32 Renderer::getLayer(const e_char* name)
	{
		for (e_int32 i = 0; i < m_layers.size(); ++i)
		{
			if (m_layers[i] == name) return i;
		}
		ASSERT(m_layers.size() < 64);
		m_layers.emplace() = name;
		return m_layers.size() - 1;
	}


	e_int32 Renderer::getLayersCount() const { return m_layers.size(); }
	const e_char* Renderer::getLayerName(e_int32 idx) const { return m_layers[idx]; }


	EntityManager& Renderer::getEntityManager() { return m_entity_manager; }
	MaterialManager& Renderer::getMaterialManager() { return m_material_manager; }
	TextureManager& Renderer::getTextureManager() { return m_texture_manager; }
	const bgfx::VertexDecl& Renderer::getBasicVertexDecl() const { return m_basic_vertex_decl; }
	const bgfx::VertexDecl& Renderer::getBasic2DVertexDecl() const { return m_basic_2d_vertex_decl; }


	e_void Renderer::createScenes(ComponentManager& ctx)
	{
		auto* scene = SceneManager::createInstance(*this, m_engine, ctx, m_allocator);
		ctx.addScene(scene);
	}


	e_void Renderer::destroyScene(SceneManager* scene) { SceneManager::destroyInstance(static_cast<SceneManager*>(scene)); }
	const e_char* Renderer::getName() const { return "renderer"; }
	EngineRoot& Renderer::getEngine() { return m_engine; }
	e_int32 Renderer::getShaderDefinesCount() const { return m_shader_defines.size(); }
	const e_char* Renderer::getShaderDefine(e_int32 define_idx) { return m_shader_defines[define_idx]; }
	const e_char* Renderer::getPassName(e_int32 idx) { return m_passes[idx]; }
	const bgfx::UniformHandle& Renderer::getMaterialColorUniform() const { return m_mat_color_uniform; }
	const bgfx::UniformHandle& Renderer::getRoughnessMetallicUniform() const { return m_roughness_metallic_uniform; }
	e_void Renderer::makeScreenshot(const ArchivePath& filename) { bgfx::requestScreenShot(BGFX_INVALID_HANDLE, filename.c_str()); }
	e_void Renderer::resize(e_int32 w, e_int32 h) { m_main_pipeline->resize(w, h); bgfx::reset(w, h, m_vsync ? BGFX_RESET_VSYNC : 0); }
	e_int32 Renderer::getViewCounter() const { return m_view_counter; }
	e_void Renderer::viewCounterAdd() { ++m_view_counter; }
	Shader* Renderer::getDefaultShader() { return m_default_shader; }


	e_uint8 Renderer::getShaderDefineIdx(const e_char* define)
	{
		for (e_int32 i = 0; i < m_shader_defines.size(); ++i)
		{
			if (m_shader_defines[i] == define)
			{
				ASSERT(i < 256);
				return i;
			}
		}

		m_shader_defines.emplace(define);
		return m_shader_defines.size() - 1;
	}


	e_int32 Renderer::getPassIdx(const e_char* pass)
	{
		for (e_int32 i = 0; i < m_passes.size(); ++i)
		{
			if (m_passes[i] == pass)
			{
				return i;
			}
		}

		m_passes.emplace(pass);
		return m_passes.size() - 1;
	}


	e_void Renderer::frame(e_bool capture)
	{
		//PROFILE_FUNCTION();
		bgfx::frame(capture);
		m_view_counter = 0;
	}
	//----------------------------------------------------------------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------------------------------------------------
		/** ´´½¨äÖÈ¾ */
	Renderer* Renderer::create(EngineRoot& engine)
	{
		return _aligned_new(engine.getAllocator(), Renderer)(engine);
	}

	/** Ð¶ÔØäÖÈ¾ */
	void Renderer::destroy(Renderer* render, IAllocator& allocator)
	{
		_delete(allocator, render);
	}
}
