#include "runtime/EngineFramework/pipeline.h"
#include "common/filesystem/ifile_device.h"
#include "runtime/EngineFramework/buffer.h"
#include "runtime/EngineFramework/component_manager.h"
#include "runtime/EngineFramework/engine_root.h"
#include "runtime/EngineFramework/scene_manager.h"
#include "runtime/EngineFramework/renderer.h"

#include "common/resource/entity_manager.h"
#include "common/resource/material_manager.h"
#include "common/resource/texture_manager.h"
#include "common/resource/shader_manager.h"

#include <lua.hpp>
#include <lauxlib.h>
#include "common/lua/lua_function.h"
#include "common/lua/lua_manager.h"
#include "common/lua/lua_pipeline.h"

#include "common/resource/resource_define.h"

#include "common/filesystem/ifile_device.h"

#include <cmath>

#include "bx/bx.h"
#include "bx/math.h"
#include "bx/timer.h"

namespace egal
{
	Pipeline::Pipeline(Renderer& renderer, const ArchivePath& path, const e_char* define, IAllocator& allocator)
		: m_allocator(allocator)
		, m_path(path)
		, m_framebuffers(allocator)
		, m_custom_commands_handlers(allocator)
		, m_uniforms(allocator)
		, m_renderer(renderer)
		, m_default_framebuffer(nullptr)
		, m_debug_line_material(nullptr)
		, m_draw2d_material(nullptr)
		, m_default_cubemap(nullptr)
		, m_debug_flags(BGFX_DEBUG_TEXT)
		, m_point_light_shadowmaps(allocator)
		, m_is_rendering_in_shadowmap(false)
		, m_is_ready(false)
		, m_debug_index_buffer(BGFX_INVALID_HANDLE)
		, m_scene(nullptr)
		, m_width(-1)
		, m_height(-1)
		, m_define(define, allocator)
	{
		for (auto& handle : m_debug_vertex_buffers)
		{
			handle = BGFX_INVALID_HANDLE;
		}
		m_deferred_point_light_vertex_decl.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();

		m_base_vertex_decl.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		m_has_shadowmap_define_idx = m_renderer.getShaderDefineIdx("HAS_SHADOWMAP");
		m_instanced_define_idx = m_renderer.getShaderDefineIdx("INSTANCED");

		createUniforms();

		MaterialManager& material_manager = renderer.getMaterialManager();

		m_debug_line_material = (Material*)material_manager.load(ArchivePath("pipelines/editor/debugline.mat"));
		m_draw2d_material = (Material*)material_manager.load(ArchivePath("pipelines/common/draw2d.mat"));
		m_default_cubemap = (Texture*)renderer.getTextureManager().load(ArchivePath("pipelines/pbr/default_probe.dds"));

		m_sky_material = (Material*)material_manager.load(ArchivePath("models/defaultmaterial.mat"));
		
		//m_sky_material = (Material*)material_manager.load(ArchivePath("models/sky/clouded01/sky.mat"));

		createParticleBuffers();
		createCubeBuffers();

		m_stats = {};
	}

	const Pipeline::Stats& Pipeline::getStats()
	{
		return m_stats;
	}

	ArchivePath& Pipeline::getPath()
	{
		return m_path;
	}

	e_void Pipeline::load()
	{
		auto& fs = *g_file_system;
		TDelegate<e_void(FS::IFile&, e_bool)> cb;
		cb.bind<Pipeline, &Pipeline::onFileLoaded>(this);
		fs.openAsync(fs.getDefaultDevice(), m_path.c_str(), FS::Mode::OPEN_AND_READ, cb);
	}

	e_void Pipeline::cleanup()
	{
		for (e_int32 i = 0; i < m_uniforms.size(); ++i)
		{
			bgfx::destroy(m_uniforms[i]);
		}
		m_uniforms.clear();

		for (e_int32 i = 0; i < m_framebuffers.size(); ++i)
		{
			_delete(m_allocator, m_framebuffers[i]);
			if (m_framebuffers[i] == m_default_framebuffer) m_default_framebuffer = nullptr;
		}
		_delete(m_allocator, m_default_framebuffer);
		m_framebuffers.clear();
		bgfx::frame();
		bgfx::frame();
	}

	e_void Pipeline::onFileLoaded(FS::IFile& file, e_bool success)
	{
		if (!success)
		{
			log_error("Renderer Failed to load %s.", m_path);
			return;
		}

		cleanup();

		m_width = m_height = -1;
		
		lua_on_file_loaded(m_renderer.getEngine().getState(), file, this);

		m_width = m_height = -1;
		if (m_scene)
			lua_callInitScene(this);

		m_is_ready = true;

		m_is_ready = true;
	}

		

		e_void Pipeline::createParticleBuffers()
		{
			BaseVertex vertices[] = {
				{ -1, -1, 1, 0xffffffff, 0, 0},
				{ -1,  1, 1, 0xffffffff, 0, 1},
				{  1,  1, 1, 0xffffffff, 1, 1},
				{  1, -1, 1, 0xffffffff, 1, 0},
			};

			const bgfx::Memory* vertex_mem = bgfx::copy(vertices, sizeof(vertices));
			m_particle_vertex_buffer = bgfx::createVertexBuffer(vertex_mem, m_base_vertex_decl);

			e_uint16 indices[] = { 0, 1, 2, 0, 2, 3 };
			const bgfx::Memory* index_mem = bgfx::copy(indices, sizeof(indices));
			m_particle_index_buffer = bgfx::createIndexBuffer(index_mem);
		}

		e_void Pipeline::createUniforms()
		{
			m_grass_max_dist_uniform					= bgfx::createUniform("u_grassMaxDist",					bgfx::UniformType::Vec4);
			m_texture_size_uniform						= bgfx::createUniform("u_textureSize",					bgfx::UniformType::Vec4);
			m_cam_params								= bgfx::createUniform("u_camParams",					bgfx::UniformType::Vec4);
			m_cam_proj_uniform							= bgfx::createUniform("u_camProj",						bgfx::UniformType::Mat4);
			m_cam_view_uniform							= bgfx::createUniform("u_camView",						bgfx::UniformType::Mat4);
			m_cam_inv_view_uniform						= bgfx::createUniform("u_camInvView",					bgfx::UniformType::Mat4);
			m_cam_inv_viewproj_uniform					= bgfx::createUniform("u_camInvViewProj",				bgfx::UniformType::Mat4);
			m_cam_inv_proj_uniform						= bgfx::createUniform("u_camInvProj",					bgfx::UniformType::Mat4);
			m_texture_uniform							= bgfx::createUniform("u_texture",						bgfx::UniformType::Int1);
			m_tex_shadowmap_uniform						= bgfx::createUniform("u_texShadowmap",					bgfx::UniformType::Int1);
			m_terrain_scale_uniform						= bgfx::createUniform("u_terrainScale",					bgfx::UniformType::Vec4);
			m_rel_camera_pos_uniform					= bgfx::createUniform("u_relCamPos",					bgfx::UniformType::Vec4);
			m_terrain_params_uniform					= bgfx::createUniform("u_terrainParams",				bgfx::UniformType::Vec4);
			m_fog_params_uniform						= bgfx::createUniform("u_fogParams",					bgfx::UniformType::Vec4);
			m_fog_color_density_uniform					= bgfx::createUniform("u_fogColorDensity",				bgfx::UniformType::Vec4);
			m_light_pos_radius_uniform					= bgfx::createUniform("u_lightPosRadius",				bgfx::UniformType::Vec4);
			m_light_color_attenuation_uniform			= bgfx::createUniform("u_lightRgbAttenuation",			bgfx::UniformType::Vec4);
			m_light_color_indirect_intensity_uniform	= bgfx::createUniform("u_lightRgbAndIndirectIntensity", bgfx::UniformType::Vec4);
			m_light_dir_fov_uniform						= bgfx::createUniform("u_lightDirFov",					bgfx::UniformType::Vec4);
			m_shadowmap_matrices_uniform				= bgfx::createUniform("u_shadowmapMatrices",			bgfx::UniformType::Mat4,	4);
			m_bone_matrices_uniform						= bgfx::createUniform("u_boneMatrices",					bgfx::UniformType::Mat4,	196);
			m_layer_uniform								= bgfx::createUniform("u_layer",						bgfx::UniformType::Vec4);
			m_terrain_matrix_uniform					= bgfx::createUniform("u_terrainMatrix",				bgfx::UniformType::Mat4);
			m_decal_matrix_uniform						= bgfx::createUniform("u_decalMatrix",					bgfx::UniformType::Mat4);
			m_emitter_matrix_uniform					= bgfx::createUniform("u_emitterMatrix",				bgfx::UniformType::Mat4);
		}

		e_void Pipeline::destroyUniforms()
		{
			bgfx::destroy(m_tex_shadowmap_uniform);
			bgfx::destroy(m_texture_uniform);
			bgfx::destroy(m_terrain_matrix_uniform);
			bgfx::destroy(m_bone_matrices_uniform);
			bgfx::destroy(m_layer_uniform);
			bgfx::destroy(m_terrain_scale_uniform);
			bgfx::destroy(m_rel_camera_pos_uniform);
			bgfx::destroy(m_terrain_params_uniform);
			bgfx::destroy(m_fog_params_uniform);
			bgfx::destroy(m_fog_color_density_uniform);
			bgfx::destroy(m_light_pos_radius_uniform);
			bgfx::destroy(m_light_color_attenuation_uniform);
			bgfx::destroy(m_light_color_indirect_intensity_uniform);
			bgfx::destroy(m_light_dir_fov_uniform);
			bgfx::destroy(m_shadowmap_matrices_uniform);
			bgfx::destroy(m_cam_inv_proj_uniform);
			bgfx::destroy(m_cam_inv_viewproj_uniform);
			bgfx::destroy(m_cam_view_uniform);
			bgfx::destroy(m_cam_proj_uniform);
			bgfx::destroy(m_cam_params);
			bgfx::destroy(m_grass_max_dist_uniform);
			bgfx::destroy(m_cam_inv_view_uniform);
			bgfx::destroy(m_texture_size_uniform);
			bgfx::destroy(m_decal_matrix_uniform);
			bgfx::destroy(m_emitter_matrix_uniform);
		}

		Pipeline::~Pipeline()
		{
			m_debug_line_material->getResourceManager().unload(*m_debug_line_material);
			m_draw2d_material->getResourceManager().unload(*m_draw2d_material);
			m_default_cubemap->getResourceManager().unload(*m_default_cubemap);
			m_sky_material->getResourceManager().unload(*m_sky_material);

			destroyUniforms();

			for (e_int32 i = 0; i < m_uniforms.size(); ++i)
			{
				bgfx::destroy(m_uniforms[i]);
			}

			for (e_int32 i = 0; i < m_framebuffers.size(); ++i)
			{
				_delete(m_allocator, m_framebuffers[i]);
				if (m_framebuffers[i] == m_default_framebuffer) m_default_framebuffer = nullptr;
			}
			_delete(m_allocator, m_default_framebuffer);

			bgfx::destroy(m_cube_vb);
			bgfx::destroy(m_cube_ib);
			bgfx::destroy(m_particle_index_buffer);
			bgfx::destroy(m_particle_vertex_buffer);
			if (bgfx::isValid(m_debug_index_buffer)) 
				bgfx::destroy(m_debug_index_buffer);
			for (auto& handle : m_debug_vertex_buffers)
			{
				if (bgfx::isValid(handle)) bgfx::destroy(handle);
			}
		}

		e_void Pipeline::bindTexture(e_int32 uniform_idx, e_int32 texture_idx)
		{
			if (!m_current_view) 
				return;

			auto* tex = (Texture*)m_renderer.getEngine().getLuaResource(texture_idx);
			m_current_view->command_buffer.beginAppend();
			m_current_view->command_buffer.setTexture(15 - m_global_textures_count, m_uniforms[uniform_idx], tex->handle);
			++m_global_textures_count;
			m_current_view->command_buffer.end();
		}

