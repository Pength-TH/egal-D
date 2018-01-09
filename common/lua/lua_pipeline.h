#ifndef _lua_pipeline_h_
#define _lua_pipeline_h_
#include "common/type.h"
#include "common/math/egal_math.h"

#include "lua_manager.h"

#include "common/resource/shader_manager.h"
#include "common/egal_string.h"

#include "runtime/EngineFramework/pipeline.h"
#include "runtime/EngineFramework/scene_manager.h"

namespace egal
{
	int lua_setViewMode(lua_State* L)
	{
		Pipeline* that = lua_manager::checkArg<Pipeline*>(L, 1);
		auto mode = (bgfx::ViewMode::Enum)lua_manager::checkArg<int>(L, 2);
		if (!that->m_current_view)
			return 0;
		bgfx::setViewMode(that->m_current_view->bgfx_id, mode);
		return 0;
	}


	int lua_bindFramebufferTexture(lua_State* L)
	{
		Pipeline* that = lua_manager::checkArg<Pipeline*>(L, 1);
		if (!that->m_current_view) 
			return 0;
		const char* framebuffer_name = lua_manager::checkArg<const char*>(L, 2);
		int renderbuffer_idx = lua_manager::checkArg<int>(L, 3);
		int uniform_idx = lua_manager::checkArg<int>(L, 4);
		e_uint32 flags = lua_gettop(L) > 4 ? lua_manager::checkArg<e_uint32>(L, 5) : 0xffffFFFF;

		FrameBuffer* fb = that->getFramebuffer(framebuffer_name);
		if (!fb) return 0;

		float4 size;
		size.x = (float)fb->getWidth();
		size.y = (float)fb->getHeight();
		size.z = 1.0f / (float)fb->getWidth();
		size.w = 1.0f / (float)fb->getHeight();
		that->m_current_view->command_buffer.beginAppend();
		if (that->m_global_textures_count == 0)
		{
			that->m_current_view->command_buffer.setUniform(that->m_texture_size_uniform, size);
		}
		that->m_current_view->command_buffer.setTexture(15 - that->m_global_textures_count,
			that->m_uniforms[uniform_idx],
			fb->getRenderbufferHandle(renderbuffer_idx),
			flags);
		++that->m_global_textures_count;
		that->m_current_view->command_buffer.end();
		return 0;
	}