		e_void Pipeline::bindEnvironmentMaps(e_int32 irradiance_uniform_idx, e_int32 radiance_uniform_idx)
		{
			if (!m_current_view) 
				return;

			if (m_applied_camera == INVALID_COMPONENT) 
				return;

			GameObject cam			= m_scene->getCameraGameObject(m_applied_camera);
			float3 pos				= m_scene->getComponentManager().getPosition(cam);
			ComponentHandle probe	= m_scene->getNearestEnvironmentProbe(pos);

			m_current_view->command_buffer.beginAppend();
			if (probe.isValid())
			{
				Texture* irradiance = m_scene->getEnvironmentProbeIrradiance(probe);
				Texture* radiance	= m_scene->getEnvironmentProbeRadiance(probe);
				m_current_view->command_buffer.setTexture(15 - m_global_textures_count, m_uniforms[irradiance_uniform_idx], irradiance->handle, irradiance->bgfx_flags & ~(BGFX_TEXTURE_MAG_MASK | BGFX_TEXTURE_MIN_MASK));
				++m_global_textures_count;
				m_current_view->command_buffer.setTexture(15 - m_global_textures_count, m_uniforms[radiance_uniform_idx], radiance->handle, radiance->bgfx_flags & ~(BGFX_TEXTURE_MAG_MASK | BGFX_TEXTURE_MIN_MASK));
				++m_global_textures_count;
			}
			else
			{
				m_current_view->command_buffer.setTexture(15 - m_global_textures_count, m_uniforms[irradiance_uniform_idx], m_default_cubemap->handle);
				++m_global_textures_count;
				m_current_view->command_buffer.setTexture(15 - m_global_textures_count, m_uniforms[radiance_uniform_idx], m_default_cubemap->handle);
				++m_global_textures_count;
			}
			m_current_view->command_buffer.end();
		}

		e_void Pipeline::bindRenderbuffer(bgfx::TextureHandle* rb, e_int32 width, e_int32 height, e_int32 uniform_idx)
		{
			if (!rb) return;
			if (!m_current_view) return;

			float4 size;
			size.x = (e_float)width;
			size.y = (e_float)height;
			size.z = 1.0f / (e_float)width;
			size.w = 1.0f / (e_float)height;
			m_current_view->command_buffer.beginAppend();
			if (m_global_textures_count == 0) m_current_view->command_buffer.setUniform(m_texture_size_uniform, size);
			m_current_view->command_buffer.setTexture(15 - m_global_textures_count,
				m_uniforms[uniform_idx],
				*rb);
			++m_global_textures_count;
			m_current_view->command_buffer.end();
		}

		e_void Pipeline::setViewProjection(const float4x4& mtx, e_int32 width, e_int32 height)
		{
			if (!m_current_view) return;
			bgfx::setViewRect(m_current_view->bgfx_id, 0, 0, (uint16_t)width, (uint16_t)height);
			bgfx::setViewTransform(m_current_view->bgfx_id, nullptr, &mtx.m11);
		}

		e_void Pipeline::finishInstances(e_int32 idx)
		{
			InstanceData& data = m_instances_data[idx];
			if (!data.buffer)
				return;
			if (!data.buffer->data)
				return;

			Mesh& mesh = *data.mesh;
			Material* material = mesh.material;

			material->setDefine(m_instanced_define_idx, true);

			e_int32 view_idx = m_layer_to_view_map[material->getRenderLayer()];
			ASSERT(view_idx >= 0);
			auto& view = m_views[view_idx >= 0 ? view_idx : 0];

			executeCommandBuffer(material->getCommandBuffer(), material);
			executeCommandBuffer(view.command_buffer.buffer, material);

			bgfx::setVertexBuffer(0, mesh.vertex_buffer_handle);
			bgfx::setIndexBuffer(mesh.index_buffer_handle);
			bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);

			bgfx::setState(view.render_state | material->getRenderStates());
			bgfx::setInstanceDataBuffer(data.buffer, data.instance_count);
			ShaderInstance& shader_instance = material->getShaderInstance();
			++m_stats.draw_call_count;
			m_stats.instance_count += data.instance_count;
			m_stats.triangle_count += data.instance_count * mesh.indices_count / 3;
			bgfx::submit(0, shader_instance.getProgramHandle(view.pass_idx));

			data.buffer = nullptr;
			//data.buffer.data = nullptr;
			data.instance_count = 0;
			mesh.instance_idx = -1;
		}

		e_void Pipeline::applyCamera(const e_char* slot)
		{
			ComponentHandle cmp = m_scene->getCameraInSlot(slot);
			if (!cmp.isValid())
				return;

			m_scene->setCameraScreenSize(cmp, m_width, m_height);
			m_applied_camera = cmp;
			m_camera_frustum = m_scene->getCameraFrustum(cmp);

			float4x4 projection_matrix = m_scene->getCameraProjection(cmp);

			ComponentManager& com_man = m_scene->getComponentManager();
			float4x4 view = com_man.getMatrix(m_scene->getCameraGameObject(cmp));
			view.fastInverse();
			bgfx::setViewTransform(0, &view.m11, &projection_matrix.m11);
			bgfx::setViewRect(0, 0, 0, (e_uint16)m_width, (e_uint16)m_height);
		}

		e_void Pipeline::finishInstances()
		{
			for (e_int32 i = 0; i < TlengthOf(m_instances_data); ++i)
			{
				finishInstances(i);
			}
			m_instance_data_idx = 0;
		}

		e_void Pipeline::setPass(const e_char* name)
		{
			if (!m_current_view)
				return;
			m_pass_idx = m_renderer.getPassIdx(name);
			m_current_view->pass_idx = m_pass_idx;
		}

		ComponentHandle Pipeline::getAppliedCamera() const
		{
			return m_applied_camera;
		}

		Pipeline::CustomCommandHandler& Pipeline::addCustomCommandHandler(const e_char* name)
		{
			auto& handler = m_custom_commands_handlers.emplace();
			StringUnitl::copyString(handler.name, name);
			handler.hash = crc32(name);
			//exposeCustomCommandToLua(handler);
			return handler;
		}

		bgfx::TextureHandle& Pipeline::getRenderbuffer(const e_char* framebuffer_name, e_int32 renderbuffer_idx)
		{
			static bgfx::TextureHandle invalid = BGFX_INVALID_HANDLE;
			FrameBuffer* fb = getFramebuffer(framebuffer_name);
			if (!fb) return invalid;
			return fb->getRenderbufferHandle(renderbuffer_idx);
		}

		FrameBuffer* Pipeline::getFramebuffer(const e_char* framebuffer_name)
		{
			for (auto* framebuffer : m_framebuffers)
			{
				if (StringUnitl::equalStrings(framebuffer->getName(), framebuffer_name))
				{
					return framebuffer;
				}
			}
			return nullptr;
		}

		e_void Pipeline::setFramebuffer(const e_char* framebuffer_name)
		{
			if (!m_current_view) return;
			if (StringUnitl::equalStrings(framebuffer_name, "default"))
			{
				m_current_framebuffer = m_default_framebuffer;
				if (m_current_framebuffer)
				{
					bgfx::setViewFrameBuffer(m_current_view->bgfx_id, m_current_framebuffer->getHandle());
					e_uint16 w = m_current_framebuffer->getWidth();
					e_uint16 h = m_current_framebuffer->getHeight();
					bgfx::setViewRect(m_current_view->bgfx_id, 0, 0, w, h);
				}
				else
				{
					bgfx::setViewFrameBuffer(m_current_view->bgfx_id, BGFX_INVALID_HANDLE);
				}
				return;
			}
			m_current_framebuffer = getFramebuffer(framebuffer_name);
			if (m_current_framebuffer)
			{
				bgfx::setViewFrameBuffer(m_current_view->bgfx_id, m_current_framebuffer->getHandle());
				e_uint16 w = m_current_framebuffer->getWidth();
				e_uint16 h = m_current_framebuffer->getHeight();
				bgfx::setViewRect(m_current_view->bgfx_id, 0, 0, w, h);
			}
			else
			{
				log_error("Renderer Framebuffer %s not found.", framebuffer_name);
			}
		}

		e_int32 Pipeline::getWidth() { return m_width; }

		e_int32 Pipeline::getHeight() { return m_height; }

		e_float Pipeline::getFPS() { return m_renderer.getEngine().getFPS(); }

		e_void Pipeline::executeCustomCommand(const e_char* name)
		{
			e_uint32 name_hash = crc32(name);
			for(CustomCommandHandler& handler : m_custom_commands_handlers)
			{
				if(handler.hash == name_hash)
				{
					handler.callback.invoke();
					break;
				}
			}
			finishInstances();
		}

		e_int32 Pipeline::newView(const e_char* debug_name, e_uint64 layer_mask)
		{
			++m_view_idx;
			if (m_view_idx >= TlengthOf(m_views))
			{
				log_error("Renderer Too many views");
				--m_view_idx;
			}
			m_current_view = &m_views[m_view_idx];
			m_renderer.viewCounterAdd();
			m_current_view->layer_mask = layer_mask;
			m_current_view->bgfx_id = (e_uint8)m_renderer.getViewCounter();
			m_current_view->stencil = BGFX_STENCIL_NONE;
			m_current_view->render_state = BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE | BGFX_STATE_DEPTH_WRITE;
			m_current_view->pass_idx = m_pass_idx;
			m_current_view->command_buffer.clear();
			m_global_textures_count = 0;
			if (layer_mask != 0)
			{
				for (e_uint64 layer = 0; layer < 64; ++layer)
				{
					if (layer_mask & (1ULL << layer)) m_layer_to_view_map[layer] = m_view_idx;
				}
			}
			if (m_current_framebuffer)
			{
				bgfx::setViewFrameBuffer(m_current_view->bgfx_id, m_current_framebuffer->getHandle());
			}
			else
			{
				bgfx::setViewFrameBuffer(m_current_view->bgfx_id, BGFX_INVALID_HANDLE);
			}
			bgfx::setViewClear(m_current_view->bgfx_id, 0);
			bgfx::setViewName(m_current_view->bgfx_id, debug_name);
			return m_view_idx;
		}

		e_void Pipeline::saveRenderbuffer(const e_char* framebuffer, e_int32 render_buffer_index, const e_char* out_path)
		{
			FrameBuffer* fb = getFramebuffer(framebuffer);
			if (!fb)
			{
				log_error("Renderer saveRenderbuffer: Framebuffer %s not found", framebuffer);
				return;
			}

			bgfx::TextureHandle texture = bgfx::createTexture2D(
				fb->getWidth(), fb->getHeight(), false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_READ_BACK);
			m_renderer.viewCounterAdd();
			bgfx::touch(m_renderer.getViewCounter());
			bgfx::setViewName(m_renderer.getViewCounter(), "saveRenderbuffer_blit");
			bgfx::TextureHandle rb = fb->getRenderbufferHandle(render_buffer_index);
			bgfx::blit(m_renderer.getViewCounter(), texture, 0, 0, rb);
		
			m_renderer.viewCounterAdd();
			bgfx::setViewName(m_renderer.getViewCounter(), "saveRenderbuffer_read");
			TArrary<e_uint8> data(m_renderer.getEngine().getAllocator());
			data.resize(fb->getWidth() * fb->getHeight() * 4);
			bgfx::readTexture(texture, &data[0]);
			bgfx::touch(m_renderer.getViewCounter());

			bgfx::frame(); // submit
			bgfx::frame(); // wait for gpu

			FS::FileSystem& fs = *g_file_system;
			FS::IFile* file = fs.open(fs.getDefaultDevice(), out_path, FS::Mode::CREATE_AND_WRITE);
			Texture::saveTGA(file, fb->getWidth(), fb->getHeight(), 4, &data[0], ArchivePath(out_path), m_renderer.getEngine().getAllocator());

			fs.close(*file);

			bgfx::destroy(texture);
		}

		e_void Pipeline::copyRenderbuffer(const e_char* src_fb_name, e_int32 src_rb_idx, const e_char* dest_fb_name, e_int32 dest_rb_idx)
		{
			FrameBuffer* src_fb = getFramebuffer(src_fb_name);
			if (!src_fb) return;
			FrameBuffer* dest_fb = getFramebuffer(dest_fb_name);
			if (!dest_fb) return;

			if (bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_BLIT)
			{
				auto src_rb = src_fb->getRenderbufferHandle(src_rb_idx);
				auto dest_rb = dest_fb->getRenderbufferHandle(dest_rb_idx);

				bgfx::blit(m_current_view->bgfx_id, dest_rb, 0, 0, src_rb);
				bgfx::touch(m_current_view->bgfx_id);
				return;
			}

			log_error("Renderer Texture blit is not supported.");
		}

		e_void Pipeline::createCubeBuffers()
		{
			const float3 cube_vertices[] = {
				{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
				{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},
				{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1},
				{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1},
				{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1},
				{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}
			};
			static const e_uint16 cube_indices[] = {
				0, 2, 1, 2, 0, 3,
				4, 5, 6, 6, 7, 4,
				8, 10, 9, 10, 8, 11,
				12, 13, 14, 14, 15, 12,
				16, 18, 17, 18, 16, 19,
				20, 21, 22, 22, 23, 20
			};
			auto* vertices_mem = bgfx::copy(cube_vertices, sizeof(cube_vertices));
			auto* indices_mem = bgfx::copy(cube_indices, sizeof(cube_indices));
			m_cube_vb = bgfx::createVertexBuffer(vertices_mem, m_deferred_point_light_vertex_decl);
			m_cube_ib = bgfx::createIndexBuffer(indices_mem);
		}

		e_void Pipeline::finishDeferredPointLightInstances(Material* material,
			const bgfx::InstanceDataBuffer* instance_buffer,
			e_int32 instance_count,
			e_bool is_intersecting,
			PointLightShadowmap* shadowmap)
		{
			View& view = *m_current_view;
			bgfx::setInstanceDataBuffer(instance_buffer, instance_count);
			bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE); 
			auto state = view.render_state | material->getRenderStates();
			if (is_intersecting)
			{
				state = ((state & ~BGFX_STATE_CULL_MASK) & ~BGFX_STATE_DEPTH_TEST_MASK) | BGFX_STATE_CULL_CCW;
			}
			bgfx::setState(state);
			executeCommandBuffer(view.command_buffer.buffer, material);
			material->setDefine(m_has_shadowmap_define_idx, shadowmap != nullptr);
			if (shadowmap)
			{
				e_uint32 flags = BGFX_TEXTURE_MIN_ANISOTROPIC | BGFX_TEXTURE_MAG_ANISOTROPIC;
				bgfx::setTexture(15 - m_global_textures_count, m_tex_shadowmap_uniform, shadowmap->framebuffer->getRenderbufferHandle(0), flags);
				bgfx::setUniform(m_shadowmap_matrices_uniform,
					&shadowmap->matrices[0],
					m_scene->getLightFOV(shadowmap->light) > Math::C_Pi ? 4 : 1);
			}
			bgfx::setVertexBuffer(0, m_cube_vb);
			bgfx::setIndexBuffer(m_cube_ib);
			++m_stats.draw_call_count;
			m_stats.instance_count += instance_count;
			m_stats.triangle_count += instance_count * 12;
			bgfx::submit(m_current_view->bgfx_id, material->getShaderInstance().getProgramHandle(m_pass_idx));
		}

		e_void Pipeline::removeFramebuffer(const e_char* framebuffer_name)
		{
			for (e_int32 i = 0; i < m_framebuffers.size(); ++i)
			{
				if (StringUnitl::equalStrings(m_framebuffers[i]->getName(), framebuffer_name))
				{
					_delete(m_allocator, m_framebuffers[i]);
					m_framebuffers.erase(i);
					break;
				}
			}
		}

		e_void Pipeline::setMaterialDefine(e_int32 material_idx, const e_char* define, e_bool enabled)
		{
			auto define_idx = m_renderer.getShaderDefineIdx(define);
			Resource* res = m_scene->getEngine().getLuaResource(material_idx);
			Material* material = static_cast<Material*>(res);
			material->setDefine(define_idx, enabled);
		}

		e_void Pipeline::renderLightVolumes(e_int32 material_index)
		{
			//PROFILE_FUNCTION();
			if (m_applied_camera == INVALID_COMPONENT) 
				return;
			Resource* res = m_scene->getEngine().getLuaResource(material_index);
			Material* material = static_cast<Material*>(res);
			if (!material->isReady()) 
				return;

			IAllocator& frame_allocator = m_renderer.getEngine().getLIFOAllocator();
			TArrary<ComponentHandle> local_lights(frame_allocator);
			m_scene->getPointLights(m_camera_frustum, local_lights);

			//PROFILE_INT("light count", local_lights.size());
			struct Data
			{
				float4x4 mtx;
				float4 pos_radius;
				float4 color_attenuation;
				float4 dir_fov;
				float4 specular;
			};
			bgfx::InstanceDataBuffer instance_buffer[2];
			instance_buffer[0].data = nullptr;
			instance_buffer[1].data = nullptr;
			Data* instance_data[2] = { nullptr, nullptr };
			ComponentManager& com_man = m_scene->getComponentManager();
			for(auto light_cmp : local_lights)
			{
				auto entity = m_scene->getPointLightGameObject(light_cmp);
				e_float range = m_scene->getLightRange(light_cmp);
				float3 light_dir = com_man.getRotation(entity).rotate(float3(0, 0, -1));
				e_float attenuation = m_scene->getLightAttenuation(light_cmp);
				e_float fov = m_scene->getLightFOV(light_cmp);
				e_float intensity = m_scene->getPointLightIntensity(light_cmp);
				intensity *= intensity;
				float3 color = m_scene->getPointLightColor(light_cmp) * intensity;

				e_int32 max_instance_count = 128;
				PointLightShadowmap* shadowmap = nullptr;
				if (m_scene->getLightCastShadows(light_cmp))
				{
					for (auto& i : m_point_light_shadowmaps)
					{
						if (i.light == light_cmp)
						{
							max_instance_count = 1;
							shadowmap = &i;
						}
					}
				}

				float3 pos = com_man.getPosition(entity);
				e_bool is_intersecting = m_camera_frustum.intersectNearPlane(pos, range * Math::C_Sqrt3);
				e_int32 buffer_idx = is_intersecting ? 0 : 1;
				if(!instance_buffer[buffer_idx].data)
				{
					bgfx::allocInstanceDataBuffer(128, sizeof(Data)); //mabe error todo
					instance_data[buffer_idx] = (Data*)instance_buffer[buffer_idx].data;
				}

				auto* id = instance_data[buffer_idx];
				id->mtx = com_man.getPositionAndRotation(entity);
				id->mtx.multiply3x3(range);
				id->pos_radius.set(pos, range);
				id->color_attenuation.set(color, attenuation);
				id->dir_fov.set(light_dir, fov);
				e_float specular_intensity = m_scene->getPointLightSpecularIntensity(light_cmp);
				id->specular.set(m_scene->getPointLightSpecularColor(light_cmp) 
					* specular_intensity * specular_intensity, 1);
				++instance_data[buffer_idx];

				e_int32 instance_count = e_int32(instance_data[buffer_idx] - (Data*)instance_buffer[buffer_idx].data);
				if(instance_count == max_instance_count)
				{
					finishDeferredPointLightInstances(material,
						&instance_buffer[buffer_idx],
						instance_count,
						is_intersecting,
						shadowmap);
					instance_buffer[buffer_idx].data = nullptr;
					instance_data[buffer_idx] = nullptr;
				}
			}

			for(e_int32 buffer_idx = 0; buffer_idx < 2; ++buffer_idx)
			{
				if(instance_data[buffer_idx])
				{
					finishDeferredPointLightInstances(material,
						&instance_buffer[buffer_idx],
						e_int32(instance_data[buffer_idx] - (Data*)instance_buffer[buffer_idx].data),
						buffer_idx == 0,
						nullptr);
				}
			}
		}

		e_void Pipeline::renderDecalsVolumes()
		{
			//PROFILE_FUNCTION();
			if (m_applied_camera == INVALID_COMPONENT) return;
			if (!m_current_view) return;

			IAllocator& frame_allocator = m_renderer.getEngine().getLIFOAllocator();
			TArrary<DecalInfo> decals(frame_allocator);
			m_scene->getDecals(m_camera_frustum, decals);

			//PROFILE_INT("decal count", decals.size());

			const View& view = *m_current_view;
			for (const DecalInfo& decal : decals)
			{
				auto state = view.render_state | decal.material->getRenderStates();
				if (m_camera_frustum.intersectNearPlane(decal.position, decal.radius))
				{
					state = ((state & ~BGFX_STATE_CULL_MASK) & ~BGFX_STATE_DEPTH_TEST_MASK) | BGFX_STATE_CULL_CCW;
				}
				bgfx::setState(state);
				executeCommandBuffer(decal.material->getCommandBuffer(), decal.material);
				executeCommandBuffer(view.command_buffer.buffer, decal.material);
				bgfx::setUniform(m_decal_matrix_uniform, &decal.inv_mtx.m11);
				bgfx::setTransform(&decal.mtx.m11);
				bgfx::setVertexBuffer(0, m_cube_vb);
				bgfx::setIndexBuffer(m_cube_ib);
				bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
			
				bgfx::submit(m_current_view->bgfx_id, decal.material->getShaderInstance().getProgramHandle(m_pass_idx));
			}
		}

		e_void Pipeline::renderSpotLightShadowmap(ComponentHandle light)
		{
			newView("point_light", ~0ULL);

			GameObject light_entity = m_scene->getPointLightGameObject(light);
			float4x4 mtx = m_scene->getComponentManager().getMatrix(light_entity);
			e_float fov = m_scene->getLightFOV(light);
			e_float range = m_scene->getLightRange(light);
			e_uint16 shadowmap_height = (e_uint16)m_current_framebuffer->getHeight();
			e_uint16 shadowmap_width = (e_uint16)m_current_framebuffer->getWidth();
			float3 pos = mtx.getTranslation();

			bgfx::setViewClear(m_current_view->bgfx_id, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
			bgfx::touch(m_current_view->bgfx_id);
			bgfx::setViewRect(m_current_view->bgfx_id, 0, 0, shadowmap_width, shadowmap_height);

			float4x4 projection_matrix;
			projection_matrix.setPerspective(fov, 1, 0.01f, range, bgfx::getCaps()->homogeneousDepth);
			float4x4 view_matrix;
			view_matrix.lookAt(pos, pos - mtx.getZVector(), mtx.getYVector());
			bgfx::setViewTransform(m_current_view->bgfx_id, &view_matrix.m11, &projection_matrix.m11);

			PointLightShadowmap& s = m_point_light_shadowmaps.emplace();
			s.framebuffer = m_current_framebuffer;
			s.light = light;
			e_float ymul = bgfx::getCaps()->originBottomLeft ? 0.5f : -0.5f;
			static const float4x4 biasMatrix(
				0.5,  0.0, 0.0, 0.0,
				0.0, ymul, 0.0, 0.0,
				0.0,  0.0, 0.5, 0.0,
				0.5,  0.5, 0.5, 1.0);
			s.matrices[0] = biasMatrix * (projection_matrix * view_matrix);

			renderPointLightInfluencedGeometry(light);
		}

		e_void Pipeline::renderOmniLightShadowmap(ComponentHandle light)
		{
			GameObject light_entity = m_scene->getPointLightGameObject(light);
			float3 light_pos = m_scene->getComponentManager().getPosition(light_entity);
			e_float range = m_scene->getLightRange(light);
			e_uint16 shadowmap_height = (e_uint16)m_current_framebuffer->getHeight();
			e_uint16 shadowmap_width = (e_uint16)m_current_framebuffer->getWidth();

			e_float viewports[] = {0, 0, 0.5, 0, 0, 0.5, 0.5, 0.5};

			static const e_float YPR_gl[4][3] = {
				{Math::degreesToRadians(-90.0f), Math::degreesToRadians(-27.36780516f), Math::degreesToRadians(0.0f)},
				{Math::degreesToRadians(90.0f), Math::degreesToRadians(-27.36780516f), Math::degreesToRadians(0.0f)},
				{Math::degreesToRadians(0.0f), Math::degreesToRadians(27.36780516f), Math::degreesToRadians(0.0f)},
				{Math::degreesToRadians(180.0f), Math::degreesToRadians(27.36780516f), Math::degreesToRadians(0.0f)},
			};

			static const e_float YPR[4][3] = {
				{Math::degreesToRadians(0.0f), Math::degreesToRadians(27.36780516f), Math::degreesToRadians(0.0f)},
				{Math::degreesToRadians(180.0f), Math::degreesToRadians(27.36780516f), Math::degreesToRadians(0.0f)},
				{Math::degreesToRadians(-90.0f), Math::degreesToRadians(-27.36780516f), Math::degreesToRadians(0.0f)},
				{Math::degreesToRadians(90.0f), Math::degreesToRadians(-27.36780516f), Math::degreesToRadians(0.0f)},
			};

			PointLightShadowmap& shadowmap_info = m_point_light_shadowmaps.emplace();
			shadowmap_info.framebuffer = m_current_framebuffer;
			shadowmap_info.light = light;
			//setPointLightUniforms(light);

			IAllocator& frame_allocator = m_renderer.getEngine().getLIFOAllocator();
			for (e_int32 i = 0; i < 4; ++i)
			{
				newView("omnilight", 0xff);

				bgfx::setViewClear(m_current_view->bgfx_id, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
				bgfx::touch(m_current_view->bgfx_id);
				e_uint16 view_x = e_uint16(shadowmap_width * viewports[i * 2]);
				e_uint16 view_y = e_uint16(shadowmap_height * viewports[i * 2 + 1]);
				bgfx::setViewRect(
					m_current_view->bgfx_id, view_x, view_y, shadowmap_width >> 1, shadowmap_height >> 1);

				e_float fovx = Math::degreesToRadians(143.98570868f + 3.51f);
				e_float fovy = Math::degreesToRadians(125.26438968f + 9.85f);
				e_float aspect = tanf(fovx * 0.5f) / tanf(fovy * 0.5f);

				float4x4 projection_matrix;
				projection_matrix.setPerspective(fovx, aspect, 0.01f, range, bgfx::getCaps()->homogeneousDepth);

				float4x4 view_matrix;
				if (bgfx::getCaps()->originBottomLeft)
				{
					view_matrix.fromEuler(YPR_gl[i][0], YPR_gl[i][1], YPR_gl[i][2]);
				}
				else
				{
					view_matrix.fromEuler(YPR[i][0], YPR[i][1], YPR[i][2]);
				}
				view_matrix.setTranslation(light_pos);
				Frustum frustum;
				frustum.computePerspective(light_pos,
					-view_matrix.getZVector(),
					view_matrix.getYVector(),
					fovx,
					aspect,
					0.01f,
					range);

				view_matrix.fastInverse();

				bgfx::setViewTransform(m_current_view->bgfx_id, &view_matrix.m11, &projection_matrix.m11);

				e_float ymul = bgfx::getCaps()->originBottomLeft ? 0.5f : -0.5f;
				static const float4x4 biasMatrix(
					0.5, 0.0, 0.0, 0.0, 0.0, ymul, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
				shadowmap_info.matrices[i] = biasMatrix * (projection_matrix * view_matrix);

				TArrary<EntityInstanceMesh> tmp_meshes(frame_allocator);
				m_is_current_light_global = false;
				m_scene->getPointLightInfluencedGeometry(light, frustum, tmp_meshes);

				renderMeshes(tmp_meshes);
			}
		}

		e_void Pipeline::renderLocalLightShadowmaps(ComponentHandle camera, FrameBuffer** fbs, e_int32 framebuffers_count)
		{
			if (!camera.isValid()) return;

			ComponentManager& com_man = m_scene->getComponentManager();
			GameObject camera_entity = m_scene->getCameraGameObject(camera);
			float3 camera_pos = com_man.getPosition(camera_entity);

			ComponentHandle lights[16];
			e_int32 light_count = m_scene->getClosestPointLights(camera_pos, lights, TlengthOf(lights));

			e_int32 fb_index = 0;
			for (e_int32 i = 0; i < light_count; ++i)
			{
				if (!m_scene->getLightCastShadows(lights[i])) continue;
				if (fb_index == framebuffers_count) break;

				e_float fov = m_scene->getLightFOV(lights[i]);

				m_current_framebuffer = fbs[i];
				if (fov < Math::C_Pi)
				{
					renderSpotLightShadowmap(lights[i]);
				}
				else
				{
					renderOmniLightShadowmap(lights[i]);
				}
				++fb_index;
			}
		}

		static float3 shadowmapTexelAlign(const float3& shadow_cam_pos,
			e_float shadowmap_width,
			e_float frustum_radius,
			const float4x4& light_mtx)
		{
			float4x4 inv = light_mtx;
			inv.fastInverse();
			float3 out = inv.transformPoint(shadow_cam_pos);
			e_float align = 2 * frustum_radius / (shadowmap_width * 0.5f - 2);
			out.x -= fmodf(out.x, align);
			out.y -= fmodf(out.y, align);
			out = light_mtx.transformPoint(out);
			return out;
		}

		e_void Pipeline::findExtraShadowcasterPlanes(const float3& light_forward, const Frustum& camera_frustum, const float3& camera_position, Frustum* shadow_camera_frustum)
		{
			static const Frustum::Planes planes[] = {
				Frustum::Planes::EP_LEFT, Frustum::Planes::EP_TOP, Frustum::Planes::EP_RIGHT, Frustum::Planes::EP_BOTTOM };
			e_bool prev_side = dotProduct(light_forward, camera_frustum.getNormal(planes[TlengthOf(planes) - 1])) < 0;
			e_int32 out_plane = (e_int32)Frustum::Planes::EP_EXTRA0;
			float3 camera_frustum_center = camera_frustum.computeBoundingSphere().position;
			for (e_int32 i = 0; i < TlengthOf(planes); ++i)
			{
				e_bool side = dotProduct(light_forward, camera_frustum.getNormal(planes[i])) < 0;
				if (prev_side != side)
				{
					float3 n0 = camera_frustum.getNormal(planes[i]);
					float3 n1 = camera_frustum.getNormal(planes[(i + TlengthOf(planes) - 1) % TlengthOf(planes)]);
					float3 line_dir = crossProduct(n1, n0);
					float3 n = crossProduct(light_forward, line_dir);
					e_float d = -dotProduct(camera_position, n);
					if (dotProduct(camera_frustum_center, n) + d < 0)
					{
						n = -n;
						d = -dotProduct(camera_position, n);
					}
					shadow_camera_frustum->setPlane((Frustum::Planes)out_plane, n, d);
					++out_plane;
					if (out_plane >(e_int32)Frustum::Planes::EP_EXTRA1)
						break;
				}
				prev_side = side;
			}
		}

		e_void Pipeline::renderShadowmap(e_int32 split_index)
		{
			if (!m_current_view) return;
			ComponentManager& com_man = m_scene->getComponentManager();
			ComponentHandle light_cmp = m_scene->getActiveGlobalLight();
			if (!light_cmp.isValid() || !m_applied_camera.isValid()) 
				return;
			e_float camera_height = m_scene->getCameraScreenHeight(m_applied_camera);
			if (!camera_height)
				return;

			float4x4 light_mtx = com_man.getMatrix(m_scene->getGlobalLightGameObject(light_cmp));
			m_global_light_shadowmap = m_current_framebuffer;
			e_float shadowmap_height = (e_float)m_current_framebuffer->getHeight();
			e_float shadowmap_width = (e_float)m_current_framebuffer->getWidth();
			e_float viewports[] = { 0, 0, 0.5f, 0, 0, 0.5f, 0.5f, 0.5f };
			e_float viewports_gl[] = { 0, 0.5f, 0.5f, 0.5f, 0, 0, 0.5f, 0};
			e_float camera_fov = m_scene->getCameraFOV(m_applied_camera);
			e_float camera_ratio = m_scene->getCameraScreenWidth(m_applied_camera) / camera_height;
			float4 cascades = m_scene->getShadowmapCascades(light_cmp);
			e_float split_distances[] = {0.1f, cascades.x, cascades.y, cascades.z, cascades.w};
			m_is_rendering_in_shadowmap = true;
			bgfx::setViewClear(m_current_view->bgfx_id, BGFX_CLEAR_DEPTH | BGFX_CLEAR_COLOR, 0xffffffff, 1.0f, 0);
			bgfx::touch(m_current_view->bgfx_id);
			e_float* viewport = (bgfx::getCaps()->originBottomLeft ? viewports_gl : viewports) + split_index * 2;
			bgfx::setViewRect(m_current_view->bgfx_id,
				(e_uint16)(1 + shadowmap_width * viewport[0]),
				(e_uint16)(1 + shadowmap_height * viewport[1]),
				(e_uint16)(0.5f * shadowmap_width - 2),
				(e_uint16)(0.5f * shadowmap_height - 2));

			Frustum camera_frustum;
			float4x4 camera_matrix = com_man.getMatrix(m_scene->getCameraGameObject(m_applied_camera));
			camera_frustum.computePerspective(camera_matrix.getTranslation(),
				-camera_matrix.getZVector(),
				camera_matrix.getYVector(),
				camera_fov,
				camera_ratio,
				split_distances[split_index],
				split_distances[split_index + 1]);

			Sphere frustum_bounding_sphere = camera_frustum.computeBoundingSphere();
			float3 shadow_cam_pos = frustum_bounding_sphere.position;
			e_float bb_size = frustum_bounding_sphere.radius;
			shadow_cam_pos = shadowmapTexelAlign(shadow_cam_pos, 0.5f * shadowmap_width - 2, bb_size, light_mtx);

			float4x4 projection_matrix;
			projection_matrix.setOrtho(-bb_size, bb_size, -bb_size, bb_size, SHADOW_CAM_NEAR, SHADOW_CAM_FAR, bgfx::getCaps()->homogeneousDepth);
			float3 light_forward = light_mtx.getZVector();
			shadow_cam_pos -= light_forward * SHADOW_CAM_FAR * 0.5f;
			float4x4 view_matrix;
			view_matrix.lookAt(shadow_cam_pos, shadow_cam_pos + light_forward, light_mtx.getYVector());
			bgfx::setViewTransform(m_current_view->bgfx_id, &view_matrix.m11, &projection_matrix.m11);
			e_float ymul = bgfx::getCaps()->originBottomLeft ? 0.5f : -0.5f;
			static const float4x4 biasMatrix(0.5, 0.0, 0.0, 0.0, 0.0, ymul, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
			m_shadow_viewprojection[split_index] = biasMatrix * (projection_matrix * view_matrix);

			Frustum shadow_camera_frustum;
			shadow_camera_frustum.computeOrtho(
				shadow_cam_pos, -light_forward, light_mtx.getYVector(), bb_size, bb_size, SHADOW_CAM_NEAR, SHADOW_CAM_FAR);

			findExtraShadowcasterPlanes(light_forward, camera_frustum, camera_matrix.getTranslation(), &shadow_camera_frustum);

			renderAll(shadow_camera_frustum, false, m_applied_camera, m_current_view->layer_mask);

			m_is_rendering_in_shadowmap = false;
		}

		e_void Pipeline::renderDebugShapes()
		{
			if (!bgfx::isValid(m_debug_index_buffer))
			{
				auto* mem = bgfx::alloc(0xffFF * 2);
				e_uint16* data = (e_uint16*)mem->data;
				for (e_uint16 i = 0; i < 0xffff; ++i) data[i] = i;
				m_debug_index_buffer = bgfx::createDynamicIndexBuffer(mem);
			}

			m_debug_buffer_idx = 0;
			renderDebugTriangles();
			renderDebugLines();
			renderDebugPoints();
		}

		e_void Pipeline::renderDebugPoints()
		{
			if (!m_current_view) return;

			const TArrary<DebugPoint>& points = m_scene->getDebugPoints();
			if (points.empty() || !m_debug_line_material->isReady()) return;

			static const e_int32 BATCH_SIZE = 0xffff;
			View& view = *m_current_view;

			for (e_int32 j = 0; j < points.size() && m_debug_buffer_idx < TlengthOf(m_debug_vertex_buffers);
				j += BATCH_SIZE, ++m_debug_buffer_idx)
			{
				if (!bgfx::isValid(m_debug_vertex_buffers[m_debug_buffer_idx]))
				{
					m_debug_vertex_buffers[m_debug_buffer_idx] = bgfx::createDynamicVertexBuffer(0xffFF, m_base_vertex_decl);
				}

				e_int32 point_count = Math::minimum(BATCH_SIZE, points.size() - j);
				auto* mem = bgfx::alloc(sizeof(BaseVertex) * point_count);

				BaseVertex* vertex = (BaseVertex*)mem->data;
				for (e_int32 i = 0; i < points.size(); ++i)
				{
					const DebugPoint& point = points[i];
					vertex[0].rgba = point.color;
					vertex[0].x = point.pos.x;
					vertex[0].y = point.pos.y;
					vertex[0].z = point.pos.z;
					vertex[0].u = vertex[0].v = 0;

					++vertex;
				}

				bgfx::updateDynamicVertexBuffer(m_debug_vertex_buffers[m_debug_buffer_idx], 0, mem);

				bgfx::setVertexBuffer(0, m_debug_vertex_buffers[m_debug_buffer_idx]);
				bgfx::setIndexBuffer(m_debug_index_buffer, 0, point_count);
				bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
				bgfx::setState(view.render_state | m_debug_line_material->getRenderStates() | BGFX_STATE_PT_POINTS);
				bgfx::submit(
					m_current_view->bgfx_id, m_debug_line_material->getShaderInstance().getProgramHandle(m_pass_idx));
			}
		}

		e_void Pipeline::renderDebugLines()
		{
			if (!m_current_view) 
				return;

			const TArrary<DebugLine>& lines = m_scene->getDebugLines();
			if (lines.empty() || !m_debug_line_material->isReady()) 
				return;

			static const e_int32 BATCH_SIZE = 0xffff / 2;
			View& view = *m_current_view;

			for (e_int32 j = 0; j < lines.size() && m_debug_buffer_idx < TlengthOf(m_debug_vertex_buffers);
				 j += BATCH_SIZE, ++m_debug_buffer_idx)
			{
				if (!bgfx::isValid(m_debug_vertex_buffers[m_debug_buffer_idx]))
				{
					m_debug_vertex_buffers[m_debug_buffer_idx] = bgfx::createDynamicVertexBuffer(0xffFF, m_base_vertex_decl);
				}

				e_int32 line_count = Math::minimum(BATCH_SIZE, lines.size() - j);
				auto* mem = bgfx::alloc(sizeof(BaseVertex) * 2 * line_count);
			
				BaseVertex* vertex = (BaseVertex*)mem->data;
				for (e_int32 i = 0; i < line_count; ++i)
				{
					const DebugLine& line = lines[j + i];
					vertex[0].rgba = line.color;
					vertex[0].x = line.from.x;
					vertex[0].y = line.from.y;
					vertex[0].z = line.from.z;
					vertex[0].u = vertex[0].v = 0;

					vertex[1].rgba = line.color;
					vertex[1].x = line.to.x;
					vertex[1].y = line.to.y;
					vertex[1].z = line.to.z;
					vertex[1].u = vertex[0].v = 0;

					vertex += 2;
				}

				bgfx::updateDynamicVertexBuffer(m_debug_vertex_buffers[m_debug_buffer_idx], 0, mem);

				bgfx::setVertexBuffer(0, m_debug_vertex_buffers[m_debug_buffer_idx]);
				bgfx::setIndexBuffer(m_debug_index_buffer, 0, line_count * 2);
				bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
				bgfx::setState(view.render_state | m_debug_line_material->getRenderStates() | BGFX_STATE_PT_LINES);
				bgfx::submit(
					m_current_view->bgfx_id, m_debug_line_material->getShaderInstance().getProgramHandle(m_pass_idx));
			}
		}


		e_void Pipeline::renderDebugTriangles()
		{
			if (!m_current_view) return;

			const auto& tris = m_scene->getDebugTriangles();
			if(tris.empty() || !m_debug_line_material->isReady()) return;

			static const e_int32 BATCH_SIZE = 0xffFF / 3;
			View& view = *m_current_view;

			for (e_int32 j = 0; j < tris.size() && m_debug_buffer_idx < TlengthOf(m_debug_vertex_buffers);
				 j += BATCH_SIZE, ++m_debug_buffer_idx)
			{
				if (!bgfx::isValid(m_debug_vertex_buffers[m_debug_buffer_idx]))
				{
					m_debug_vertex_buffers[m_debug_buffer_idx] = bgfx::createDynamicVertexBuffer(0xffFF, m_base_vertex_decl);
				}

				e_int32 tri_count = Math::minimum(BATCH_SIZE, tris.size() - j);
				auto* mem = bgfx::alloc(sizeof(BaseVertex) * 3 * tri_count);

				BaseVertex* vertex = (BaseVertex*)mem->data;
				for (e_int32 i = 0; i < tri_count; ++i)
				{
					const DebugTriangle& tri = tris[j + i];
					vertex[0].rgba = tri.color;
					vertex[0].x = tri.p0.x;
					vertex[0].y = tri.p0.y;
					vertex[0].z = tri.p0.z;
					vertex[0].u = vertex[0].v = 0;

					vertex[1].rgba = tri.color;
					vertex[1].x = tri.p1.x;
					vertex[1].y = tri.p1.y;
					vertex[1].z = tri.p1.z;
					vertex[1].u = vertex[0].v = 0;

					vertex[2].rgba = tri.color;
					vertex[2].x = tri.p2.x;
					vertex[2].y = tri.p2.y;
					vertex[2].z = tri.p2.z;
					vertex[2].u = vertex[0].v = 0;

					vertex += 3;
				}

				bgfx::updateDynamicVertexBuffer(m_debug_vertex_buffers[m_debug_buffer_idx], 0, mem);

				bgfx::setVertexBuffer(0, m_debug_vertex_buffers[m_debug_buffer_idx]);
				bgfx::setIndexBuffer(m_debug_index_buffer, 0, tri_count * 3);
				bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
				bgfx::setState(view.render_state | m_debug_line_material->getRenderStates());
				bgfx::submit(
					m_current_view->bgfx_id, m_debug_line_material->getShaderInstance().getProgramHandle(m_pass_idx));
			}
		}

		e_void Pipeline::setPointLightUniforms(ComponentHandle light_cmp)
		{
			if (!light_cmp.isValid()) return;
			if (!m_current_view) return;

			ComponentManager& com_man = m_scene->getComponentManager();
			GameObject light_entity = m_scene->getPointLightGameObject(light_cmp);
			float3 light_pos = com_man.getPosition(light_entity);
			float3 light_dir = com_man.getRotation(light_entity).rotate(float3(0, 0, -1));
			e_float fov = m_scene->getLightFOV(light_cmp);
			e_float intensity = m_scene->getPointLightIntensity(light_cmp);
			intensity *= intensity;
			float3 color = m_scene->getPointLightColor(light_cmp) * intensity;
			e_float range = m_scene->getLightRange(light_cmp);
			e_float attenuation = m_scene->getLightAttenuation(light_cmp);
			float4 light_pos_radius(light_pos, range);
			float4 light_color_attenuation(color, attenuation);
			float4 light_dir_fov(light_dir, fov);

			m_current_view->command_buffer.setUniform(m_light_pos_radius_uniform, light_pos_radius);
			m_current_view->command_buffer.setUniform(m_light_color_attenuation_uniform, light_color_attenuation);
			m_current_view->command_buffer.setUniform(m_light_dir_fov_uniform, light_dir_fov);

			FrameBuffer* shadowmap = nullptr;
			if (m_scene->getLightCastShadows(light_cmp))
			{
				for (auto& info : m_point_light_shadowmaps)
				{
					if (info.light == light_cmp)
					{
						shadowmap = info.framebuffer;
						m_current_view->command_buffer.setUniform(m_shadowmap_matrices_uniform,
							&info.matrices[0],
							m_scene->getLightFOV(light_cmp) > Math::C_Pi ? 4 : 1);
						break;
					}
				}
			}
			if (shadowmap)
			{
				m_current_view->command_buffer.setLocalShadowmap(shadowmap->getRenderbufferHandle(0));
			}
			else
			{
				m_current_view->command_buffer.setLocalShadowmap(BGFX_INVALID_HANDLE);
			}
			m_current_view->command_buffer.end();
		}


		e_void Pipeline::setStencilRef(e_uint32 ref)
		{
			if (!m_current_view) return;
			m_current_view->stencil |= BGFX_STENCIL_FUNC_REF(ref);
		}


		e_void Pipeline::setStencilRMask(e_uint32 rmask)
		{
			if (!m_current_view) return;
			m_current_view->stencil |= BGFX_STENCIL_FUNC_RMASK(rmask);
		}


		e_void Pipeline::setStencil(e_uint32 flags)
		{
			if (!m_current_view) return;
			m_current_view->stencil |= flags;
		}


		e_void Pipeline::setActiveGlobalLightUniforms()
		{
			if (!m_current_view) 
				return;

			auto current_light			= m_scene->getActiveGlobalLight();
			if (!current_light.isValid()) 
				return;

			ComponentManager& com_man	= m_scene->getComponentManager();
			GameObject light_entity		= m_scene->getGlobalLightGameObject(current_light);
			float3 light_dir			= com_man.getRotation(light_entity).rotate(float3(0, 0, 1));
			float3 diffuse_color		= m_scene->getGlobalLightColor(current_light) * m_scene->getGlobalLightIntensity(current_light);
			float3 fog_color			= m_scene->getFogColor(current_light);
			e_float fog_density			= m_scene->getFogDensity(current_light);
			e_float indirect_intensity	= m_scene->getGlobalLightIndirectIntensity(current_light);

			m_current_view->command_buffer.beginAppend();
			m_current_view->command_buffer.setUniform(m_light_color_indirect_intensity_uniform, float4(diffuse_color, indirect_intensity));
			m_current_view->command_buffer.setUniform(m_light_dir_fov_uniform, float4(light_dir, 0));

			fog_density *= fog_density * fog_density;
			m_current_view->command_buffer.setUniform(m_fog_color_density_uniform, float4(fog_color, fog_density));
			m_current_view->command_buffer.setUniform(m_fog_params_uniform,
				float4(m_scene->getFogBottom(current_light),
									 m_scene->getFogHeight(current_light),
									 0,
									 0));
			if (m_global_light_shadowmap && !m_is_rendering_in_shadowmap)
			{
				m_current_view->command_buffer.setUniform(m_shadowmap_matrices_uniform, m_shadow_viewprojection, 4);
				m_current_view->command_buffer.setGlobalShadowmap();
			}
			m_current_view->command_buffer.end();
		}

		e_void Pipeline::disableBlending()
		{
			if (!m_current_view) return;
			m_current_view->render_state &= ~BGFX_STATE_BLEND_MASK;
		}

		e_void Pipeline::enableDepthWrite()
		{
			if (!m_current_view) return;
			m_current_view->render_state |= BGFX_STATE_DEPTH_WRITE;
		}

		e_void Pipeline::disableDepthWrite()
		{
			if (!m_current_view) return;
			m_current_view->render_state &= ~BGFX_STATE_DEPTH_WRITE;
		}

		e_void Pipeline::enableAlphaWrite()
		{
			if (!m_current_view) return;
			m_current_view->render_state |= BGFX_STATE_ALPHA_WRITE;
		}
		e_void Pipeline::disableAlphaWrite()
		{
			if (!m_current_view) return;
			m_current_view->render_state &= ~BGFX_STATE_ALPHA_WRITE;
		}

		e_void Pipeline::enableRGBWrite()
		{
			if (!m_current_view) return;
			m_current_view->render_state |= BGFX_STATE_RGB_WRITE;
		}
		e_void Pipeline::disableRGBWrite()
		{
			if (!m_current_view) return;
			m_current_view->render_state &= ~BGFX_STATE_RGB_WRITE;
		}


		e_void Pipeline::renderPointLightInfluencedGeometry(ComponentHandle light)
		{
			//PROFILE_FUNCTION();

			TArrary<EntityInstanceMesh> tmp_meshes(m_renderer.getEngine().getLIFOAllocator());
			m_scene->getPointLightInfluencedGeometry(light, tmp_meshes);
			renderMeshes(tmp_meshes);
		}


		e_void Pipeline::renderPointLightInfluencedGeometry(const Frustum& frustum)
		{
			//PROFILE_FUNCTION();

			TArrary<ComponentHandle> lights(m_allocator);
			m_scene->getPointLights(frustum, lights);
			IAllocator& frame_allocator = m_renderer.getEngine().getLIFOAllocator();
			m_is_current_light_global	= false;
			for (e_int32 i = 0; i < lights.size(); ++i)
			{
				ComponentHandle light = lights[i];
				setPointLightUniforms(light);

				{
					TArrary<EntityInstanceMesh> tmp_meshes(frame_allocator);
					m_scene->getPointLightInfluencedGeometry(light, frustum, tmp_meshes);
					renderMeshes(tmp_meshes);
				}

				//{
				//	TVector<TerrainInfo> tmp_terrains(frame_allocator);
				//	GameObject camera_entity = m_scene->getCameraEntity(m_applied_camera);
				//	float3 lod_ref_point = m_scene->getComponentManager().getPosition(camera_entity);
				//	Frustum frustum = m_scene->getCameraFrustum(m_applied_camera);
				//	m_scene->getTerrainInfos(frustum, lod_ref_point, tmp_terrains);
				//	renderTerrains(tmp_terrains);
				//}

				//{
				//	TVector<GrassInfo> tmp_grasses(frame_allocator);
				//	m_scene->getGrassInfos(frustum, m_applied_camera, tmp_grasses);
				//	renderGrasses(tmp_grasses);
				//}
			}
		}


		e_uint64 Pipeline::getLayerMask(const e_char* layer)
		{
			return e_uint64(1) << m_renderer.getLayer(layer);
		}


		e_void Pipeline::drawQuadEx(e_float left, e_float top, e_float w, e_float h, e_float u0, e_float v0, e_float u1, e_float v1, e_int32 material_index)
		{
			Resource* res = m_scene->getEngine().getLuaResource(material_index);
			Material* material = static_cast<Material*>(res);
			drawQuadExMaterial(left, top, w, h, u0, v0, u1, v1, material);
		}


		e_void Pipeline::drawQuadExMaterial(e_float left, e_float top, e_float w, e_float h, e_float u0, e_float v0, e_float u1, e_float v1, Material* material)
		{
			if (!m_current_view) return;
			if (!material->isReady() || bgfx::getAvailTransientVertexBuffer(3, m_base_vertex_decl) < 3)
			{
				bgfx::touch(m_current_view->bgfx_id);
				return;
			}

			float4x4 projection_mtx;
			projection_mtx.setOrtho(0, 1, 0, 1, 0, 30, bgfx::getCaps()->homogeneousDepth);

			bgfx::setViewTransform(m_current_view->bgfx_id, &float4x4::IDENTITY.m11, &projection_mtx.m11);
			if (m_current_framebuffer)
			{
				bgfx::setViewRect(m_current_view->bgfx_id,
					0,
					0,
					(e_uint16)m_current_framebuffer->getWidth(),
					(e_uint16)m_current_framebuffer->getHeight());
			}
			else
			{
				bgfx::setViewRect(m_current_view->bgfx_id, 0, 0, (e_uint16)m_width, (e_uint16)m_height);
			}

			bgfx::TransientVertexBuffer vb;
			bgfx::allocTransientVertexBuffer(&vb, 6, m_base_vertex_decl);
			BaseVertex* vertex = (BaseVertex*)vb.data;
			e_float right = left + w;
			e_float bottom = top + h;
			if (!bgfx::getCaps()->originBottomLeft)
			{
				top = 1 - top;
				bottom = 1 - bottom;
			}

			vertex[0].x = left;
			vertex[0].y = top;
			vertex[0].z = 0;
			vertex[0].rgba = 0xffffffff;
			vertex[0].u = u0;
			vertex[0].v = v0;

			vertex[1].x = right;
			vertex[1].y = top;
			vertex[1].z = 0;
			vertex[1].rgba = 0xffffffff;
			vertex[1].u = u1;
			vertex[1].v = v0;

			vertex[2].x = right;
			vertex[2].y = bottom;
			vertex[2].z = 0;
			vertex[2].rgba = 0xffffffff;
			vertex[2].u = u1;
			vertex[2].v = v1;

			vertex[3].x = left;
			vertex[3].y = top;
			vertex[3].z = 0;
			vertex[3].rgba = 0xffffffff;
			vertex[3].u = u0;
			vertex[3].v = v0;

			vertex[4].x = right;
			vertex[4].y = bottom;
			vertex[4].z = 0;
			vertex[4].rgba = 0xffffffff;
			vertex[4].u = u1;
			vertex[4].v = v1;

			vertex[5].x = left;
			vertex[5].y = bottom;
			vertex[5].z = 0;
			vertex[5].rgba = 0xffffffff;
			vertex[5].u = u0;
			vertex[5].v = v1;

			View& view = *m_current_view;

			executeCommandBuffer(material->getCommandBuffer(), material);
			executeCommandBuffer(view.command_buffer.buffer, material);

			if (m_applied_camera.isValid())
			{
				float4x4 projection_matrix;
				ComponentManager& com_man	= m_scene->getComponentManager();
				e_float fov					= m_scene->getCameraFOV(m_applied_camera);
				e_float near_plane			= m_scene->getCameraNearPlane(m_applied_camera);
				e_float far_plane			= m_scene->getCameraFarPlane(m_applied_camera);
				e_float ratio				= e_float(m_width) / m_height;
				GameObject camera_entity	= m_scene->getCameraGameObject(m_applied_camera);
				float4x4 inv_view_matrix	= com_man.getPositionAndRotation(camera_entity);
				float4x4 view_matrix		= inv_view_matrix;

				view_matrix.fastInverse();
				projection_matrix.setPerspective(fov, ratio, near_plane, far_plane, bgfx::getCaps()->homogeneousDepth);
				float4x4 inv_projection = projection_matrix;
				inv_projection.inverse();

				float4x4 inv_view_proj = projection_matrix * view_matrix;
				inv_view_proj.inverse();

				bgfx::setUniform(m_cam_inv_proj_uniform,		&inv_projection.m11);
				bgfx::setUniform(m_cam_inv_viewproj_uniform,	&inv_view_proj.m11);
				bgfx::setUniform(m_cam_view_uniform,			&view_matrix.m11);
				bgfx::setUniform(m_cam_proj_uniform,			&projection_matrix.m11);
				bgfx::setUniform(m_cam_inv_view_uniform,		&inv_view_matrix.m11);

				auto cam_params = float4(near_plane, far_plane, fov, ratio);
				bgfx::setUniform(m_cam_params,					&cam_params);
			}

			bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
			bgfx::setState((view.render_state | material->getRenderStates()) & ~BGFX_STATE_CULL_MASK);
			bgfx::setVertexBuffer(0, &vb);
			++m_stats.draw_call_count;
			++m_stats.instance_count;
			m_stats.triangle_count += 2;
			bgfx::submit(m_current_view->bgfx_id, material->getShaderInstance().getProgramHandle(m_pass_idx));
		}


		e_void Pipeline::drawQuad(e_float left, e_float top, e_float w, e_float h, e_int32 material_index)
		{
			drawQuadEx(left, top, w, h, 0, 0, 1, 1, material_index);
		}


		e_void Pipeline::renderAll(const Frustum& frustum, e_bool render_grass, ComponentHandle camera, e_uint64 layer_mask)
		{
			//PROFILE_FUNCTION();

			if (!m_applied_camera.isValid()) 
				return;

			GameObject camera_entity = m_scene->getCameraGameObject(camera);
			float3 lod_ref_point = m_scene->getComponentManager().getPosition(camera_entity);
			m_is_current_light_global = true;

			//m_grasses_buffer.clear();
			//m_terrains_buffer.clear();

			JobSystem::JobDecl jobs[1];
			JobSystem::LambdaJob job_storage[1];

			JobSystem::fromLambda(
				[this, &frustum, &lod_ref_point, layer_mask, camera]()
				{
					m_mesh_buffer = &m_scene->getEntityInstanceInfos(frustum, lod_ref_point, camera, layer_mask);
				},
				&job_storage[0], &jobs[0], nullptr);

			//JobSystem::fromLambda([this, &frustum, &lod_ref_point]() {
			//	m_scene->getTerrainInfos(frustum, lod_ref_point, m_terrains_buffer);
			//}, &job_storage[1], &jobs[1], nullptr);

			if (render_grass)
			{
				//JobSystem::fromLambda([this, &frustum]() {
				//	m_scene->getGrassInfos(frustum, m_applied_camera, m_grasses_buffer);
				//}, &job_storage[2], &jobs[2], nullptr);
			}

			volatile e_int32 counter = 0;
			JobSystem::runJobs(jobs, render_grass ? 2 : 1, &counter);
			JobSystem::wait(&counter);
		
			renderMeshes(*m_mesh_buffer);

			//if(render_grass) 
			//	renderGrasses(m_grasses_buffer);
			//renderTerrains(m_terrains_buffer);
		}


		e_void Pipeline::toggleStats()
		{
			m_debug_flags ^= BGFX_DEBUG_STATS;
			bgfx::setDebug(m_debug_flags);
		}


		e_void Pipeline::setWindowHandle(e_void* data)
		{
			m_default_framebuffer =	_aligned_new(m_allocator, FrameBuffer)("default", m_width, m_height, data);
		}


		e_void Pipeline::renderEntity(Entity& model, Pose* pose, const float4x4& mtx)
		{
			GameObject camera_entity = m_scene->getCameraGameObject(m_applied_camera);
			float3 camera_pos = m_scene->getComponentManager().getPosition(camera_entity);

			for (e_int32 i = 0; i < model.getMeshCount(); ++i)
			{
				Mesh& mesh = model.getMesh(i);
				switch (mesh.type)
				{
					case Mesh::RIGID_INSTANCED:
						renderRigidMeshInstanced(mtx, mesh);
						break;
					case Mesh::RIGID:
					{
						e_float depth = (camera_pos - mtx.getTranslation()).squaredLength();
						renderRigidMesh(mtx, mesh, depth);
						break;
					}
					case Mesh::MULTILAYER_RIGID:
						renderMultilayerRigidMesh(model, mtx, mesh);
						break;
					case Mesh::MULTILAYER_SKINNED:
						renderMultilayerSkinnedMesh(*pose, model, mtx, mesh);
						break;
					case Mesh::SKINNED:
						if(pose) renderSkinnedMesh(*pose, model, mtx, mesh);
						break;
				}
			}
		}


		e_void Pipeline::renderSkinnedMesh(const Pose& pose, const Entity& model, const float4x4& matrix, const Mesh& mesh)
		{
			Material* material = mesh.material;
			auto& shader_instance = mesh.material->getShaderInstance();

			material->setDefine(m_instanced_define_idx, false);

			float4x4 bone_mtx[196];

			float3* poss = pose.positions;
			Quaternion* rots = pose.rotations;

			ASSERT(pose.count <= TlengthOf(bone_mtx));
			for (e_int32 bone_index = 0, bone_count = pose.count; bone_index < bone_count; ++bone_index)
			{
				auto& bone = model.getBone(bone_index);
				RigidTransform tmp = {poss[bone_index], rots[bone_index]};
				bone_mtx[bone_index] = (tmp * bone.inv_bind_transform).toMatrix();
			}

			e_int32 view_idx = m_layer_to_view_map[material->getRenderLayer()];
			ASSERT(view_idx >= 0);
			auto& view = m_views[view_idx >= 0 ? view_idx : 0];

			if (!bgfx::isValid(shader_instance.getProgramHandle(view.pass_idx))) return;

			bgfx::setUniform(m_bone_matrices_uniform, bone_mtx, pose.count);
			executeCommandBuffer(material->getCommandBuffer(), material);
			executeCommandBuffer(view.command_buffer.buffer, material);

			bgfx::setTransform(&matrix);
			bgfx::setVertexBuffer(0, mesh.vertex_buffer_handle);
			bgfx::setIndexBuffer(mesh.index_buffer_handle);
			bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
			bgfx::setState(view.render_state | material->getRenderStates());
			++m_stats.draw_call_count;
			++m_stats.instance_count;
			m_stats.triangle_count += mesh.indices_count / 3;
			bgfx::submit(view.bgfx_id, shader_instance.getProgramHandle(view.pass_idx));
		}


		e_void Pipeline::renderMultilayerRigidMesh(const Entity& model, const float4x4& float4x4, const Mesh& mesh)
		{
			Material* material = mesh.material;

			material->setDefine(m_instanced_define_idx, true);

			e_int32 layers_count = material->getLayersCount();
			auto& shader_instance = mesh.material->getShaderInstance();

			auto renderLayer = [&](View& view) {
				executeCommandBuffer(material->getCommandBuffer(), material);
				executeCommandBuffer(view.command_buffer.buffer, material);

				bgfx::setTransform(&float4x4);
				bgfx::setVertexBuffer(0, mesh.vertex_buffer_handle);
				bgfx::setIndexBuffer(mesh.index_buffer_handle);
				bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
				bgfx::setState(view.render_state | material->getRenderStates());
				++m_stats.draw_call_count;
				++m_stats.instance_count;
				m_stats.triangle_count += mesh.indices_count / 3;
				bgfx::submit(view.bgfx_id, shader_instance.getProgramHandle(view.pass_idx));
			};

			e_int32 view_idx = m_layer_to_view_map[material->getRenderLayer()];
			if (view_idx >= 0 && !m_is_rendering_in_shadowmap)
			{
				auto& view = m_views[view_idx];
				if (bgfx::isValid(shader_instance.getProgramHandle(view.pass_idx)))
				{
					for (e_int32 i = 0; i < layers_count; ++i)
					{
						float4 layer((i + 1) / (e_float)layers_count, 0, 0, 0);
						bgfx::setUniform(m_layer_uniform, &layer);
						renderLayer(view);
					}
				}
			}

			static const e_int32 default_layer = m_renderer.getLayer("default");
			e_int32 default_view_idx = m_layer_to_view_map[default_layer];
			if (default_view_idx < 0) return;
			View& default_view = m_views[default_view_idx];
			renderLayer(default_view);
		}


		e_void Pipeline::renderRigidMesh(const float4x4& matrix, Mesh& mesh, e_float depth)
		{
			Material* material = mesh.material;

			material->setDefine(m_instanced_define_idx, false);

			e_int32 view_idx = m_layer_to_view_map[material->getRenderLayer()];
			ASSERT(view_idx >= 0);
			auto& view = m_views[view_idx >= 0 ? view_idx : 0];

			executeCommandBuffer(material->getCommandBuffer(), material);
			executeCommandBuffer(view.command_buffer.buffer, material);
			 
			bgfx::setTransform(&matrix);
			bgfx::setVertexBuffer(0, mesh.vertex_buffer_handle);
			bgfx::setIndexBuffer(mesh.index_buffer_handle);
			bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);

			bgfx::setState(0
				| BGFX_STATE_RGB_WRITE
				| BGFX_STATE_ALPHA_WRITE
				| BGFX_STATE_DEPTH_WRITE
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CCW
				| BGFX_STATE_PT_TRISTRIP
				//| res->getRenderStates()
			);

			//bgfx::setState(view.render_state | material->getRenderStates());
			ShaderInstance& shader_instance = material->getShaderInstance();
			++m_stats.draw_call_count;
			++m_stats.instance_count;
			m_stats.triangle_count += mesh.indices_count / 3;
			bgfx::submit(0, shader_instance.getProgramHandle(2), Math::floatFlip(*(e_uint32*)&depth));
		}


		e_void Pipeline::renderMultilayerSkinnedMesh(const Pose& pose, const Entity& model, const float4x4& matrix, const Mesh& mesh)
		{
			Material* material = mesh.material;

			material->setDefine(m_instanced_define_idx, false);

			float4x4 bone_mtx[196];
			float3* poss = pose.positions;
			Quaternion* rots = pose.rotations;

			ASSERT(pose.count <= TlengthOf(bone_mtx));
			for (e_int32 bone_index = 0, bone_count = pose.count; bone_index < bone_count; ++bone_index)
			{
				auto& bone = model.getBone(bone_index);
				RigidTransform tmp = { poss[bone_index], rots[bone_index] };
				bone_mtx[bone_index] = (tmp * bone.inv_bind_transform).toMatrix();
			}

			e_int32 layers_count = material->getLayersCount();
			auto& shader_instance = mesh.material->getShaderInstance();

			auto renderLayer = [&](View& view) {
				bgfx::setUniform(m_bone_matrices_uniform, bone_mtx, pose.count);
				executeCommandBuffer(material->getCommandBuffer(), material);
				executeCommandBuffer(view.command_buffer.buffer, material);

				bgfx::setTransform(&matrix);
				bgfx::setVertexBuffer(0, mesh.vertex_buffer_handle);
				bgfx::setIndexBuffer(mesh.index_buffer_handle);
				bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
				bgfx::setState(view.render_state | material->getRenderStates());
				++m_stats.draw_call_count;
				++m_stats.instance_count;
				m_stats.triangle_count += mesh.indices_count / 3;
				bgfx::submit(view.bgfx_id, shader_instance.getProgramHandle(view.pass_idx));
			};

			e_int32 view_idx = m_layer_to_view_map[material->getRenderLayer()];
			if (view_idx >= 0 && !m_is_rendering_in_shadowmap)
			{
				auto& view = m_views[view_idx];
				if (bgfx::isValid(shader_instance.getProgramHandle(view.pass_idx)))
				{
					for (e_int32 i = 0; i < layers_count; ++i)
					{
						float4 layer((i + 1) / (e_float)layers_count, 0, 0, 0);
						bgfx::setUniform(m_layer_uniform, &layer);
						renderLayer(view);
					}
				}
			}

			static const e_int32 default_layer = m_renderer.getLayer("default");
			e_int32 default_view_idx = m_layer_to_view_map[default_layer];
			if (default_view_idx < 0) return;
			View& default_view = m_views[default_view_idx];
			renderLayer(default_view);
		}


		e_void Pipeline::setScissor(e_int32 x, e_int32 y, e_int32 width, e_int32 height)
		{
			bgfx::setScissor(x, y, width, height);
		}


		e_bool Pipeline::checkAvailTransientBuffers(e_uint32 num_vertices, const bgfx::VertexDecl& decl, e_uint32 num_indices)
		{
			return bgfx::getAvailTransientIndexBuffer(num_indices) >= num_indices &&
				bgfx::getAvailTransientVertexBuffer(num_vertices, decl) >= num_vertices;
		}


		e_void Pipeline::allocTransientBuffers(bgfx::TransientVertexBuffer* tvb,
			e_uint32 num_vertices,
			const bgfx::VertexDecl& decl,
			bgfx::TransientIndexBuffer* tib,
			e_uint32 num_indices)
		{
			bgfx::allocTransientIndexBuffer(tib, num_indices);
			bgfx::allocTransientVertexBuffer(tvb, num_vertices, decl);
		}


		e_void Pipeline::destroyUniform(bgfx::UniformHandle uniform) { bgfx::destroy(uniform); }
	
	
		bgfx::UniformHandle Pipeline::createTextureUniform(const e_char* name)
		{
			return bgfx::createUniform(name, bgfx::UniformType::Int1);
		}


		bgfx::TextureHandle Pipeline::createTexture(e_int32 width, e_int32 height, const e_uint32* data)
		{
			return bgfx::createTexture2D(
				width, height, false, 1, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(data, 4 * width * height));
		}


		e_void Pipeline::destroyTexture(bgfx::TextureHandle texture)
		{
			bgfx::destroy(texture);
		}


		e_void Pipeline::setTexture(e_int32 slot, bgfx::TextureHandle texture, bgfx::UniformHandle uniform)
		{
			bgfx::setTexture(slot, uniform, texture);
		}


		e_void Pipeline::frame(const bgfx::TransientVertexBuffer& vertex_buffer,
			const bgfx::TransientIndexBuffer& index_buffer,
			const float4x4& mtx,
			e_int32 first_index,
			e_int32 num_indices,
			e_uint64 render_states,
			ShaderInstance& shader_instance)
		{
			ASSERT(m_current_view);
			View& view = *m_current_view;
			bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
			bgfx::setState(view.render_state | render_states);
			bgfx::setTransform(&mtx.m11);
			bgfx::setVertexBuffer(0, &vertex_buffer);
			bgfx::setIndexBuffer(&index_buffer, first_index, num_indices);
			++m_stats.draw_call_count;
			++m_stats.instance_count;
			m_stats.triangle_count += num_indices / 3;
			bgfx::submit(m_current_view->bgfx_id, shader_instance.getProgramHandle(m_pass_idx));
		}


		e_void Pipeline::renderRigidMeshInstanced(const float4x4& matrix, Mesh& mesh)
		{
			e_int32 instance_idx = mesh.instance_idx;
			if (instance_idx == -1)
			{
				instance_idx = m_instance_data_idx;
				m_instance_data_idx = (m_instance_data_idx + 1) % TlengthOf(m_instances_data);
				if (m_instances_data[instance_idx].buffer && m_instances_data[instance_idx].buffer->data)
				{
					finishInstances(instance_idx);
				}
				InstanceData& data = m_instances_data[instance_idx];
				if (bgfx::getAvailInstanceDataBuffer(MAX_INSTANCE_COUNT, sizeof(float4x4)) < MAX_INSTANCE_COUNT)
				{
					log_error("Renderer Could not allocate instance data buffer");
					return;
				}

				data.buffer = bgfx::allocInstanceDataBuffer(MAX_INSTANCE_COUNT, sizeof(float4x4));
				data.instance_count = 0;
				data.mesh = &mesh;
				mesh.instance_idx = instance_idx;

			}

			InstanceData& data = m_instances_data[instance_idx];
			float* mtcs = (float*)data.buffer->data;
			StringUnitl::copyMemory(&mtcs[data.instance_count * 16], &matrix, sizeof(float4x4));

			++data.instance_count;
			
			if (data.instance_count == MAX_INSTANCE_COUNT)
			{
				finishInstances(instance_idx);
			}
		}


		e_void Pipeline::frame(const bgfx::VertexBufferHandle& vertex_buffer,
			const bgfx::IndexBufferHandle& index_buffer,
			const bgfx::InstanceDataBuffer& instance_buffer,
			e_int32 count,
			Material& material)
		{
			if (!m_current_view) return;
			View& view = *m_current_view;

			executeCommandBuffer(material.getCommandBuffer(), &material);
			executeCommandBuffer(view.command_buffer.buffer, &material);

			bgfx::setInstanceDataBuffer(&instance_buffer, count);
			bgfx::setVertexBuffer(0, vertex_buffer);
			bgfx::setIndexBuffer(index_buffer);
			bgfx::setStencil(view.stencil, BGFX_STENCIL_NONE);
			bgfx::setState(view.render_state | material.getRenderStates());
			++m_stats.draw_call_count;
			m_stats.instance_count += count;
			m_stats.triangle_count += count * 2;

			bgfx::submit(view.bgfx_id, material.getShaderInstance().getProgramHandle(view.pass_idx));
		}


		e_void Pipeline::executeCommandBuffer(const e_uint8* data, Material* material) const
		{
			const e_uint8* ip = data;
			for (;;)
			{
				switch ((BufferCommands)*ip)
				{
					case BufferCommands::END:
						return;
					case BufferCommands::SET_TEXTURE:
					{
						auto cmd = (SetTextureCommand*)ip;
						bgfx::setTexture(cmd->stage, cmd->uniform, cmd->texture, cmd->flags);
						ip += sizeof(*cmd);
						break;
					}
					case BufferCommands::SET_UNIFORM_TIME:
					{
						auto cmd = (SetUniformTimeCommand*)ip;
						auto uniform_time = float4(m_scene->getTime(), 0, 0, 0);
						bgfx::setUniform(cmd->uniform, &uniform_time);
						ip += sizeof(*cmd);
						break;
					}
					case BufferCommands::SET_UNIFORM_VEC4:
					{
						auto cmd = (SetUniformVec4Command*)ip;
						bgfx::setUniform(cmd->uniform, &cmd->value);
						ip += sizeof(*cmd);
						break;
					}
					case BufferCommands::SET_UNIFORM_ARRAY:
					{
						auto cmd = (SetUniformArrayCommand*)ip;
						ip += sizeof(*cmd);
						bgfx::setUniform(cmd->uniform, ip, cmd->count);
						ip += cmd->size;
						break;
					}
					case BufferCommands::SET_GLOBAL_SHADOWMAP:
					{
						auto handle = m_global_light_shadowmap->getRenderbufferHandle(0);
						bgfx::setTexture(15 - m_global_textures_count,
							m_tex_shadowmap_uniform,
							handle, FrameBuffer::RenderBuffer::DEFAULT_FLAGS & ~ (BGFX_TEXTURE_MAG_MASK | BGFX_TEXTURE_MIN_MASK));
						ip += 1;
						break;
					}
					case BufferCommands::SET_LOCAL_SHADOWMAP:
					{
						auto cmd = (SetLocalShadowmapCommand*)ip;
						material->setDefine(m_has_shadowmap_define_idx, bgfx::isValid(cmd->texture));
						bgfx::setTexture(15 - m_global_textures_count,
							m_tex_shadowmap_uniform,
							cmd->texture);
						ip += sizeof(*cmd);
						break;
					}
					default:
						ASSERT(false);
						break;
				}
			}
		}

		e_void Pipeline::renderMeshes(const TArrary<EntityInstanceMesh>& meshes)
		{
			PROFILE_FUNCTION();
			if (meshes.empty()) return;

			EntityInstance* model_instances = m_scene->getEntityInstances();
			PROFILE_INT("mesh count", meshes.size());
			for (auto& mesh : meshes)
			{
				EntityInstance& model_instance = model_instances[mesh.entity_instance.index];
				switch (mesh.mesh->type)
				{
				case Mesh::RIGID_INSTANCED:
					renderRigidMeshInstanced(model_instance.matrix, *mesh.mesh);
					break;
				case Mesh::RIGID:
					renderRigidMesh(model_instance.matrix, *mesh.mesh, mesh.depth);
					break;
				case Mesh::SKINNED:
					renderSkinnedMesh(*model_instance.pose, *model_instance.entity, model_instance.matrix, *mesh.mesh);
					break;
				case Mesh::MULTILAYER_SKINNED:
					renderMultilayerSkinnedMesh(*model_instance.pose, *model_instance.entity, model_instance.matrix, *mesh.mesh);
					break;
				case Mesh::MULTILAYER_RIGID:
					renderMultilayerRigidMesh(*model_instance.entity, model_instance.matrix, *mesh.mesh);
					break;
				}
			}
			finishInstances();
		}


		e_void Pipeline::renderMeshes(const TArrary<TArrary<EntityInstanceMesh>>& meshes)
		{
			//PROFILE_FUNCTION();
			e_int32 mesh_count = 0;
			for (auto& submeshes : meshes)
			{
				if(submeshes.empty()) 
					continue;

				EntityInstance* model_instances = m_scene->getEntityInstances();
				mesh_count += submeshes.size();
				for (auto& mesh : submeshes)
				{
					EntityInstance& model_instance = model_instances[mesh.entity_instance.index];
					switch (mesh.mesh->type)
					{
						case Mesh::RIGID_INSTANCED:
							renderRigidMeshInstanced(model_instance.matrix, *mesh.mesh);
							break;
						case Mesh::RIGID:
							renderRigidMesh(model_instance.matrix, *mesh.mesh, mesh.depth);
							break;
						case Mesh::SKINNED:
							renderSkinnedMesh(*model_instance.pose, *model_instance.entity, model_instance.matrix, *mesh.mesh);
							break;
						case Mesh::MULTILAYER_SKINNED:
							renderMultilayerSkinnedMesh(*model_instance.pose, *model_instance.entity, model_instance.matrix, *mesh.mesh);
							break;
						case Mesh::MULTILAYER_RIGID:
							renderMultilayerRigidMesh(*model_instance.entity, model_instance.matrix, *mesh.mesh);
							break;
					}
				}
			}
			finishInstances();
			//PROFILE_INT("mesh count", mesh_count);
		}


		e_void Pipeline::resize(e_int32 w, e_int32 h)
		{
			if (m_width == w && m_height == h) return;

			if (m_default_framebuffer)
			{
				m_default_framebuffer->resize(w, h);
			}
			for (auto& i : m_framebuffers)
			{
				auto size_ratio = i->getSizeRatio();
				if (size_ratio.x > 0 || size_ratio.y > 0)
				{
					i->resize(e_int32(w * size_ratio.x), e_int32(h * size_ratio.y));
				}
			}
			m_width = w;
			m_height = h;
		}

		e_int32 Pipeline::bindFramebufferTexture(const char* framebuffer_name, e_int32 renderbuffer_idx, e_int32 uniform_idx,e_uint32 flags)
		{
			if (!m_current_view) 
				return 0;

			FrameBuffer* fb = getFramebuffer(framebuffer_name);
			if (!fb) 
				return 0;

			float4 size;
			size.x = (float)fb->getWidth();
			size.y = (float)fb->getHeight();
			size.z = 1.0f / (float)fb->getWidth();
			size.w = 1.0f / (float)fb->getHeight();
			m_current_view->command_buffer.beginAppend();
			if (m_global_textures_count == 0)
			{
				m_current_view->command_buffer.setUniform(m_texture_size_uniform, size);
			}
			m_current_view->command_buffer.setTexture(15 - m_global_textures_count,
				m_uniforms[uniform_idx],
				fb->getRenderbufferHandle(renderbuffer_idx),
				flags);
			++m_global_textures_count;
			m_current_view->command_buffer.end();
			return 0;
		}

		e_int32 Pipeline::addFramebuffer(const e_char* name, e_int32 width, e_int32 height, bool is_screen_size, float2 size_ratio, TArrary<FrameBuffer::RenderBuffer>& buffers)
		{
			FrameBuffer* framebuffer = getFramebuffer(name);
			if (framebuffer)
			{
				log_waring("Renderer Trying to create already existing framebuffer %s.", name);
				return 0;
			}

			FrameBuffer::Declaration decl;
			StringUnitl::copyString(decl.m_name, name);

			decl.m_width = width;
			decl.m_height = height;

			decl.m_size_ratio = size_ratio;
			if (is_screen_size)
				decl.m_size_ratio = is_screen_size ? float2(1, 1) : float2(-1, -1);

			decl.m_renderbuffers_count = buffers.size();
			for (int i = 0; i < decl.m_renderbuffers_count; ++i)
			{
				FrameBuffer::RenderBuffer& buf = decl.m_renderbuffers[i];
				buf.m_format = buffers[i].m_format;
				buf.m_handle = buffers[i].m_handle;
				buf.m_shared = buffers[i].m_shared;
			}

			if ((decl.m_size_ratio.x > 0 || decl.m_size_ratio.y > 0) && m_height > 0)
			{
				decl.m_width  = e_int32(m_width * decl.m_size_ratio.x);
				decl.m_height = e_int32(m_height * decl.m_size_ratio.y);
			}

			auto* fb = _aligned_new(m_allocator, FrameBuffer)(decl);
			m_framebuffers.push_back(fb);

			if (StringUnitl::equalStrings(decl.m_name, "default"))
				m_default_framebuffer = fb;

			return 0;
		}

		e_void Pipeline::clearLayerToViewMap()
		{
			for (e_int32& i : m_layer_to_view_map)
			{
				i = -1;
			}
		}

		e_bool Pipeline::frame()
		{
			//PROFILE_FUNCTION();
			if (!isReady()) 
				return false;
			if (!m_scene) 
				return false;

			m_stats = {};
			m_applied_camera = INVALID_COMPONENT;
			m_global_light_shadowmap = nullptr;
			m_current_view = nullptr;
			m_view_idx = -1;
			m_layer_mask = 0;
			m_pass_idx = -1;
			m_current_framebuffer = m_default_framebuffer;
			m_instance_data_idx = 0;
			m_point_light_shadowmaps.clear();
			clearLayerToViewMap();
			for (e_int32 i = 0; i < TlengthOf(m_terrain_instances); ++i)
			{
				m_terrain_instances[i].m_count = 0;
			}
			for (e_int32 i = 0; i < TlengthOf(m_instances_data); ++i)
			{
				m_instances_data[i].buffer = nullptr;
				//m_instances_data[i].buffer.data = nullptr;
				m_instances_data[i].instance_count = 0;
			}
			bgfx::touch(0);

			//sky
			newView("main", 1);
			applyCamera("main");
			setPass("DEFERRED");
			clear(BGFX_CLEAR_COLOR, 0x303030ff);

			//disableDepthWrite();
			//enableBlending("alpha");

			//executeCommandBuffer(m_debug_line_material->getCommandBuffer(), m_debug_line_material);
			//renderDebugShapes();

			e_uint64 all_render_mask = getLayerMask("default") + getLayerMask("transparent") + getLayerMask("water") + getLayerMask("fur") + getLayerMask("no_shadows");
			renderAll(m_camera_frustum, false, m_applied_camera, all_render_mask);

			bool success = true;// lua_frame(this);

			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x0f, "FPS:%f", getFPS());

			finishInstances();
			return success;
		}

		e_int32 Pipeline::createUniform(const e_char* name)
		{
			bgfx::UniformHandle handle = bgfx::createUniform(name, bgfx::UniformType::Int1);
			m_uniforms.push_back(handle);
			return m_uniforms.size() - 1;
		}

		e_int32 Pipeline::createVec4ArrayUniform(const e_char* name, e_int32 num)
		{
			bgfx::UniformHandle handle = bgfx::createUniform(name, bgfx::UniformType::Vec4, num);
			m_uniforms.push_back(handle);
			return m_uniforms.size() - 1;
		}

		e_bool Pipeline::hasScene()
		{
			return m_scene != nullptr;
		}

		e_bool Pipeline::cameraExists(const e_char* slot_name)
		{
			return m_scene->getCameraInSlot(slot_name) != INVALID_COMPONENT;
		}

		e_void Pipeline::enableBlending(const e_char* mode)
		{
			if (!m_current_view)
				return;

			e_uint64 mode_value = 0;
			if (StringUnitl::equalStrings(mode, "alpha")) 
				mode_value = BGFX_STATE_BLEND_ALPHA;
			else if (StringUnitl::equalStrings(mode, "add")) 
				mode_value = BGFX_STATE_BLEND_ADD;
			else if (StringUnitl::equalStrings(mode, "multiply"))
				mode_value = BGFX_STATE_BLEND_MULTIPLY;

			m_current_view->render_state |= mode_value;
		}

		e_void Pipeline::clear(e_uint32 flags, e_uint32 color)
		{
			if (!m_current_view) return;
			bgfx::setViewClear(m_current_view->bgfx_id, (e_uint16)flags, color, 1.0f, 0);
			bgfx::touch(m_current_view->bgfx_id);
		}

		e_void Pipeline::renderPointLightLitGeometry()
		{
			renderPointLightInfluencedGeometry(m_camera_frustum);
		}

		e_bool Pipeline::isReady() const { return m_is_ready; }

		SceneManager* Pipeline::getScene()
		{
			return m_scene;
		}

		e_void Pipeline::setScene(SceneManager* scene)
		{
			m_scene = scene;
			if (m_lua_state && m_scene)
				lua_callInitScene(this);
		}


	Pipeline* Pipeline::create(Renderer& renderer, const ArchivePath& path, const e_char* defines, IAllocator& allocator)
	{
		return _aligned_new(allocator, Pipeline)(renderer, path, defines, allocator);
	}

	e_void Pipeline::destroy(Pipeline* pipeline)
	{
		_delete(static_cast<Pipeline*>(pipeline)->m_allocator, pipeline);
	}

}