	void lua_setDefine(Pipeline* pipeline)
	{
		if (!pipeline && pipeline->m_define.length() == 0)
			return;

		lua_State* L = pipeline->m_lua_state;
		StaticString<256> tmp(pipeline->m_define.c_str(), " = true");

		bool errors =
			luaL_loadbuffer(L, tmp, StringUnitl::stringLength(tmp.data), pipeline->m_path.c_str()) != LUA_OK;
		if (errors)
		{
			log_error("Renderer %s:%s.", pipeline->m_path.c_str(), lua_tostring(L, -1));
			lua_pop(L, 1);
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, pipeline->m_lua_env);
		lua_setupvalue(L, -2, 1);
		errors = lua_pcall(L, 0, 0, 0) != LUA_OK;
		if (errors)
		{
			log_error("Renderer %s:%s.", pipeline->m_path.c_str(), lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}

	void lua_exposeCustomCommandToLua(Pipeline* pipline, const Pipeline::CustomCommandHandler& handler)
	{
		if (!pipline && !pipline->m_lua_state)
			return;
		
		lua_State* L = pipline->m_lua_state;

		char tmp[1024];
		StringUnitl::copyString(tmp, "function ");
		StringUnitl::catString(tmp, handler.name);
		StringUnitl::catString(tmp, "(pipeline) executeCustomCommand(pipeline, \"");
		StringUnitl::catString(tmp, handler.name);
		StringUnitl::catString(tmp, "\") end");

		bool errors = luaL_loadbuffer(L, tmp, StringUnitl::stringLength(tmp), nullptr) != LUA_OK;
		errors = errors || lua_pcall(L, 0, 0, 0) != LUA_OK;

		if (errors)
		{
			log_error("Renderer %s.", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}

	int lua_newView(lua_State* L)
	{
		auto* pipeline = lua_manager::checkArg<Pipeline*>(L, 1);
		const char* debug_name = lua_manager::checkArg<const char*>(L, 2);
		const char* framebuffer_name = lua_manager::checkArg<const char*>(L, 3);
		e_uint64 layer_mask = 0;
		if (lua_gettop(L) > 3) layer_mask = lua_manager::checkArg<e_uint64>(L, 4);

		pipeline->m_layer_mask |= layer_mask;

		lua_manager::push(L, pipeline->newView(debug_name, layer_mask));
		pipeline->setFramebuffer(framebuffer_name);
		return 1;
	}

	static void lua_parseRenderbuffers(lua_State* L, FrameBuffer::Declaration& decl, Pipeline* pipeline)
	{
		decl.m_renderbuffers_count = 0;
		int len = (int)lua_rawlen(L, -1);
		for (int i = 0; i < len; ++i)
		{
			if (lua_rawgeti(L, -1, 1 + i) == LUA_TTABLE)
			{
				FrameBuffer::RenderBuffer& buf = decl.m_renderbuffers[decl.m_renderbuffers_count];
				bool is_shared = lua_getfield(L, -1, "shared") == LUA_TTABLE;
				if (is_shared)
				{
					StaticString<64> fb_name;
					int rb_idx;
					if (lua_getfield(L, -1, "fb") == LUA_TSTRING)
					{
						fb_name = lua_tostring(L, -1);
					}
					lua_pop(L, 1);
					if (lua_getfield(L, -1, "rb") == LUA_TNUMBER)
					{
						rb_idx = (int)lua_tonumber(L, -1);
					}
					lua_pop(L, 1);
					FrameBuffer* shared_fb = pipeline->getFramebuffer(fb_name);
					if (!shared_fb || rb_idx >= shared_fb->getRenderbuffersCounts())
					{
						buf.m_format = bgfx::TextureFormat::RGBA8;
						log_error("Renderer Can not share render buffer from %s , it does not exist.", fb_name);
					}
					else
					{
						buf.m_shared = &shared_fb->getRenderbuffer(rb_idx);
					}
				}
				lua_pop(L, 1);
				if (!is_shared) buf.parse(L);
				++decl.m_renderbuffers_count;
			}
			lua_pop(L, 1);
		}
	}

	int lua_addFramebuffer(lua_State* L)
	{
		auto* pipeline = lua_manager::checkArg<Pipeline*>(L, 1);
		const char* name = lua_manager::checkArg<const char*>(L, 2);
		FrameBuffer* framebuffer = pipeline->getFramebuffer(name);
		if (framebuffer)
		{
			log_waring("Renderer Trying to create already existing framebuffer %s.", name);
			return 0;
		}

		lua_manager::checkTableArg(L, 3);
		FrameBuffer::Declaration decl;
		StringUnitl::copyString(decl.m_name, name);

		lua_manager::getOptionalField(L, 3, "width", &decl.m_width);
		decl.m_size_ratio = float2(-1, -1);
		if (lua_getfield(L, 3, "size_ratio") == LUA_TTABLE)
		{
			decl.m_size_ratio = lua_manager::toType<float2>(L, -1);
		}
		lua_pop(L, 1);
		if (lua_getfield(L, 3, "screen_size") == LUA_TBOOLEAN)
		{
			bool is_screen_size = lua_toboolean(L, -1) != 0;
			decl.m_size_ratio = is_screen_size ? float2(1, 1) : float2(-1, -1);
		}
		lua_pop(L, 1);
		lua_manager::getOptionalField(L, 3, "height", &decl.m_height);

		if (lua_getfield(L, 3, "render_buffers") == LUA_TTABLE)
		{
			lua_parseRenderbuffers(L, decl, pipeline);
		}
		lua_pop(L, 1);
		if ((decl.m_size_ratio.x > 0 || decl.m_size_ratio.y > 0) && pipeline->m_height > 0)
		{
			decl.m_width = int(pipeline->m_width * decl.m_size_ratio.x);
			decl.m_height = int(pipeline->m_height * decl.m_size_ratio.y);
		}
		auto* fb = _aligned_new(pipeline->m_allocator, FrameBuffer)(decl);
		pipeline->m_framebuffers.push_back(fb);
		if (StringUnitl::equalStrings(decl.m_name, "default")) 
			pipeline->m_default_framebuffer = fb;

		return 0;
	}


	int lua_renderModels(lua_State* L)
	{
		auto* pipeline = lua_manager::checkArg<Pipeline*>(L, 1);

		ComponentHandle cam = pipeline->m_applied_camera;

		pipeline->renderAll(pipeline->m_camera_frustum, true, cam, pipeline->m_layer_mask);
		pipeline->m_layer_mask = 0;
		return 0;
	}


	void lua_logError(const char* message)
	{
		log_error("Renderer %s.", message);
	}


	int lua_setUniform(lua_State* L)
	{
		auto* pipeline = lua_manager::checkArg<Pipeline*>(L, 1);
		if (!pipeline->m_current_view) return 0;

		int uniform_idx = lua_manager::checkArg<int>(L, 2);
		lua_manager::checkTableArg(L, 3);

		float4 tmp[64];
		int len = Math::minimum((int)lua_rawlen(L, 3), TlengthOf(tmp));
		for (int i = 0; i < len; ++i)
		{
			if (lua_rawgeti(L, 3, 1 + i) == LUA_TTABLE)
			{
				if (lua_rawgeti(L, -1, 1) == LUA_TNUMBER) tmp[i].x = (float)lua_tonumber(L, -1);
				if (lua_rawgeti(L, -2, 2) == LUA_TNUMBER) tmp[i].y = (float)lua_tonumber(L, -1);
				if (lua_rawgeti(L, -3, 3) == LUA_TNUMBER) tmp[i].z = (float)lua_tonumber(L, -1);
				if (lua_rawgeti(L, -4, 4) == LUA_TNUMBER) tmp[i].w = (float)lua_tonumber(L, -1);
				lua_pop(L, 4);
			}
			lua_pop(L, 1);
		}

		if (uniform_idx >= pipeline->m_uniforms.size()) 
			luaL_argerror(L, 2, "unknown uniform");

		pipeline->m_current_view->command_buffer.beginAppend();
		pipeline->m_current_view->command_buffer.setUniform(pipeline->m_uniforms[uniform_idx], tmp, len);
		pipeline->m_current_view->command_buffer.end();
		return 0;
	}


	int lua_getRenderbuffer(lua_State* L)
	{
		auto* pipeline = lua_manager::checkArg<Pipeline*>(L, 1);
		const char* fb_name = lua_manager::checkArg<const char*>(L, 2);
		int rb_idx = lua_manager::checkArg<int>(L, 3);

		void* rb = &pipeline->getRenderbuffer(fb_name, rb_idx);
		lua_manager::push(L, rb);
		return 1;
	}


	int lua_renderLocalLightsShadowmaps(lua_State* L)
	{
		auto* pipeline = lua_manager::checkArg<Pipeline*>(L, 1);
		const char* camera_slot = lua_manager::checkArg<const char*>(L, 2);

		FrameBuffer* fbs[16];
		int len = Math::minimum((int)lua_rawlen(L, 3), TlengthOf(fbs));
		for (int i = 0; i < len; ++i)
		{
			if (lua_rawgeti(L, 3, 1 + i) == LUA_TSTRING)
			{
				const char* fb_name = lua_tostring(L, -1);
				fbs[i] = pipeline->getFramebuffer(fb_name);
			}
			lua_pop(L, 1);
		}

		SceneManager* scene = pipeline->m_scene;
		ComponentHandle camera = scene->getCameraInSlot(camera_slot);
		pipeline->renderLocalLightShadowmaps(camera, fbs, len);

		return 0;
	}


	void lua_print(int x, int y, const char* text)
	{
		bgfx::dbgTextPrintf(x, y, 0x4f, text);
	}

	void lua_callInitScene(Pipeline* pipeline)
	{
		lua_State* L = pipeline->m_lua_state;

		lua_rawgeti(L, LUA_REGISTRYINDEX, pipeline->m_lua_env);
		if (lua_getfield(L, -1, "initScene") == LUA_TFUNCTION)
		{
			lua_pushlightuserdata(L, pipeline);
			if (lua_pcall(L, 1, 0, 0) != LUA_OK)
			{
				log_waring("lua %s.", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		else
		{
			lua_pop(L, 1);
		}
	}

	static bool lua_frame(Pipeline* pipeline)
	{
		lua_State* L = pipeline->m_lua_state;

		lua_rawgeti(L, LUA_REGISTRYINDEX, pipeline->m_lua_env);
		bool success = true;
		if (lua_getfield(L, -1, "render") == LUA_TFUNCTION)
		{
			lua_pushlightuserdata(L, pipeline);
			if (lua_pcall(L, 1, 0, 0) != LUA_OK)
			{
				success = false;
				log_waring("Renderer %s.", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		else
		{
			lua_pop(L, 1);
		}
		return success;
	}

	void callInitScene(lua_State* L)
	{
		auto* pipeline = lua_manager::checkArg<Pipeline*>(L, 1);
		if (!pipeline)
			return;

		lua_rawgeti(L, LUA_REGISTRYINDEX, pipeline->m_lua_env);
		if (lua_getfield(L, -1, "initScene") == LUA_TFUNCTION)
		{
			lua_pushlightuserdata(L, pipeline);
			if (lua_pcall(L, 1, 0, 0) != LUA_OK)
			{
				log_error("lua %s.", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		else
		{
			lua_pop(L, 1);
		}
	}

	void lua_registerLuaAPI(lua_State* L)
	{
		auto registerCFunction = [L](const char* name, lua_CFunction function)
		{
			lua_pushcfunction(L, function);
			lua_setglobal(L, name);
		};

		auto registerConst = [L](const char* name, e_uint32 value)
		{
			lua_pushinteger(L, value);
			lua_setglobal(L, name);
		};

		registerCFunction("newView", &lua_newView);
		registerCFunction("bindFramebufferTexture", &lua_bindFramebufferTexture);
		registerCFunction("setViewMode", &lua_setViewMode);

#define REGISTER_FUNCTION(name) \
		do {\
			auto f = &lua_manager::wrapMethod<Pipeline, decltype(&Pipeline::name), &Pipeline::name>; \
			registerCFunction(#name, f); \
		} while(false) \

		REGISTER_FUNCTION(drawQuad);
		REGISTER_FUNCTION(getLayerMask);
		REGISTER_FUNCTION(drawQuadEx);
		REGISTER_FUNCTION(setPass);
		REGISTER_FUNCTION(bindRenderbuffer);
		REGISTER_FUNCTION(bindTexture);
		REGISTER_FUNCTION(bindEnvironmentMaps);
		REGISTER_FUNCTION(applyCamera);
		REGISTER_FUNCTION(getWidth);
		REGISTER_FUNCTION(getHeight);

		REGISTER_FUNCTION(disableBlending);
		REGISTER_FUNCTION(enableAlphaWrite);
		REGISTER_FUNCTION(disableAlphaWrite);
		REGISTER_FUNCTION(enableRGBWrite);
		REGISTER_FUNCTION(disableRGBWrite);
		REGISTER_FUNCTION(enableDepthWrite);
		REGISTER_FUNCTION(disableDepthWrite);
		REGISTER_FUNCTION(renderDebugShapes);
		REGISTER_FUNCTION(executeCustomCommand);
		REGISTER_FUNCTION(getFPS);
		REGISTER_FUNCTION(createUniform);
		REGISTER_FUNCTION(createVec4ArrayUniform);
		REGISTER_FUNCTION(hasScene);
		REGISTER_FUNCTION(cameraExists);
		REGISTER_FUNCTION(enableBlending);
		REGISTER_FUNCTION(clear);
		REGISTER_FUNCTION(renderPointLightLitGeometry);
		REGISTER_FUNCTION(renderShadowmap);
		REGISTER_FUNCTION(copyRenderbuffer);
		REGISTER_FUNCTION(setActiveGlobalLightUniforms);
		REGISTER_FUNCTION(setStencil);
		REGISTER_FUNCTION(setStencilRMask);
		REGISTER_FUNCTION(setStencilRef);
		REGISTER_FUNCTION(renderLightVolumes);
		REGISTER_FUNCTION(renderDecalsVolumes);
		REGISTER_FUNCTION(removeFramebuffer);
		REGISTER_FUNCTION(setMaterialDefine);
		REGISTER_FUNCTION(saveRenderbuffer);

#undef REGISTER_FUNCTION

#define REGISTER_FUNCTION(name) \
		registerCFunction(#name, lua_manager::wrap<decltype(&lua_##name), lua_##name>)

		REGISTER_FUNCTION(print);
		REGISTER_FUNCTION(logError);
		REGISTER_FUNCTION(getRenderbuffer);
		REGISTER_FUNCTION(renderLocalLightsShadowmaps);
		REGISTER_FUNCTION(setUniform);
		REGISTER_FUNCTION(addFramebuffer);
		REGISTER_FUNCTION(renderModels);

#undef REGISTER_FUNCTION

#define REGISTER_STENCIL_CONST(a) \
		registerConst("STENCIL_" #a, BGFX_STENCIL_##a)

		REGISTER_STENCIL_CONST(TEST_LESS);
		REGISTER_STENCIL_CONST(TEST_LEQUAL);
		REGISTER_STENCIL_CONST(TEST_EQUAL);
		REGISTER_STENCIL_CONST(TEST_GEQUAL);
		REGISTER_STENCIL_CONST(TEST_GREATER);
		REGISTER_STENCIL_CONST(TEST_NOTEQUAL);
		REGISTER_STENCIL_CONST(TEST_NEVER);
		REGISTER_STENCIL_CONST(TEST_ALWAYS);
		REGISTER_STENCIL_CONST(TEST_SHIFT);
		REGISTER_STENCIL_CONST(TEST_MASK);

		REGISTER_STENCIL_CONST(OP_FAIL_S_ZERO);
		REGISTER_STENCIL_CONST(OP_FAIL_S_KEEP);
		REGISTER_STENCIL_CONST(OP_FAIL_S_REPLACE);
		REGISTER_STENCIL_CONST(OP_FAIL_S_INCR);
		REGISTER_STENCIL_CONST(OP_FAIL_S_INCRSAT);
		REGISTER_STENCIL_CONST(OP_FAIL_S_DECR);
		REGISTER_STENCIL_CONST(OP_FAIL_S_DECRSAT);
		REGISTER_STENCIL_CONST(OP_FAIL_S_INVERT);
		REGISTER_STENCIL_CONST(OP_FAIL_S_SHIFT);
		REGISTER_STENCIL_CONST(OP_FAIL_S_MASK);

		REGISTER_STENCIL_CONST(OP_FAIL_Z_ZERO);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_KEEP);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_REPLACE);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_INCR);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_INCRSAT);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_DECR);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_DECRSAT);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_INVERT);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_SHIFT);
		REGISTER_STENCIL_CONST(OP_FAIL_Z_MASK);

		REGISTER_STENCIL_CONST(OP_PASS_Z_ZERO);
		REGISTER_STENCIL_CONST(OP_PASS_Z_KEEP);
		REGISTER_STENCIL_CONST(OP_PASS_Z_REPLACE);
		REGISTER_STENCIL_CONST(OP_PASS_Z_INCR);
		REGISTER_STENCIL_CONST(OP_PASS_Z_INCRSAT);
		REGISTER_STENCIL_CONST(OP_PASS_Z_DECR);
		REGISTER_STENCIL_CONST(OP_PASS_Z_DECRSAT);
		REGISTER_STENCIL_CONST(OP_PASS_Z_INVERT);
		REGISTER_STENCIL_CONST(OP_PASS_Z_SHIFT);
		REGISTER_STENCIL_CONST(OP_PASS_Z_MASK);

		registerConst("TEXTURE_MAG_ANISOTROPIC", BGFX_TEXTURE_MAG_ANISOTROPIC);
		registerConst("TEXTURE_MIN_ANISOTROPIC", BGFX_TEXTURE_MIN_ANISOTROPIC);
		registerConst("CLEAR_DEPTH", BGFX_CLEAR_DEPTH);
		registerConst("CLEAR_COLOR", BGFX_CLEAR_COLOR);
		registerConst("CLEAR_STENCIL", BGFX_CLEAR_STENCIL);
		registerConst("CLEAR_ALL", BGFX_CLEAR_STENCIL | BGFX_CLEAR_DEPTH | BGFX_CLEAR_COLOR);

		registerConst("VIEW_MODE_DEPTH_ASCENDING", bgfx::ViewMode::DepthAscending);
		registerConst("VIEW_MODE_DEPTH_DESCENDING", bgfx::ViewMode::DepthDescending);
		registerConst("VIEW_MODE_SEQUENTIAL", bgfx::ViewMode::Sequential);

#undef REGISTER_STENCIL_CONST
	}

	static void lua_on_file_loaded(lua_State* L, FS::IFile& file, Pipeline* pipline)
	{
		pipline->m_lua_state = lua_newthread(L);
		pipline->m_lua_thread_ref = luaL_ref(L, LUA_REGISTRYINDEX);

		lua_newtable(pipline->m_lua_state);
		lua_pushvalue(pipline->m_lua_state, -1);
		pipline->m_lua_env = luaL_ref(pipline->m_lua_state, LUA_REGISTRYINDEX);
		lua_pushvalue(pipline->m_lua_state, -1);
		lua_setmetatable(pipline->m_lua_state, -2);
		lua_pushglobaltable(pipline->m_lua_state);
		lua_setfield(pipline->m_lua_state, -2, "__index");

		lua_rawgeti(pipline->m_lua_state, LUA_REGISTRYINDEX, pipline->m_lua_env);
		lua_pushlightuserdata(pipline->m_lua_state, pipline);
		lua_setfield(pipline->m_lua_state, -2, "this");

		lua_registerLuaAPI(pipline->m_lua_state);
		for (auto& handler : pipline->m_custom_commands_handlers)
		{
			lua_exposeCustomCommandToLua(pipline, handler);
		}

		lua_setDefine(pipline);

		bool errors =
			luaL_loadbuffer(pipline->m_lua_state, (const char*)file.getBuffer(), file.size(), pipline->m_path.c_str()) != LUA_OK;
		if (errors)
		{
			log_error("Renderer %s:%s.", pipline->m_path.c_str(), lua_tostring(pipline->m_lua_state, -1));
			lua_pop(pipline->m_lua_state, 1);
			return;
		}

		lua_rawgeti(pipline->m_lua_state, LUA_REGISTRYINDEX, pipline->m_lua_env);
		lua_setupvalue(pipline->m_lua_state, -2, 1);
		errors = lua_pcall(pipline->m_lua_state, 0, 0, 0) != LUA_OK;
		if (errors)
		{
			log_error("Renderer %s:%s", pipline->m_path.c_str(), lua_tostring(pipline->m_lua_state, -1));
			lua_pop(pipline->m_lua_state, 1);
			return;
		}
	}


}

#endif
