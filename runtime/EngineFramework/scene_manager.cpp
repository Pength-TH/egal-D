#include "runtime/EngineFramework/scene_manager.h"

#include "common/resource/resource_public.h"
#include "common/resource/resource_manager.h"
#include "common/egal-d.h"

#include "runtime/EngineFramework/buffer.h"

#include "runtime/EngineFramework/engine_root.h"
#include "runtime/EngineFramework/scene_manager.h"
#include "runtime/EngineFramework/renderer.h"
#include "runtime/EngineFramework/culling_system.h"
#include "runtime/EngineFramework/pipeline.h"

#include "common/resource/entity_manager.h"
#include "common/resource/material_manager.h"
#include "common/resource/texture_manager.h"
#include "common/resource/shader_manager.h"

#include "common/lua/lua_function.h"
#include "common/lua/lua_manager.h"

#include "common/input/framework_listener.h"

#include <cfloat>
#include <cmath>

namespace egal
{
	static e_uint64 getLayerMask(EntityInstance& entity_instance)
	{
		Entity* entity = entity_instance.entity;
		
		if (!entity->isReady()) 
			return 1;

		e_uint64 layer_mask = 0;
		for (e_int32 i = 0; i < entity->getMeshCount(); ++i)
		{
			layer_mask |= entity->getMesh(i).material->getRenderLayerMask();
		}
		return layer_mask;
	}

	static e_uint32 ARGBToABGR(e_uint32 color)
	{
		return ((color & 0xff) << 16) | (color & 0xff00) | ((color & 0xff0000) >> 16) | (color & 0xff000000);
	}

#define COMPONENT_TYPE(type, name) \
	{ \
		type \
		, static_cast<ComponentManager::Serialize>(&SceneManager::serialize##name) \
		, static_cast<ComponentManager::Deserialize>(&SceneManager::deserialize##name) \
		, &SceneManager::create##name \
		, &SceneManager::destroy##name \
	}

	struct Component_infos 
	{
		ComponentType					type;
		ComponentManager::Serialize		serialize;
		ComponentManager::Deserialize	deserialize;
		ComponentHandle(SceneManager::*creator)(GameObject);
		e_void(SceneManager::*destroyer)(ComponentHandle);
	};
	
	static Component_infos COMPONENT_INFOS[] =
	{
		COMPONENT_TYPE(COMPONENT_ENTITY_INSTANCE_TYPE,		EntityInstance),
		COMPONENT_TYPE(COMPONENT_GLOBAL_LIGHT_TYPE,			GlobalLight),
		COMPONENT_TYPE(COMPONENT_POINT_LIGHT_TYPE,			PointLight),
		COMPONENT_TYPE(COMPONENT_DECAL_TYPE,				Decal),
		COMPONENT_TYPE(COMPONENT_CAMERA_TYPE,				Camera),
		COMPONENT_TYPE(COMPONENT_BONE_ATTACHMENT_TYPE,		BoneAttachment),
		COMPONENT_TYPE(COMPONENT_ENVIRONMENT_PROBE_TYPE,	EnvironmentProbe),
	};

#undef COMPONENT_TYPE
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	SceneManager::SceneManager(Renderer& renderer,
		EngineRoot& engine,
		ComponentManager& com_man,
		IAllocator& allocator)
		: m_engine(engine)
		, m_com_man(com_man)
		, m_renderer(renderer)
		, m_allocator(allocator)
		, m_entity_loaded_callbacks(allocator)
		, m_entity_instances(allocator)
		, m_cameras(allocator)
		, m_point_lights(allocator)
		, m_light_influenced_geometry(allocator)
		, m_global_lights(allocator)
		, m_decals(allocator)
		, m_debug_triangles(allocator)
		, m_debug_lines(allocator)
		, m_debug_points(allocator)
		, m_temporary_infos(allocator)
		, m_active_global_light_cmp(INVALID_COMPONENT)
		, m_is_grass_enabled(true)
		, m_is_game_running(false)
		, m_point_lights_map(allocator)
		, m_bone_attachments(allocator)
		, m_environment_probes(allocator)
		, m_lod_multiplier(1.0f)
		, m_time(0)
		, m_mouse_sensitivity(200, 200)
		, m_horizontalAngle(0)
		, m_verticalAngle(0)
		, m_is_updating_attachments(false)
	{
		m_com_man.m_game_object_destroyed.bind<SceneManager, &SceneManager::onEntityDestroyed>(this);
		m_com_man.m_game_object_moved.bind<SceneManager, &SceneManager::onEntityMoved>(this);

		m_culling_system = CullingSystem::create(m_allocator);
		m_entity_instances.reserve(5000);

		for (auto& i : COMPONENT_INFOS)
		{
			com_man.registerComponentType(i.type, this, i.serialize, i.deserialize);
		}
	}
		SceneManager::~SceneManager()
		{
			m_com_man.GameObjectTransformed().unbind<SceneManager, &SceneManager::onEntityMoved>(this);
			m_com_man.GameObjectDestroyed().unbind<SceneManager, &SceneManager::onEntityDestroyed>(this);

			CullingSystem::destroy(*m_culling_system);
		}


		e_void SceneManager::entityStateChanged(Resource::State old_state, Resource::State new_state, Resource& resource)
		{
			Entity* entity = static_cast<Entity*>(&resource);
			if (new_state == Resource::State::READY)
			{
				EntityLoaded(entity);
			}
			else if (old_state == Resource::State::READY && new_state != Resource::State::READY)
			{
				EntityUnloaded(entity);
			}
		}


		e_void SceneManager::clear()
		{
			auto& rm = m_engine.getResourceManager();
			auto* material_manager = static_cast<MaterialManager*>(rm.get(RESOURCE_MATERIAL_TYPE));

			m_entity_loaded_callbacks.clear();

			for (Decal& decal : m_decals)
			{
				if (decal.material) 
					material_manager->unload(*decal.material);
			}
			m_decals.clear();

			m_cameras.clear();

			for (auto& i : m_entity_instances)
			{
				if (i.entity && i.game_object != INVALID_GAME_OBJECT )
				{
					freeCustomMeshes(i, material_manager);
					i.entity->getResourceManager().unload(*i.entity);
					_delete(m_allocator, i.pose);
				}
			}
			m_entity_instances.clear();
			m_culling_system->clear();

			for (auto& probe : m_environment_probes)
			{
				if (probe.texture) 
					probe.texture->getResourceManager().unload(*probe.texture);
				
				if (probe.radiance) 
					probe.radiance->getResourceManager().unload(*probe.radiance);
				
				if (probe.irradiance) 
					probe.irradiance->getResourceManager().unload(*probe.irradiance);
			}
			m_environment_probes.clear();
		}

		ComponentManager& SceneManager::getComponentManager() { return m_com_man; }


		ComponentHandle SceneManager::getComponent(GameObject game_object, ComponentType type)
		{
			if (type == COMPONENT_ENTITY_INSTANCE_TYPE)
			{
				if (game_object.index >= m_entity_instances.size()) 
					return INVALID_COMPONENT;
				ComponentHandle cmp = {game_object.index};
				return m_entity_instances[game_object.index].game_object.isValid() ? cmp : INVALID_COMPONENT;
			}
			if (type == COMPONENT_ENVIRONMENT_PROBE_TYPE)
			{
				e_int32 index = m_environment_probes.find(game_object);
				if (index < 0) 
					return INVALID_COMPONENT;
				return {game_object.index};
			}
			if (type == COMPONENT_DECAL_TYPE)
			{
				e_int32 index = m_decals.find(game_object);
				if (index < 0) 
					return INVALID_COMPONENT;
				return {game_object.index};
			}
			if (type == COMPONENT_POINT_LIGHT_TYPE)
			{
				for (auto& i : m_point_lights)
				{
					if (i.m_game_object == game_object) 
						return {game_object.index};
				}
				return INVALID_COMPONENT;
			}
			if (type == COMPONENT_GLOBAL_LIGHT_TYPE)
			{
				auto iter = m_global_lights.find(game_object);
				if (iter.isValid()) 
					return {game_object.index};
				return INVALID_COMPONENT;
			}
			if (type == COMPONENT_CAMERA_TYPE)
			{
				auto iter = m_cameras.find(game_object);
				ComponentHandle cmp = {game_object.index};
				return iter.isValid() ? cmp : INVALID_COMPONENT;
			}
			//if (type == TERRAIN_TYPE)
			//{
			//	auto iter = m_terrains.find(game_object);
			//	if (!iter.isValid()) return INVALID_COMPONENT;
			//	return {game_object.index};
			//}
			//if (type == PARTICLE_EMITTER_TYPE)
			//{
			//	e_int32 index = m_particle_emitters.find(game_object);
			//	if (index < 0) return INVALID_COMPONENT;
			//	if (m_particle_emitters.at(index)->m_is_valid) return {game_object.index};
			//	return INVALID_COMPONENT;
			//}
			//if (type == SCRIPTED_PARTICLE_EMITTER_TYPE)
			//{
			//	e_int32 index = m_scripted_particle_emitters.find(game_object);
			//	if (index < 0) return INVALID_COMPONENT;
			//	return {game_object.index};
			//}
			if (type == COMPONENT_BONE_ATTACHMENT_TYPE)
			{
				for (auto& attachment : m_bone_attachments)
				{
					if (attachment.game_object == game_object)
					{
						return {game_object.index};
					}
				}
				return INVALID_COMPONENT;
			}
			return INVALID_COMPONENT;
		}

		IPlugin& SceneManager::getPlugin() const
		{ 
			return IPlugin();
		}

		Renderer& SceneManager::getRenderer()
		{
			return m_renderer;
		}

		e_void SceneManager::getRay(ComponentHandle camera_index,
			const float2& screen_pos,
			float3& origin,
			float3& dir)
		{
			Camera& camera = m_cameras[{camera_index.index}];
			origin = m_com_man.getPosition(camera.game_object);

			e_float width = camera.screen_width;
			e_float height = camera.screen_height;
			if (width <= 0 || height <= 0)
			{
				dir = m_com_man.getRotation(camera.game_object).rotate(float3(0, 0, 1));
				return;
			}

			e_float nx = 2 * (screen_pos.x / width) - 1;
			e_float ny = 2 * ((height - screen_pos.y) / height) - 1;

			float4x4 projection_matrix = getCameraProjection(camera_index);
			float4x4 view_matrix = m_com_man.getMatrix(camera.game_object);

			if (camera.is_ortho)
			{
				e_float ratio = camera.screen_height > 0 ? camera.screen_width / camera.screen_height : 1;
				origin += view_matrix.getXVector() * nx * camera.ortho_size * ratio
					+ view_matrix.getYVector() * ny * camera.ortho_size;
			}

			view_matrix.inverse();
			float4x4 inverted = (projection_matrix * view_matrix);
			inverted.inverse();

			float4 p0 = inverted * float4(nx, ny, -1, 1);
			float4 p1 = inverted * float4(nx, ny, 1, 1);
			p0 *= 1 / p0.w;
			p1 *= 1 / p1.w;
			dir = p1 - p0;
			dir.normalize();
		}


		Frustum SceneManager::getCameraFrustum(ComponentHandle cmp) const
		{
			const Camera& camera = m_cameras[{cmp.index}];
			float4x4 mtx = m_com_man.getMatrix(camera.game_object);
			Frustum ret;
			e_float ratio = camera.screen_height > 0 ? camera.screen_width / camera.screen_height : 1;
			if (camera.is_ortho)
			{
				ret.computeOrtho(mtx.getTranslation(),
					mtx.getZVector(),
					mtx.getYVector(),
					camera.ortho_size * ratio,
					camera.ortho_size,
					camera.near_flip,
					camera.far_flip);
				return ret;
			}
			ret.computePerspective(mtx.getTranslation(),
				-mtx.getZVector(),
				mtx.getYVector(),
				camera.fov,
				ratio,
				camera.near_flip,
				camera.far_flip);

			return ret;
		}


		Frustum SceneManager::getCameraFrustum(ComponentHandle cmp, const float2& viewport_min_px, const float2& viewport_max_px) const
		{
			const Camera& camera = m_cameras[{cmp.index}];
			float4x4 mtx = m_com_man.getMatrix(camera.game_object);
			Frustum ret;
			e_float ratio = camera.screen_height > 0 ? camera.screen_width / camera.screen_height : 1;
			float2 viewport_min = { viewport_min_px.x / camera.screen_width * 2 - 1, (1 - viewport_max_px.y / camera.screen_height) * 2 - 1 };
			float2 viewport_max = { viewport_max_px.x / camera.screen_width * 2 - 1, (1 - viewport_min_px.y / camera.screen_height) * 2 - 1 };
			if (camera.is_ortho)
			{
				ret.computeOrtho(mtx.getTranslation(),
					mtx.getZVector(),
					mtx.getYVector(),
					camera.ortho_size * ratio,
					camera.ortho_size,
					camera.near_flip,
					camera.far_flip,
					viewport_min,
					viewport_max);
				return ret;
			}
			ret.computePerspective(mtx.getTranslation(),
				-mtx.getZVector(),
				mtx.getYVector(),
				camera.fov,
				ratio,
				camera.near_flip,
				camera.far_flip,
				viewport_min,
				viewport_max);

			return ret;
		}


		egal::e_void SceneManager::camera_navigate(GameObject cmera_object, e_float forward, e_float right, e_float up, e_float speed)
		{
			ComponentHandle cmp = getComponent(cmera_object, COMPONENT_CAMERA_TYPE);

			const Camera& camera = m_cameras[{cmp.index}];

			float3 pos = m_com_man.getPosition(cmera_object);
			Quaternion rot = m_com_man.getRotation(cmera_object);

			right = camera.is_ortho ? 0 : right;

			pos += rot.rotate(float3(0, 0, -1)) * forward * speed;
			pos += rot.rotate(float3(1, 0, 0)) * right * speed;
			pos += rot.rotate(float3(0, 1, 0)) * up * speed;
			m_com_man.setPosition(cmera_object, pos);
		}

		egal::e_void SceneManager::camera_rotate(GameObject cmera_object, e_int32 x, e_int32 y)
		{
			if (!g_mouse_board[MB_Right])
			{
				m_mouse_last = float2(x, y);
				return;
			}

			m_mouse_now = float2(x, y);
			m_horizontalAngle = m_mouse_now.x - m_mouse_last.x;
			m_verticalAngle = m_mouse_now.y - m_mouse_last.y;
			m_mouse_last = float2(x, y);

			ComponentHandle cmp = getComponent(cmera_object, COMPONENT_CAMERA_TYPE);

			const Camera& camera = m_cameras[{cmp.index}];

			float3 pos = m_com_man.getPosition(cmera_object);
			Quaternion rot = m_com_man.getRotation(cmera_object);
			Quaternion old_rot = rot;

			Quaternion yaw_rot(float3(0, 1, 0), egal::Math::C_DegreeToRadian * (m_horizontalAngle/4));
			rot = yaw_rot * rot;
			rot.normalize();

			float3 pitch_axis = rot.rotate(float3(1, 0, 0));
			Quaternion pitch_rot(pitch_axis, egal::Math::C_DegreeToRadian * (m_verticalAngle/4));
			rot = pitch_rot * rot;
			rot.normalize();

			m_com_man.setRotation(cmera_object, rot);
			m_com_man.setPosition(cmera_object, pos);
		}



		e_void SceneManager::updateBoneAttachment(const BoneAttachment& bone_attachment)
		{
			if (!bone_attachment.parent_game_object.isValid())
				return;
			ComponentHandle entity_instance = getEntityInstanceComponent(bone_attachment.parent_game_object);
			if (entity_instance == INVALID_COMPONENT) 
				return;
			const Pose* parent_pose = lockPose(entity_instance);
			if (!parent_pose) 
				return;

			Transform parent_entity_transform = m_com_man.getTransform(bone_attachment.parent_game_object);
			e_int32 idx = bone_attachment.bone_index;
			if (idx < 0 || idx > parent_pose->count)
			{
				unlockPose(entity_instance, false);
				return;
			}
			e_float original_scale = m_com_man.getScale(bone_attachment.game_object);
			Transform bone_transform = {parent_pose->positions[idx], parent_pose->rotations[idx], 1.0f};
			Transform relative_transform = { bone_attachment.relative_transform.pos, bone_attachment.relative_transform.rot, 1.0f};
			Transform result = parent_entity_transform * bone_transform * relative_transform;
			result.scale = original_scale;
			m_com_man.setTransform(bone_attachment.game_object, result);
			unlockPose(entity_instance, false);
		}


		GameObject SceneManager::getBoneAttachmentParent(ComponentHandle cmp)
		{
			return m_bone_attachments[{cmp.index}].parent_game_object;
		}


		e_void SceneManager::updateRelativeMatrix(BoneAttachment& attachment)
		{
			if (attachment.parent_game_object == INVALID_GAME_OBJECT) 
				return;
			if (attachment.bone_index < 0) 
				return;
			ComponentHandle entity_instance = getEntityInstanceComponent(attachment.parent_game_object);
			if (entity_instance == INVALID_COMPONENT)
				return;
			const Pose* pose = lockPose(entity_instance);
			if (!pose) 
				return;
			ASSERT(pose->is_absolute);
			if (attachment.bone_index >= pose->count)
			{
				unlockPose(entity_instance, false);
				return;
			}
			Transform bone_transform = {pose->positions[attachment.bone_index], pose->rotations[attachment.bone_index], 1.0f};

			Transform inv_parent_transform = m_com_man.getTransform(attachment.parent_game_object) * bone_transform;
			inv_parent_transform = inv_parent_transform.inverted();
			Transform child_transform = m_com_man.getTransform(attachment.parent_game_object);
			Transform res = inv_parent_transform * child_transform;
			attachment.relative_transform = {res.pos, res.rot};
			unlockPose(entity_instance, false);
		}


		float3 SceneManager::getBoneAttachmentPosition(ComponentHandle cmp)
		{
			return m_bone_attachments[{cmp.index}].relative_transform.pos;
		}


		e_void SceneManager::setBoneAttachmentPosition(ComponentHandle cmp, const float3& pos)
		{
			BoneAttachment& attachment = m_bone_attachments[{cmp.index}];
			attachment.relative_transform.pos = pos;
			m_is_updating_attachments = true;
			updateBoneAttachment(attachment);
			m_is_updating_attachments = false;
		}


		float3 SceneManager::getBoneAttachmentRotation(ComponentHandle cmp)
		{
			return m_bone_attachments[{cmp.index}].relative_transform.rot.toEuler();
		}


		e_void SceneManager::setBoneAttachmentRotation(ComponentHandle cmp, const float3& rot)
		{
			BoneAttachment& attachment = m_bone_attachments[{cmp.index}];
			float3 euler = rot;
			euler.x = Math::clamp(euler.x, -Math::C_Pi * 0.5f, Math::C_Pi * 0.5f);
			attachment.relative_transform.rot.fromEuler(euler);
			m_is_updating_attachments = true;
			updateBoneAttachment(attachment);
			m_is_updating_attachments = false;
		}


		e_void SceneManager::setBoneAttachmentRotationQuat(ComponentHandle cmp, const Quaternion& rot)
		{
			BoneAttachment& attachment = m_bone_attachments[{cmp.index}];
			attachment.relative_transform.rot = rot;
			m_is_updating_attachments = true;
			updateBoneAttachment(attachment);
			m_is_updating_attachments = false;
		}


		e_int32 SceneManager::getBoneAttachmentBone(ComponentHandle cmp)
		{
			return m_bone_attachments[{cmp.index}].bone_index;
		}


		e_void SceneManager::setBoneAttachmentBone(ComponentHandle cmp, e_int32 value)
		{
			BoneAttachment& ba = m_bone_attachments[{cmp.index}];
			ba.bone_index = value;
			updateRelativeMatrix(ba);
		}


		e_void SceneManager::setBoneAttachmentParent(ComponentHandle cmp, GameObject entity)
		{
			BoneAttachment& ba = m_bone_attachments[{cmp.index}];
			ba.parent_game_object = entity;
			if (entity.isValid() && entity.index < m_entity_instances.size())
			{
				EntityInstance& mi = m_entity_instances[entity.index];
				mi.flags = mi.flags | EntityInstance::IS_BONE_ATTACHMENT_PARENT;
			}
			updateRelativeMatrix(ba);
		}

		e_void SceneManager::startGame()
		{
			m_is_game_running = true;
		}


		e_void SceneManager::stopGame()
		{
			m_is_game_running = false;
		}


		e_void SceneManager::frame(e_float dt, e_bool paused)
		{
			PROFILE_FUNCTION();

			m_time += dt;
			for (e_int32 i = m_debug_triangles.size() - 1; i >= 0; --i)
			{
				e_float life = m_debug_triangles[i].life;
				if (life < 0)
				{
					m_debug_triangles.eraseFast(i);
				}
				else
				{
					life -= dt;
					m_debug_triangles[i].life = life;
				}
			}

			for(e_int32 i = m_debug_lines.size() - 1; i >= 0; --i)
			{
				e_float life = m_debug_lines[i].life;
				if(life < 0)
				{
					m_debug_lines.eraseFast(i);
				}
				else
				{
					life -= dt;
					m_debug_lines[i].life = life;
				}
			}


			for (e_int32 i = m_debug_points.size() - 1; i >= 0; --i)
			{
				e_float life = m_debug_points[i].life;
				if (life < 0)
				{
					m_debug_points.eraseFast(i);
				}
				else
				{
					life -= dt;
					m_debug_points[i].life = life;
				}
			}

			if (m_is_game_running && !paused)
			{
				//for (auto* emitter : m_particle_emitters)
				//{
				//	if (emitter->m_is_valid) emitter->update(dt);
				//}
				//for (auto* emitter : m_scripted_particle_emitters)
				//{
				//	emitter->update(dt);
				//}
			}
		}


		egal::e_void SceneManager::lateUpdate(e_float time_delta, e_bool paused)
		{

		}

		e_void SceneManager::serializeEntityInstance(ISerializer& serialize, ComponentHandle cmp)
		{
			EntityInstance& r = m_entity_instances[cmp.index];
			ASSERT(r.game_object != INVALID_GAME_OBJECT);

			serialize.write("source", r.entity ? r.entity->getPath().c_str() : "");
			serialize.write("flags", e_uint8(r.flags & EntityInstance::PERSISTENT_FLAGS));
			e_bool has_changed_materials = r.entity && r.entity->isReady() && r.meshes != &r.entity->getMesh(0);
			serialize.write("custom_materials", has_changed_materials ? r.mesh_count : 0);
			if (has_changed_materials)
			{
				for (e_int32 i = 0; i < r.mesh_count; ++i)
				{
					serialize.write("", r.meshes[i].material->getPath().c_str());
				}
			}
		}


		static e_bool keepSkin(EntityInstance& r)
		{
			return (r.flags & (e_uint8)EntityInstance::KEEP_SKIN) != 0;
		}


		static e_bool hasCustomMeshes(EntityInstance& r)
		{
			return (r.flags & (e_uint8)EntityInstance::CUSTOM_MESHES) != 0;
		}


		e_void SceneManager::deserializeEntityInstance(IDeserializer& serializer, GameObject _game_object, e_int32 scene_version)
		{
			while (_game_object.index >= m_entity_instances.size())
			{
				auto& r = m_entity_instances.emplace();
				r.game_object = INVALID_GAME_OBJECT;
				r.pose = nullptr;
				r.entity = nullptr;
				r.meshes = nullptr;
				r.mesh_count = 0;
			}
			auto& r = m_entity_instances[_game_object.index];
			r.game_object = _game_object;
			r.entity = nullptr;
			r.pose = nullptr;
			r.flags = 0;
			r.meshes = nullptr;
			r.mesh_count = 0;

			r.matrix = m_com_man.getMatrix(r.game_object);

			e_char path[MAX_PATH_LENGTH];
			serializer.read(path, TlengthOf(path));
			if (scene_version > (e_int32)RenderSceneVersion::MODEL_INSTNACE_FLAGS)
			{
				serializer.read(&r.flags);
				r.flags &= EntityInstance::PERSISTENT_FLAGS;
			}

			ComponentHandle cmp = {r.game_object.index};
			if (path[0] != 0)
			{
				auto* entity = static_cast<Entity*>(m_engine.getResourceManager().get( RESOURCE_ENTITY_TYPE)->load(ArchivePath(path)));
				setEntity(cmp, entity);
			}

			e_int32 material_count;
			serializer.read(&material_count);
			if (material_count > 0)
			{
				allocateCustomMeshes(r, material_count);
				for (e_int32 j = 0; j < material_count; ++j)
				{
					e_char path[MAX_PATH_LENGTH];
					serializer.read(path, TlengthOf(path));
					setEntityInstanceMaterial(cmp, j, ArchivePath(path));
				}
			}

			m_com_man.addComponent(r.game_object, COMPONENT_ENTITY_INSTANCE_TYPE, this, cmp);
		}


		e_void SceneManager::serializeGlobalLight(ISerializer& serializer, ComponentHandle cmp)
		{
			GlobalLight& light = m_global_lights[{cmp.index}];
			serializer.write("cascades",			light.m_cascades);
			serializer.write("diffuse_color",		light.m_diffuse_color);
			serializer.write("diffuse_intensity",	light.m_diffuse_intensity);
			serializer.write("indirect_intensity",	light.m_indirect_intensity);
			serializer.write("fog_bottom",			light.m_fog_bottom);
			serializer.write("fog_color",			light.m_fog_color);
			serializer.write("fog_density",			light.m_fog_density);
			serializer.write("fog_height",			light.m_fog_height);
		}


		e_void SceneManager::deserializeGlobalLight(IDeserializer& serializer, GameObject _game_object, e_int32 scene_version)
		{
			GlobalLight light;
			light.m_game_object = _game_object;
			serializer.read(&light.m_cascades);
			if (scene_version <= (e_int32)RenderSceneVersion::GLOBAL_LIGHT_REFACTOR)
			{
				ComponentHandle dummy;
				serializer.read(&dummy);
			}
			serializer.read(&light.m_diffuse_color);
			serializer.read(&light.m_diffuse_intensity);
			if (scene_version > (e_int32)RenderSceneVersion::INDIRECT_INTENSITY)
			{
				serializer.read(&light.m_indirect_intensity);
			}
			else
			{
				light.m_indirect_intensity = 1;
			}
			serializer.read(&light.m_fog_bottom);
			serializer.read(&light.m_fog_color);
			serializer.read(&light.m_fog_density);
			serializer.read(&light.m_fog_height);
			m_global_lights.insert(_game_object, light);
			ComponentHandle cmp = { _game_object.index};
			m_com_man.addComponent(light.m_game_object, COMPONENT_GLOBAL_LIGHT_TYPE, this, cmp);
			m_active_global_light_cmp = cmp;
		}
	
	
		e_void SceneManager::serializePointLight(ISerializer& serializer, ComponentHandle cmp)
		{
			PointLight& light = m_point_lights[m_point_lights_map[cmp]];
			serializer.write("attenuation",			light.m_attenuation_param);
			serializer.write("cast_shadow",			light.m_cast_shadows);
			serializer.write("diffuse_color",		light.m_diffuse_color);
			serializer.write("diffuse_intensity",	light.m_diffuse_intensity);
			serializer.write("fov",					light.m_fov);
			serializer.write("range",				light.m_range);
			serializer.write("specular_color",		light.m_specular_color);
			serializer.write("specular_intensity",	light.m_specular_intensity);
		}


		e_void SceneManager::deserializePointLight(IDeserializer& serializer, GameObject entity, e_int32 scene_version)
		{
			m_light_influenced_geometry.emplace(m_allocator);
			PointLight& light = m_point_lights.emplace();
			light.m_game_object = entity;
			serializer.read(&light.m_attenuation_param);
			serializer.read(&light.m_cast_shadows);
		
			if (scene_version <= (e_int32)RenderSceneVersion::POINT_LIGHT_NO_COMPONENT)
			{
				ComponentHandle dummy;
				serializer.read(&dummy);
			}
			serializer.read(&light.m_diffuse_color);
			serializer.read(&light.m_diffuse_intensity);
			serializer.read(&light.m_fov);
			serializer.read(&light.m_range);
			serializer.read(&light.m_specular_color);
			serializer.read(&light.m_specular_intensity);
			ComponentHandle cmp = { light.m_game_object.index };
			m_point_lights_map.insert(cmp, m_point_lights.size() - 1);

			m_com_man.addComponent(light.m_game_object, COMPONENT_POINT_LIGHT_TYPE, this, cmp);
		}


		e_void SceneManager::serializeDecal(ISerializer& serializer, ComponentHandle cmp)
		{
			const Decal& decal = m_decals[{cmp.index}];
			serializer.write("scale",		decal.scale);
			serializer.write("material",	decal.material ? decal.material->getPath().c_str() : "");
		}


		e_void SceneManager::deserializeDecal(IDeserializer& serializer, GameObject entity, e_int32 /*scene_version*/)
		{
			ResourceManagerBase* material_manager = m_engine.getResourceManager().get(RESOURCE_MATERIAL_TYPE);
			Decal& decal = m_decals.insert(entity);
			e_char tmp[MAX_PATH_LENGTH];
			decal.game_object = entity;
			serializer.read(&decal.scale);
			serializer.read(tmp, TlengthOf(tmp));
			decal.material = tmp[0] == '\0' ? nullptr : static_cast<Material*>(material_manager->load(ArchivePath(tmp)));
			updateDecalInfo(decal);
			m_com_man.addComponent(decal.game_object, COMPONENT_DECAL_TYPE, this, {decal.game_object.index});
		}


		e_void SceneManager::serializeCamera(ISerializer& serialize, ComponentHandle cmp)
		{
			Camera& camera = m_cameras[{cmp.index}];
			serialize.write("far",			camera.far_flip);
			serialize.write("fov",			camera.fov);
			serialize.write("is_ortho",		camera.is_ortho);
			serialize.write("ortho_size",	camera.ortho_size);
			serialize.write("near",			camera.near_flip);
			serialize.write("slot",			camera.slot);
		}


		e_void SceneManager::deserializeCamera(IDeserializer& serializer, GameObject game_object, e_int32 /*scene_version*/)
		{
			Camera camera;
			camera.game_object = game_object;
			serializer.read(&camera.far_flip);
			serializer.read(&camera.fov);
			serializer.read(&camera.is_ortho);
			serializer.read(&camera.ortho_size);
			serializer.read(&camera.near_flip);
			serializer.read(camera.slot, TlengthOf(camera.slot));
			m_cameras.insert(camera.game_object, camera);
			m_com_man.addComponent(camera.game_object, COMPONENT_CAMERA_TYPE, this, {camera.game_object.index});
		}


		e_void SceneManager::serializeBoneAttachment(ISerializer& serializer, ComponentHandle cmp)
		{
			BoneAttachment& attachment = m_bone_attachments[{cmp.index}];
			serializer.write("bone_index",			attachment.bone_index);
			serializer.write("parent",				attachment.parent_game_object);
			serializer.write("relative_transform",	attachment.relative_transform);
		}


		e_void SceneManager::deserializeBoneAttachment(IDeserializer& serializer, GameObject game_object, e_int32 /*scene_version*/)
		{
			BoneAttachment& bone_attachment = m_bone_attachments.emplace(game_object);
			bone_attachment.game_object = game_object;
			serializer.read(&bone_attachment.bone_index);
			serializer.read(&bone_attachment.parent_game_object);
			serializer.read(&bone_attachment.relative_transform);
			ComponentHandle cmp = {bone_attachment.game_object.index};
			m_com_man.addComponent(bone_attachment.game_object, COMPONENT_BONE_ATTACHMENT_TYPE, this, cmp);
			GameObject parent_game_object = bone_attachment.parent_game_object;
			if (parent_game_object.isValid() && parent_game_object.index < m_entity_instances.size())
			{
				EntityInstance& mi = m_entity_instances[parent_game_object.index];
				mi.flags = mi.flags | EntityInstance::IS_BONE_ATTACHMENT_PARENT;
			}
		}


		e_void SceneManager::serializeTerrain(ISerializer& serializer, ComponentHandle cmp)
		{
			//Terrain* terrain = m_terrains[{cmp.index}];
			//serializer.write("layer_mask", terrain->m_layer_mask);
			//serializer.write("scale", terrain->m_scale);
			//serializer.write("material", terrain->m_material ? terrain->m_material->getPath().c_str() : "");
			//serializer.write("grass_count", terrain->m_grass_types.size());
			//for (Terrain::GrassType& type : terrain->m_grass_types)
			//{
			//	serializer.write("density", type.m_density);
			//	serializer.write("distance", type.m_distance);
			//	serializer.write("rotation_mode", (e_int32)type.m_rotation_mode);
			//	serializer.write("entity", type.m_grass_entity ? type.m_grass_entity->getPath().c_str() : "");
			//}
		}

		e_void SceneManager::deserializeTerrain(IDeserializer& serializer, GameObject entity, e_int32 version)
		{
			//Terrain* terrain = _aligned_new(m_allocator, Terrain)(m_renderer, entity, *this, m_allocator);
			//m_terrains.insert(entity, terrain);
			//terrain->m_entity = entity;
			//serializer.read(&terrain->m_layer_mask);
			//serializer.read(&terrain->m_scale);
			//e_char tmp[MAX_PATH_LENGTH];
			//serializer.read(tmp, TlengthOf(tmp));
			//auto* material = tmp[0] ? m_engine.getResourceManager().get(MATERIAL_TYPE)->load(Path(tmp)) : nullptr;
			//terrain->setMaterial((Material*)material);

			//e_int32 count;
			//serializer.read(&count);
			//for(e_int32 i = 0; i < count; ++i)
			//{
			//	Terrain::GrassType type(*terrain);
			//	serializer.read(&type.m_density);
			//	serializer.read(&type.m_distance);
			//	if (version >= (e_int32)RenderSceneVersion::GRASS_ROTATION_MODE)
			//	{
			//		serializer.read((e_int32*)&type.m_rotation_mode);
			//	}
			//	type.m_idx = i;
			//	serializer.read(tmp, TlengthOf(tmp));
			//	terrain->m_grass_types.push_back(type);
			//	terrain->setGrassTypePath(terrain->m_grass_types.size() - 1, Path(tmp));
			//}

			//m_com_man.addComponent(entity, TERRAIN_TYPE, this, { entity.index });
		}

		e_void SceneManager::serializeEnvironmentProbe(ISerializer& serializer, ComponentHandle cmp)
		{
			serializer.write("guid", m_environment_probes[{cmp.index}].guid);
		}


		e_int32 SceneManager::getVersion() const { return (e_int32)RenderSceneVersion::LATEST; }


		e_void SceneManager::deserializeEnvironmentProbe(IDeserializer& serializer, GameObject entity, e_int32 /*scene_version*/)
		{
			auto* texture_manager = m_engine.getResourceManager().get( RESOURCE_TEXTURE_TYPE);
			StaticString<MAX_PATH_LENGTH> probe_dir("com_mans/", m_com_man.getName(), "/probes/");
			EnvironmentProbe& probe = m_environment_probes.insert(entity);
			serializer.read(&probe.guid);
			StaticString<MAX_PATH_LENGTH> path_str(probe_dir, probe.guid, ".dds");
			probe.texture = static_cast<Texture*>(texture_manager->load(ArchivePath(path_str)));
			probe.texture->setFlag(BGFX_TEXTURE_SRGB, true);
			StaticString<MAX_PATH_LENGTH> irr_path_str(probe_dir, probe.guid, "_irradiance.dds");
			probe.irradiance = static_cast<Texture*>(texture_manager->load(ArchivePath(irr_path_str)));
			probe.irradiance->setFlag(BGFX_TEXTURE_SRGB, true);
			probe.irradiance->setFlag(BGFX_TEXTURE_MIN_ANISOTROPIC, true);
			probe.irradiance->setFlag(BGFX_TEXTURE_MAG_ANISOTROPIC, true);
			StaticString<MAX_PATH_LENGTH> r_path_str(probe_dir, probe.guid, "_radiance.dds");
			probe.radiance = static_cast<Texture*>(texture_manager->load(ArchivePath(r_path_str)));
			probe.radiance->setFlag(BGFX_TEXTURE_SRGB, true);
			probe.radiance->setFlag(BGFX_TEXTURE_MIN_ANISOTROPIC, true);
			probe.radiance->setFlag(BGFX_TEXTURE_MAG_ANISOTROPIC, true);

			m_com_man.addComponent(entity, COMPONENT_ENVIRONMENT_PROBE_TYPE, this, {entity.index});
		}


		//e_void serializeScriptedParticleEmitter(ISerializer& serializer, ComponentHandle cmp)
		//{
		//	ScriptedParticleEmitter* emitter = m_scripted_particle_emitters[{cmp.index}];
		//	const Material* material = emitter->getMaterial();
		//	serializer.write("material", material ? material->getPath().c_str() : "");
		//}


		//e_void deserializeScriptedParticleEmitter(IDeserializer& serializer, GameObject entity, e_int32 scene_version)
		//{
		//	ScriptedParticleEmitter* emitter = _aligned_new(m_allocator, ScriptedParticleEmitter)(entity, m_allocator);
		//	emitter->m_entity = entity;

		//	e_char tmp[MAX_PATH_LENGTH];
		//	serializer.read(tmp, TlengthOf(tmp));
		//	ResourceManagerBase* material_manager = m_engine.getResourceManager().get(MATERIAL_TYPE);
		//	Material* material = (Material*)material_manager->load(Path(tmp));
		//	emitter->setMaterial(material);

		//	m_scripted_particle_emitters.insert(entity, emitter);
		//	m_com_man.addComponent(entity, SCRIPTED_PARTICLE_EMITTER_TYPE, this, {entity.index});
		//}

		e_void SceneManager::serializeBoneAttachments(WriteBinary& serializer)
		{
			serializer.write((e_int32)m_bone_attachments.size());
			for (auto& attachment : m_bone_attachments)
			{
				serializer.write(attachment.bone_index);
				serializer.write(attachment.game_object);
				serializer.write(attachment.parent_game_object);
				serializer.write(attachment.relative_transform);
			}
		}

		e_void SceneManager::serializeCameras(WriteBinary& serializer)
		{
			serializer.write((e_int32)m_cameras.size());
			for (auto& camera : m_cameras)
			{
				serializer.write(camera.game_object);
				serializer.write(camera.far_flip);
				serializer.write(camera.fov);
				serializer.write(camera.is_ortho);
				serializer.write(camera.ortho_size);
				serializer.write(camera.near_flip);
				serializer.writeString(camera.slot);
			}
		}

		e_void SceneManager::serializeLights(WriteBinary& serializer)
		{
			serializer.write((e_int32)m_point_lights.size());
			for (e_int32 i = 0, c = m_point_lights.size(); i < c; ++i)
			{
				serializer.write(m_point_lights[i]);
			}

			serializer.write((e_int32)m_global_lights.size());
			for (const GlobalLight& light : m_global_lights)
			{
				serializer.write(light);
			}
			serializer.write(m_active_global_light_cmp);
		}

		e_void SceneManager::serializeEntityInstances(WriteBinary& serializer)
		{
			serializer.write((e_int32)m_entity_instances.size());
			for (auto& r : m_entity_instances)
			{
				serializer.write(r.entity);
				serializer.write(e_uint8(r.flags & EntityInstance::PERSISTENT_FLAGS));
				if(r.game_object != INVALID_GAME_OBJECT)
				{
					serializer.write(r.entity ? r.entity->getPath().getHash() : 0);
					e_bool has_changed_materials = r.entity && r.entity->isReady() && r.meshes != &r.entity->getMesh(0);
					serializer.write(has_changed_materials ? r.mesh_count : 0);
					if (has_changed_materials)
					{
						for (e_int32 i = 0; i < r.mesh_count; ++i)
						{
							serializer.writeString(r.meshes[i].material->getPath().c_str());
						}
					}
				}
			
			}
		}

		e_void SceneManager::serializeTerrains(WriteBinary& serializer)
		{
			//serializer.write((e_int32)m_terrains.size());
			//for (auto* terrain : m_terrains)
			//{
			//	terrain->serialize(serializer);
			//}
		}


		e_void SceneManager::deserializeDecals(ReadBinary& serializer)
		{
			ResourceManagerBase* material_manager = m_engine.getResourceManager().get(RESOURCE_MATERIAL_TYPE);
			e_int32 count;
			serializer.read(count);
			m_decals.reserve(count);
			for (e_int32 i = 0; i < count; ++i)
			{
				e_char tmp[MAX_PATH_LENGTH];
				Decal decal;
				serializer.read(decal.game_object);
				serializer.read(decal.scale);
				serializer.readString(tmp, TlengthOf(tmp));
				decal.material = tmp[0] == '\0' ? nullptr : static_cast<Material*>(material_manager->load(ArchivePath(tmp)));
				updateDecalInfo(decal);
				m_decals.insert(decal.game_object, decal);
				m_com_man.addComponent(decal.game_object, COMPONENT_DECAL_TYPE, this, {decal.game_object.index});
			}
		}


		e_void SceneManager::serializeDecals(WriteBinary& serializer)
		{
			serializer.write(m_decals.size());
			for (auto& decal : m_decals)
			{
				serializer.write(decal.game_object);
				serializer.write(decal.scale);
				serializer.writeString(decal.material ? decal.material->getPath().c_str() : "");
			}
		}


		e_void SceneManager::serializeEnvironmentProbes(WriteBinary& serializer)
		{
			e_int32 count = m_environment_probes.size();
			serializer.write(count);
			for (e_int32 i = 0; i < count; ++i)
			{
				GameObject entity = m_environment_probes.getKey(i);
				serializer.write(entity);
				serializer.write(m_environment_probes.at(i).guid);
			}
		}


		e_void SceneManager::deserializeEnvironmentProbes(ReadBinary& serializer)
		{
			e_int32 count;
			serializer.read(count);
			m_environment_probes.reserve(count);
			auto* texture_manager = m_engine.getResourceManager().get( RESOURCE_TEXTURE_TYPE);
			StaticString<MAX_PATH_LENGTH> probe_dir("com_mans/", m_com_man.getName(), "/probes/");
			for (e_int32 i = 0; i < count; ++i)
			{
				GameObject entity;
				serializer.read(entity);
				EnvironmentProbe& probe = m_environment_probes.insert(entity);
				serializer.read(probe.guid);
				StaticString<MAX_PATH_LENGTH> path_str(probe_dir, probe.guid, ".dds");
				probe.texture = static_cast<Texture*>(texture_manager->load(ArchivePath(path_str)));
				probe.texture->setFlag(BGFX_TEXTURE_SRGB, true);
				StaticString<MAX_PATH_LENGTH> irr_path_str(probe_dir, probe.guid, "_irradiance.dds");
				probe.irradiance = static_cast<Texture*>(texture_manager->load(ArchivePath(irr_path_str)));
				probe.irradiance->setFlag(BGFX_TEXTURE_SRGB, true);
				probe.irradiance->setFlag(BGFX_TEXTURE_MIN_ANISOTROPIC, true);
				probe.irradiance->setFlag(BGFX_TEXTURE_MAG_ANISOTROPIC, true);
				StaticString<MAX_PATH_LENGTH> r_path_str(probe_dir, probe.guid, "_radiance.dds");
				probe.radiance = static_cast<Texture*>(texture_manager->load(ArchivePath(r_path_str)));
				probe.radiance->setFlag(BGFX_TEXTURE_SRGB, true);
				probe.radiance->setFlag(BGFX_TEXTURE_MIN_ANISOTROPIC, true);
				probe.radiance->setFlag(BGFX_TEXTURE_MAG_ANISOTROPIC, true);

				ComponentHandle cmp = {entity.index};
				m_com_man.addComponent(entity, COMPONENT_ENVIRONMENT_PROBE_TYPE, this, cmp);
			}
		}


		e_void SceneManager::deserializeBoneAttachments(ReadBinary& serializer)
		{
			e_int32 count;
			serializer.read(count);
			m_bone_attachments.clear();
			m_bone_attachments.reserve(count);
			for (e_int32 i = 0; i < count; ++i)
			{
				BoneAttachment bone_attachment;
				serializer.read(bone_attachment.bone_index);
				serializer.read(bone_attachment.game_object);
				serializer.read(bone_attachment.parent_game_object);
				serializer.read(bone_attachment.relative_transform);
				m_bone_attachments.insert(bone_attachment.game_object, bone_attachment);
				ComponentHandle cmp = {bone_attachment.game_object.index};
				m_com_man.addComponent(bone_attachment.game_object, COMPONENT_BONE_ATTACHMENT_TYPE, this, cmp);
			}
		}


		e_void SceneManager::serialize(WriteBinary& serializer)
		{
			serializeCameras(serializer);
			serializeEntityInstances(serializer);
			serializeLights(serializer);
			serializeTerrains(serializer);
			serializeBoneAttachments(serializer);
			serializeEnvironmentProbes(serializer);
			serializeDecals(serializer);
		}


		e_void SceneManager::deserializeCameras(ReadBinary& serializer)
		{
			e_int32 size;
			serializer.read(size);
			m_cameras.rehash(size);
			for (e_int32 i = 0; i < size; ++i)
			{
				Camera camera;
				serializer.read(camera.game_object);
				serializer.read(camera.far_flip);
				serializer.read(camera.fov);
				serializer.read(camera.is_ortho);
				serializer.read(camera.ortho_size);
				serializer.read(camera.near_flip);
				serializer.readString(camera.slot, TlengthOf(camera.slot));

				m_cameras.insert(camera.game_object, camera);
				m_com_man.addComponent(camera.game_object, COMPONENT_CAMERA_TYPE, this, {camera.game_object.index});
			}
		}

		e_void SceneManager::deserializeEntityInstances(ReadBinary& serializer)
		{
			e_int32 size = 0;
			serializer.read(size);
			m_entity_instances.reserve(size);
			for (e_int32 i = 0; i < size; ++i)
			{
				auto& r = m_entity_instances.emplace();
				serializer.read(r.game_object);
				serializer.read(r.flags);
				r.flags &= EntityInstance::PERSISTENT_FLAGS;
				ASSERT(r.game_object.index == i || !r.game_object.isValid());
				r.entity = nullptr;
				r.pose = nullptr;
				r.meshes = nullptr;
				r.mesh_count = 0;

				if(r.game_object != INVALID_GAME_OBJECT)
				{
					r.matrix = m_com_man.getMatrix(r.game_object);

					e_uint32 path;
					serializer.read(path);

					ComponentHandle cmp = { r.game_object.index };
					if (path != 0)
					{
						auto* entity = static_cast<Entity*>(m_engine.getResourceManager().get( RESOURCE_ENTITY_TYPE)->load(ArchivePath(path)));
						setEntity(cmp, entity);
					}

					e_int32 material_count;
					serializer.read(material_count);
					if (material_count > 0)
					{
						allocateCustomMeshes(r, material_count);
						for (e_int32 j = 0; j < material_count; ++j)
						{
							e_char path[MAX_PATH_LENGTH];
							serializer.readString(path, TlengthOf(path));
							setEntityInstanceMaterial(cmp, j, ArchivePath(path));
						}
					}

					m_com_man.addComponent(r.game_object, COMPONENT_ENTITY_INSTANCE_TYPE, this, cmp);
				}
			}
		}

		e_void SceneManager::deserializeLights(ReadBinary& serializer)
		{
			e_int32 size = 0;
			serializer.read(size);
			m_point_lights.resize(size);
			for (e_int32 i = 0; i < size; ++i)
			{
				m_light_influenced_geometry.emplace(m_allocator);
				PointLight& light = m_point_lights[i];
				serializer.read(light);
				ComponentHandle cmp = { light.m_game_object.index };
				m_point_lights_map.insert(cmp, i);

				m_com_man.addComponent(light.m_game_object, COMPONENT_POINT_LIGHT_TYPE, this, cmp);
			}

			serializer.read(size);
			for (e_int32 i = 0; i < size; ++i)
			{
				GlobalLight light;
				serializer.read(light);
				m_global_lights.insert(light.m_game_object, light);
				m_com_man.addComponent(light.m_game_object, COMPONENT_GLOBAL_LIGHT_TYPE, this, {light.m_game_object.index});
			}
			serializer.read(m_active_global_light_cmp);
		}

		e_void SceneManager::deserializeTerrains(ReadBinary& serializer)
		{
			//e_int32 size = 0;
			//serializer.read(size);
			//for (e_int32 i = 0; i < size; ++i)
			//{
			//	auto* terrain = _aligned_new(m_allocator, Terrain)(m_renderer, INVALID_GAME_OBJECT, *this, m_allocator);
			//	terrain->deserialize(serializer, m_com_man, *this);
			//	m_terrains.insert(terrain->getEntity(), terrain);
			//}
		}


		e_void SceneManager::deserialize(ReadBinary& serializer)
		{
			deserializeCameras(serializer);
			deserializeEntityInstances(serializer);
			deserializeLights(serializer);
			deserializeTerrains(serializer);
			//deserializeParticleEmitters(serializer);
			deserializeBoneAttachments(serializer);
			deserializeEnvironmentProbes(serializer);
			deserializeDecals(serializer);
		}


		e_void SceneManager::destroyBoneAttachment(ComponentHandle component)
		{
			GameObject entity = {component.index};
			const BoneAttachment& bone_attachment = m_bone_attachments[entity];
			GameObject parent_entity = bone_attachment.parent_game_object;
			if (parent_entity.isValid() && parent_entity.index < m_entity_instances.size())
			{
				EntityInstance& mi = m_entity_instances[bone_attachment.parent_game_object.index];
				mi.flags = mi.flags & ~EntityInstance::IS_BONE_ATTACHMENT_PARENT;
			}
			m_bone_attachments.erase(entity);
			m_com_man.destroyComponent(entity, COMPONENT_BONE_ATTACHMENT_TYPE, this, component);
		}


		e_void SceneManager::destroyEnvironmentProbe(ComponentHandle component)
		{
			GameObject entity = {component.index};
			auto& probe = m_environment_probes[entity];
			if (probe.texture) probe.texture->getResourceManager().unload(*probe.texture);
			if (probe.irradiance) probe.irradiance->getResourceManager().unload(*probe.irradiance);
			if (probe.radiance) probe.radiance->getResourceManager().unload(*probe.radiance);
			m_environment_probes.erase(entity);
			m_com_man.destroyComponent(entity, COMPONENT_ENVIRONMENT_PROBE_TYPE, this, component);
		}


		e_void SceneManager::destroyEntityInstance(ComponentHandle component)
		{
			for (e_int32 i = 0; i < m_light_influenced_geometry.size(); ++i)
			{
				TVector<ComponentHandle>& influenced_geometry = m_light_influenced_geometry[i];
				for (e_int32 j = 0; j < influenced_geometry.size(); ++j)
				{
					if (influenced_geometry[j] == component)
					{
						influenced_geometry.erase(j);
						break;
					}
				}
			}

			setEntity(component, nullptr);
			auto& entity_instance = m_entity_instances[component.index];
			GameObject entity = entity_instance.game_object;
			_delete(m_allocator, entity_instance.pose);
			entity_instance.pose = nullptr;
			entity_instance.game_object = INVALID_GAME_OBJECT;
			m_com_man.destroyComponent(entity, COMPONENT_ENTITY_INSTANCE_TYPE, this, component);
		}


		e_void SceneManager::destroyGlobalLight(ComponentHandle component)
		{
			GameObject entity = {component.index};

			m_com_man.destroyComponent(entity, COMPONENT_GLOBAL_LIGHT_TYPE, this, component);

			if (component == m_active_global_light_cmp)
			{
				m_active_global_light_cmp = INVALID_COMPONENT;
			}
			m_global_lights.erase(entity);
		}


		e_void SceneManager::destroyDecal(ComponentHandle component)
		{
			GameObject entity = {component.index};
			m_decals.erase(entity);
			m_com_man.destroyComponent(entity, COMPONENT_DECAL_TYPE, this, component);
		}


		e_void SceneManager::destroyPointLight(ComponentHandle component)
		{
			e_int32 index = m_point_lights_map[component];
			GameObject entity = m_point_lights[index].m_game_object;
			m_point_lights.eraseFast(index);
			m_point_lights_map.erase(component);
			m_light_influenced_geometry.eraseFast(index);
			if (index < m_point_lights.size())
			{
				m_point_lights_map[{m_point_lights[index].m_game_object.index}] = index;
			}
			m_com_man.destroyComponent(entity, COMPONENT_POINT_LIGHT_TYPE, this, component);
		}


		e_void SceneManager::destroyCamera(ComponentHandle component)
		{
			GameObject game_object = {component.index};
			m_cameras.erase(game_object);
			m_com_man.destroyComponent(game_object, COMPONENT_CAMERA_TYPE, this, component);
		}


		e_void SceneManager::destroyTerrain(ComponentHandle component)
		{
			//GameObject entity = {component.index};
			//_delete(m_allocator, m_terrains[entity]);
			//m_terrains.erase(entity);
			//m_com_man.destroyComponent(entity, TERRAIN_TYPE, this, component);
		}


		e_void SceneManager::destroyParticleEmitter(ComponentHandle component)
		{
			//auto* emitter = m_particle_emitters[{component.index}];
			//emitter->reset();
			//emitter->m_is_valid = false;
			//m_com_man.destroyComponent(emitter->m_entity, PARTICLE_EMITTER_TYPE, this, component);
			//cleanup(emitter);
		}

		ComponentHandle SceneManager::createCamera(GameObject game_object)
		{
			Camera camera;
			camera.is_ortho = false;
			camera.ortho_size = 10;
			camera.game_object = game_object;
			camera.fov = Math::degreesToRadians(60);
			camera.screen_width = 800;
			camera.screen_height = 600;
			camera.aspect = 800.0f / 600.0f;
			camera.near_flip = 0.1f;
			camera.far_flip = 10000.0f;
			camera.slot[0] = '\0';
			if (!getCameraInSlot("main").isValid()) 
				StringUnitl::copyString(camera.slot, "main");
			m_cameras.insert(game_object, camera);
			m_com_man.addComponent(game_object, COMPONENT_CAMERA_TYPE, this, { game_object.index});
			return { game_object.index};
		}

		EntityInstance* SceneManager::getEntityInstances()
		{
			return &m_entity_instances[0];
		}


		EntityInstance* SceneManager::getEntityInstance(ComponentHandle cmp)
		{
			return &m_entity_instances[cmp.index];
		}


		ComponentHandle SceneManager::getEntityInstanceComponent(GameObject entity)
		{
			ComponentHandle cmp = {entity.index};
			if (cmp.index >= m_entity_instances.size()) return INVALID_COMPONENT;
			if (!m_entity_instances[cmp.index].game_object.isValid()) return INVALID_COMPONENT;
			return cmp;
		}


		float3 SceneManager::getPoseBonePosition(ComponentHandle entity_instance, e_int32 bone_index)
		{
			Pose* pose = m_entity_instances[entity_instance.index].pose;
			return pose->positions[bone_index];
		}


		Frustum SceneManager::getPointLightFrustum(e_int32 light_idx) const
		{
			const PointLight& light = m_point_lights[light_idx];
			Frustum frustum;
			frustum.computeOrtho(m_com_man.getPosition(light.m_game_object),
				float3(1, 0, 0),
				float3(0, 1, 0),
				light.m_range,
				light.m_range,
				-light.m_range,
				light.m_range);

			return frustum;
		}


		e_void SceneManager::onEntityDestroyed(GameObject entity)
		{
			for (auto& i : m_bone_attachments)
			{
				if (i.parent_game_object == entity)
				{
					i.parent_game_object = INVALID_GAME_OBJECT;
					break;
				}
			}
		}


		e_void SceneManager::onEntityMoved(GameObject game_object)
		{
			e_int32 index = game_object.index;
			ComponentHandle cmp = {index};

			if (index < m_entity_instances.size() && m_entity_instances[index].game_object.isValid() &&
				m_entity_instances[index].entity && m_entity_instances[index].entity->isReady())
			{
				EntityInstance& r = m_entity_instances[index];
				r.matrix = m_com_man.getMatrix(game_object);
				if (r.entity && r.entity->isReady())
				{
					e_float radius = m_com_man.getScale(game_object) * r.entity->getBoundingRadius();
					float3 position = m_com_man.getPosition(game_object);
					m_culling_system->updateBoundingSphere({position, radius}, cmp);
				}

				e_float bounding_radius = r.entity ? r.entity->getBoundingRadius() : 1;
				for (e_int32 light_idx = 0, c = m_point_lights.size(); light_idx < c; ++light_idx)
				{
					for (e_int32 j = 0, c2 = m_light_influenced_geometry[light_idx].size(); j < c2; ++j)
					{
						if(m_light_influenced_geometry[light_idx][j] == cmp)
						{
							m_light_influenced_geometry[light_idx].eraseFast(j);
							break;
						}
					}

					float3 pos = m_com_man.getPosition(r.game_object);
					Frustum frustum = getPointLightFrustum(light_idx);
					if(frustum.isSphereInside(pos, bounding_radius))
					{
						m_light_influenced_geometry[light_idx].push_back(cmp);
					}
				}
			}

			e_int32 decal_idx = m_decals.find(game_object);
			if (decal_idx >= 0)
			{
				updateDecalInfo(m_decals.at(decal_idx));
			}

			for (e_int32 i = 0, c = m_point_lights.size(); i < c; ++i)
			{
				if (m_point_lights[i].m_game_object == game_object)
				{
					detectLightInfluencedGeometry({ m_point_lights[i].m_game_object.index });
					break;
				}
			}

			e_bool was_updating = m_is_updating_attachments;
			m_is_updating_attachments = true;
			for (auto& attachment : m_bone_attachments)
			{
				if (attachment.parent_game_object == game_object)
				{
					updateBoneAttachment(attachment);
				}
			}
			m_is_updating_attachments = was_updating;

			if (m_is_updating_attachments || m_is_game_running) return;
			for (auto& attachment : m_bone_attachments)
			{
				if (attachment.game_object == game_object)
				{
					updateRelativeMatrix(attachment);
					break;
				}
			}

		}

		EngineRoot& SceneManager::getEngine() const { return m_engine; }

		e_void SceneManager::setDecalScale(ComponentHandle cmp, const float3& value)
		{
			Decal& decal = m_decals[{cmp.index}];
			decal.scale = value;
			updateDecalInfo(decal);
		}


		float3 SceneManager::getDecalScale(ComponentHandle cmp)
		{
			return m_decals[{cmp.index}].scale;
		}


		e_void SceneManager::getDecals(const Frustum& frustum, TVector<DecalInfo>& decals)
		{
			decals.reserve(m_decals.size());
			for (const Decal& decal : m_decals)
			{
				if (!decal.material || !decal.material->isReady()) continue;
				if (frustum.isSphereInside(decal.position, decal.radius)) 
					decals.push_back(decal);
			}
		}


		e_void SceneManager::setDecalMaterialPath(ComponentHandle cmp, const ArchivePath& path)
		{
			ResourceManagerBase* material_manager = m_engine.getResourceManager().get(RESOURCE_MATERIAL_TYPE);
			Decal& decal = m_decals[{cmp.index}];
			if (decal.material)
			{
				material_manager->unload(*decal.material);
			}
			if (path.isValid())
			{
				decal.material = static_cast<Material*>(material_manager->load(path));
			}
			else
			{
				decal.material = nullptr;
			}
		}


		ArchivePath SceneManager::getDecalMaterialPath(ComponentHandle cmp)
		{
			Decal& decal = m_decals[{cmp.index}];
			return decal.material ? decal.material->getPath() : ArchivePath("");
		}

		Pose* SceneManager::lockPose(ComponentHandle cmp) { return m_entity_instances[cmp.index].pose; }
		e_void SceneManager::unlockPose(ComponentHandle cmp, e_bool changed)
		{
			if (!changed) return;
			if (cmp.index < m_entity_instances.size()
				&& (m_entity_instances[cmp.index].flags & EntityInstance::IS_BONE_ATTACHMENT_PARENT) == 0)
			{
				return;
			}

			GameObject parent = {cmp.index};
			for (BoneAttachment& ba : m_bone_attachments)
			{
				if (ba.parent_game_object != parent) continue;
				m_is_updating_attachments = true;
				updateBoneAttachment(ba);
				m_is_updating_attachments = false;
			}
		}


		GameObject SceneManager::getEntityInstanceGameObject(ComponentHandle cmp) { return m_entity_instances[cmp.index].game_object; }


		Entity* SceneManager::getEntityInstanceEntity(ComponentHandle cmp) { return m_entity_instances[cmp.index].entity; }





		e_void SceneManager::showEntityInstance(ComponentHandle cmp)
		{
			auto& entity_instance = m_entity_instances[cmp.index];
			if (!entity_instance.entity || !entity_instance.entity->isReady())
				return;

			Sphere sphere(m_com_man.getPosition(entity_instance.game_object), entity_instance.entity->getBoundingRadius());
			e_uint64 layer_mask = getLayerMask(entity_instance);
			if(!m_culling_system->isAdded(cmp)) m_culling_system->addStatic(cmp, sphere, layer_mask);
		}


		e_void SceneManager::hideEntityInstance(ComponentHandle cmp)
		{
			m_culling_system->removeStatic(cmp);
		}


		ArchivePath SceneManager::getEntityInstancePath(ComponentHandle cmp)
		{
			return m_entity_instances[cmp.index].entity ? m_entity_instances[cmp.index].entity->getPath() : ArchivePath("");
		}


		e_int32 SceneManager::getEntityInstanceMaterialsCount(ComponentHandle cmp)
		{
			return m_entity_instances[cmp.index].entity ? m_entity_instances[cmp.index].mesh_count : 0;
		}


		e_void SceneManager::setEntityInstancePath(ComponentHandle cmp, const ArchivePath& path)
		{
			EntityInstance& r = m_entity_instances[cmp.index];

			auto* manager = m_engine.getResourceManager().get( RESOURCE_ENTITY_TYPE);
			if (path.isValid())
			{
				Entity* entity = static_cast<Entity*>(manager->load(path));
				setEntity(cmp, entity);
			}
			else
			{
				setEntity(cmp, nullptr);
			}
			r.matrix = m_com_man.getMatrix(r.game_object);
		}

		e_void SceneManager::setTerrainHeightAt(ComponentHandle cmp, e_int32 x, e_int32 z, e_float height)
		{
			//m_terrains[{cmp.index}]->setHeight(x, z, height);
		}

		ComponentHandle SceneManager::getFirstEntityInstance()
		{
			return getNextEntityInstance(INVALID_COMPONENT);
		}

		ComponentHandle SceneManager::getNextEntityInstance(ComponentHandle cmp)
		{
			for(e_int32 i = cmp.index + 1; i < m_entity_instances.size(); ++i)
			{
				if (m_entity_instances[i].game_object != INVALID_GAME_OBJECT) return {i};
			}
			return INVALID_COMPONENT;
		}

	
		e_int32 SceneManager::getClosestPointLights(const float3& reference_pos,
										   ComponentHandle* lights,
										   e_int32 max_lights)
		{
			e_float dists[16];
			ASSERT(max_lights <= TlengthOf(dists));
			ASSERT(max_lights > 0);
			if (m_point_lights.empty()) return 0;

			e_int32 light_count = 0;
			for (auto light : m_point_lights)
			{
				float3 light_pos = m_com_man.getPosition(light.m_game_object);
				e_float dist_squared = (reference_pos - light_pos).squaredLength();

				dists[light_count] = dist_squared;
				lights[light_count] = { light.m_game_object.index };

				for (e_int32 i = light_count; i > 0 && dists[i - 1] > dists[i]; --i)
				{
					e_float tmp = dists[i];
					dists[i] = dists[i - 1];
					dists[i - 1] = tmp;

					ComponentHandle tmp2 = lights[i];
					lights[i] = lights[i - 1];
					lights[i - 1] = tmp2;
				}
				++light_count;
				if (light_count == max_lights)
				{
					break;
				}
			}

			for (e_int32 i = max_lights; i < m_point_lights.size(); ++i)
			{
				PointLight& light = m_point_lights[i];
				float3 light_pos = m_com_man.getPosition(light.m_game_object);
				e_float dist_squared = (reference_pos - light_pos).squaredLength();

				if (dist_squared < dists[max_lights - 1])
				{
					dists[max_lights - 1] = dist_squared;
					lights[max_lights - 1] = { light.m_game_object.index };

					for (e_int32 i = max_lights - 1; i > 0 && dists[i - 1] > dists[i];
						 --i)
					{
						e_float tmp = dists[i];
						dists[i] = dists[i - 1];
						dists[i - 1] = tmp;

						ComponentHandle tmp2 = lights[i];
						lights[i] = lights[i - 1];
						lights[i - 1] = tmp2;
					}
				}
			}

			return light_count;
		}


		e_void SceneManager::getPointLights(const Frustum& frustum, TVector<ComponentHandle>& lights)
		{
			for (e_int32 i = 0, ci = m_point_lights.size(); i < ci; ++i)
			{
				PointLight& light = m_point_lights[i];

				if (frustum.isSphereInside(m_com_man.getPosition(light.m_game_object), light.m_range))
				{
					lights.push_back({ light.m_game_object.index });
				}
			}
		}


		GameObject SceneManager::getCameraGameObject(ComponentHandle camera) const { return {camera.index}; }


		e_void SceneManager::setLightCastShadows(ComponentHandle cmp, e_bool cast_shadows)
		{
			m_point_lights[m_point_lights_map[cmp]].m_cast_shadows = cast_shadows;
		}


		e_bool SceneManager::getLightCastShadows(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_cast_shadows;
		}


		e_void SceneManager::getPointLightInfluencedGeometry(ComponentHandle light_cmp,
			const Frustum& frustum,
			TVector<EntityInstanceMesh>& infos)
		{
			PROFILE_FUNCTION();

			e_int32 light_index = m_point_lights_map[light_cmp];
			for (e_int32 j = 0, cj = m_light_influenced_geometry[light_index].size(); j < cj; ++j)
			{
				ComponentHandle entity_instance_cmp = m_light_influenced_geometry[light_index][j];
				EntityInstance& entity_instance = m_entity_instances[entity_instance_cmp.index];
				const Sphere& sphere = m_culling_system->getSphere(entity_instance_cmp);
				if (frustum.isSphereInside(sphere.position, sphere.radius))
				{
					for (e_int32 k = 0, kc = entity_instance.entity->getMeshCount(); k < kc; ++k)
					{
						auto& info = infos.emplace();
						info.mesh = &entity_instance.entity->getMesh(k);
						info.entity_instance = entity_instance_cmp;
					}
				}
			}
		}


		e_void SceneManager::getPointLightInfluencedGeometry(ComponentHandle light_cmp, TVector<EntityInstanceMesh>& infos)
		{
			PROFILE_FUNCTION();

			e_int32 light_index = m_point_lights_map[light_cmp];
			auto& geoms = m_light_influenced_geometry[light_index];
			for (e_int32 j = 0, cj = geoms.size(); j < cj; ++j)
			{
				const EntityInstance& entity_instance = m_entity_instances[geoms[j].index];
				for (e_int32 k = 0, kc = entity_instance.entity->getMeshCount(); k < kc; ++k)
				{
					auto& info = infos.emplace();
					info.mesh = &entity_instance.entity->getMesh(k);
					info.entity_instance = geoms[j];
				}
			}
		}


		e_void SceneManager::getEntityInstanceGameObjects(const Frustum& frustum, TVector<GameObject>& entities)
		{
			PROFILE_FUNCTION();

			auto& results = m_culling_system->cull(frustum, ~0ULL);

			for (auto& subresults : results)
			{
				for (ComponentHandle entity_instance_cmp : subresults)
				{
					entities.push_back(m_entity_instances[entity_instance_cmp.index].game_object);
				}
			}
		}


		e_float SceneManager::getCameraLODMultiplier(ComponentHandle camera)
		{
			e_float lod_multiplier;
			if (isCameraOrtho(camera))
			{
				lod_multiplier = 1;
			}
			else
			{
				lod_multiplier = getCameraFOV(camera) / Math::degreesToRadians(60);
				lod_multiplier *= lod_multiplier;
			}
			return lod_multiplier;
		}


		TVector<TVector<EntityInstanceMesh>>& SceneManager::getEntityInstanceInfos(const Frustum& frustum,
			const float3& lod_ref_point,
			ComponentHandle camera,
			e_uint64 layer_mask)
		{
			for (auto& i : m_temporary_infos) 
				i.clear();

			const CullingSystem::Results& results = m_culling_system->cull(frustum, layer_mask);

			while (m_temporary_infos.size() < results.size())
			{
				m_temporary_infos.emplace(m_allocator);
			}

			while (m_temporary_infos.size() > results.size())
			{
				m_temporary_infos.pop_back();
			}

			JobSystem::JobDecl jobs[64];
			JobSystem::LambdaJob job_storage[64];
			ASSERT(results.size() <= TlengthOf(jobs));

			volatile e_int32 counter = 0;
			for (e_int32 subresult_index = 0; subresult_index < results.size(); ++subresult_index)
			{
				TVector<EntityInstanceMesh>& subinfos = m_temporary_infos[subresult_index];
				subinfos.clear();

				JobSystem::fromLambda(
					[&layer_mask, &subinfos, this, &results, subresult_index, lod_ref_point, camera]() 
					{
						//PROFILE_BLOCK("Temporary Info Job");
						//PROFILE_INT("EntityInstance count", results[subresult_index].size());
						if (results[subresult_index].empty()) 
							return;

						e_float lod_multiplier = getCameraLODMultiplier(camera);
						float3 ref_point = lod_ref_point;
						e_float final_lod_multiplier = m_lod_multiplier * lod_multiplier;
						const ComponentHandle* RESTRICT raw_subresults = &results[subresult_index][0];
						EntityInstance* RESTRICT entity_instances = &m_entity_instances[0];
						for (e_int32 i = 0, c = results[subresult_index].size(); i < c; ++i)
						{
							const EntityInstance* RESTRICT entity_instance = &entity_instances[raw_subresults[i].index];
							e_float squared_distance = (entity_instance->matrix.getTranslation() - ref_point).squaredLength();
							squared_distance *= final_lod_multiplier;

							const Entity* RESTRICT entity = entity_instance->entity;
							LODMeshIndices lod = entity->getLODMeshIndices(squared_distance);
							for (e_int32 j = lod.from, c = lod.to; j <= c; ++j)
							{
								Mesh& mesh = entity_instance->meshes[j];
								if ((mesh.layer_mask & layer_mask) == 0) 
									continue;
						
								EntityInstanceMesh& info = subinfos.emplace();
								info.entity_instance	 = raw_subresults[i];
								info.mesh				 = &mesh;
								info.depth				 = squared_distance;
							}
						}
					},
				&job_storage[subresult_index], 
				&jobs[subresult_index], 
				nullptr);
			}
			JobSystem::runJobs(jobs, results.size(), &counter);
			JobSystem::wait(&counter);

			return m_temporary_infos;
		}


		e_void SceneManager::setCameraSlot(ComponentHandle cmp, const e_char* slot)
		{
			auto& camera = m_cameras[{cmp.index}];
			StringUnitl::copyString(camera.slot, TlengthOf(camera.slot), slot);
		}


		ComponentHandle SceneManager::getCameraComponent(GameObject entity)
		{
			auto iter = m_cameras.find(entity);
			if (!iter.isValid()) return INVALID_COMPONENT;
			return {entity.index};
		}

		const e_char* SceneManager::getCameraSlot(ComponentHandle camera) { return m_cameras[{camera.index}].slot; }
		e_float SceneManager::getCameraFOV(ComponentHandle camera) { return m_cameras[{camera.index}].fov; }
		e_void	SceneManager::setCameraFOV(ComponentHandle camera, e_float fov) { m_cameras[{camera.index}].fov = fov; }
		e_void	SceneManager::setCameraNearPlane(ComponentHandle camera, e_float near_plane) { m_cameras[{camera.index}].near_flip = Math::maximum(near_plane, 0.00001f); }
		e_float SceneManager::getCameraNearPlane(ComponentHandle camera) { return m_cameras[{camera.index}].near_flip; }
		e_void	SceneManager::setCameraFarPlane(ComponentHandle camera, e_float far_plane) { m_cameras[{camera.index}].far_flip = far_plane; }
		e_float SceneManager::getCameraFarPlane(ComponentHandle camera) { return m_cameras[{camera.index}].far_flip; }
		e_float SceneManager::getCameraScreenWidth(ComponentHandle camera) { return m_cameras[{camera.index}].screen_width; }
		e_float SceneManager::getCameraScreenHeight(ComponentHandle camera) { return m_cameras[{camera.index}].screen_height; }


		e_void	SceneManager::setGlobalLODMultiplier(e_float multiplier) { m_lod_multiplier = multiplier; }
		e_float SceneManager::getGlobalLODMultiplier() const { return m_lod_multiplier; }


		float4x4 SceneManager::getCameraViewProjection(ComponentHandle cmp)
		{
			float4x4 view = m_com_man.getMatrix({cmp.index});
			view.fastInverse();
			return getCameraProjection(cmp) * view;
		}


		float4x4 SceneManager::getCameraProjection(ComponentHandle cmp)
		{
			Camera& camera = m_cameras[{cmp.index}];
			float4x4 mtx;
			e_float ratio = camera.screen_height > 0 ? camera.screen_width / camera.screen_height : 1;
			e_bool is_homogenous_depth = bgfx::getCaps()->homogeneousDepth;
			if (camera.is_ortho)
			{
				mtx.setOrtho(-camera.ortho_size * ratio,
					camera.ortho_size * ratio,
					-camera.ortho_size,
					camera.ortho_size,
					camera.near_flip,
					camera.far_flip,
					is_homogenous_depth);
			}
			else
			{
				mtx.setPerspective(camera.fov, ratio, camera.near_flip, camera.far_flip, is_homogenous_depth);
			}

			//mtx.m31 = -mtx.m31;
			//mtx.m32 = -mtx.m32;
			//mtx.m33 = -mtx.m33;
			//mtx.m34 = 1.0f;

			return mtx;
		}


		e_void SceneManager::setCameraScreenSize(ComponentHandle camera, e_int32 w, e_int32 h)
		{
			auto& cam = m_cameras[{camera.index}];
			cam.screen_width	= (e_float)w;
			cam.screen_height	= (e_float)h;
			cam.aspect			= w / (e_float)h;
		}


		float2 SceneManager::getCameraScreenSize(ComponentHandle camera)
		{
			auto& cam = m_cameras[{camera.index}];
			return float2(cam.screen_width, cam.screen_height);
		}


		e_float SceneManager::getCameraOrthoSize(ComponentHandle camera) { return m_cameras[{camera.index}].ortho_size; }
		e_void SceneManager::setCameraOrthoSize(ComponentHandle camera, e_float value) { m_cameras[{camera.index}].ortho_size = value; }
		e_bool SceneManager::isCameraOrtho(ComponentHandle camera) { return m_cameras[{camera.index}].is_ortho; }
		e_void SceneManager::setCameraOrtho(ComponentHandle camera, e_bool is_ortho) { m_cameras[{camera.index}].is_ortho = is_ortho; }


		const TVector<DebugTriangle>& SceneManager::getDebugTriangles() const { return m_debug_triangles; }
		const TVector<DebugLine>& SceneManager::getDebugLines() const { return m_debug_lines; }
		const TVector<DebugPoint>& SceneManager::getDebugPoints() const { return m_debug_points; }


		e_void SceneManager::addDebugSphere(const float3& center,
			e_float radius,
			e_uint32 color,
			e_float life)
		{
			static const e_int32 COLS = 36;
			static const e_int32 ROWS = COLS >> 1;
			static const e_float STEP = (Math::C_Pi / 180.0f) * 360.0f / COLS;
			e_int32 p2 = COLS >> 1;
			e_int32 r2 = ROWS >> 1;
			e_float prev_ci = 1;
			e_float prev_si = 0;
			for (e_int32 y = -r2; y < r2; ++y)
			{
				e_float cy = cos(y * STEP);
				e_float cy1 = cos((y + 1) * STEP);
				e_float sy = sin(y * STEP);
				e_float sy1 = sin((y + 1) * STEP);

				for (e_int32 i = -p2; i < p2; ++i)
				{
					e_float ci = cos(i * STEP);
					e_float si = sin(i * STEP);
					addDebugLine(float3(center.x + radius * ci * cy,
									  center.y + radius * sy,
									  center.z + radius * si * cy),
								 float3(center.x + radius * ci * cy1,
									  center.y + radius * sy1,
									  center.z + radius * si * cy1),
								 color,
								 life);
					addDebugLine(float3(center.x + radius * ci * cy,
									  center.y + radius * sy,
									  center.z + radius * si * cy),
								 float3(center.x + radius * prev_ci * cy,
									  center.y + radius * sy,
									  center.z + radius * prev_si * cy),
								 color,
								 life);
					addDebugLine(float3(center.x + radius * prev_ci * cy1,
									  center.y + radius * sy1,
									  center.z + radius * prev_si * cy1),
								 float3(center.x + radius * ci * cy1,
									  center.y + radius * sy1,
									  center.z + radius * si * cy1),
								 color,
								 life);
					prev_ci = ci;
					prev_si = si;
				}
			}
		}


		e_void SceneManager::addDebugHalfSphere(const float4x4& transform, e_float radius, e_bool top, e_uint32 color, e_float life)
		{
			float3 center = transform.getTranslation();
			float3 x_vec = transform.getXVector();
			float3 y_vec = transform.getYVector();
			if (!top) y_vec *= -1;
			float3 z_vec = transform.getZVector();
			static const e_int32 COLS = 36;
			static const e_int32 ROWS = COLS >> 1;
			static const e_float STEP = Math::degreesToRadians(360.0f) / COLS;
			for (e_int32 y = 0; y < ROWS >> 1; ++y)
			{
				e_float cy = cos(y * STEP);
				e_float cy1 = cos((y + 1) * STEP);
				e_float sy = sin(y * STEP);
				e_float sy1 = sin((y + 1) * STEP);
				e_float prev_ci = cos(-STEP);
				e_float prev_si = sin(-STEP);

				float3 y_offset = y_vec * sy;
				float3 y_offset1 = y_vec * sy1;

				for (e_int32 i = 0; i < COLS; ++i)
				{
					e_float ci = cos(i * STEP);
					e_float si = sin(i * STEP);

					addDebugLine(
						center + radius * (x_vec * ci * cy + z_vec * si * cy + y_offset),
						center + radius * (x_vec * prev_ci * cy + z_vec * prev_si * cy + y_offset),
						color,
						life);
					addDebugLine(
						center + radius * (x_vec * ci * cy + z_vec * si * cy + y_offset),
						center + radius * (x_vec * ci * cy1 + z_vec * si * cy1 + y_offset1),
						color,
						life);
					prev_ci = ci;
					prev_si = si;
				}
			}
		}


		e_void SceneManager::addDebugHalfSphere(const float3& center, e_float radius, e_bool top, e_uint32 color, e_float life)
		{
			static const e_int32 COLS = 36;
			static const e_int32 ROWS = COLS >> 1;
			static const e_float STEP = (Math::C_Pi / 180.0f) * 360.0f / COLS;
			e_int32 p2 = COLS >> 1;
			e_int32 yfrom = top ? 0 : -(ROWS >> 1);
			e_int32 yto = top ? ROWS >> 1 : 0;
			for (e_int32 y = yfrom; y < yto; ++y)
			{
				e_float cy = cos(y * STEP);
				e_float cy1 = cos((y + 1) * STEP);
				e_float sy = sin(y * STEP);
				e_float sy1 = sin((y + 1) * STEP);
				e_float prev_ci = cos((-p2 - 1) * STEP);
				e_float prev_si = sin((-p2 - 1) * STEP);

				for (e_int32 i = -p2; i < p2; ++i)
				{
					e_float ci = cos(i * STEP);
					e_float si = sin(i * STEP);
					addDebugLine(float3(center.x + radius * ci * cy,
						center.y + radius * sy,
						center.z + radius * si * cy),
						float3(center.x + radius * ci * cy1,
						center.y + radius * sy1,
						center.z + radius * si * cy1),
						color,
						life);
					addDebugLine(float3(center.x + radius * ci * cy,
						center.y + radius * sy,
						center.z + radius * si * cy),
						float3(center.x + radius * prev_ci * cy,
						center.y + radius * sy,
						center.z + radius * prev_si * cy),
						color,
						life);
					addDebugLine(float3(center.x + radius * prev_ci * cy1,
						center.y + radius * sy1,
						center.z + radius * prev_si * cy1),
						float3(center.x + radius * ci * cy1,
						center.y + radius * sy1,
						center.z + radius * si * cy1),
						color,
						life);
					prev_ci = ci;
					prev_si = si;
				}
			}
		}


		e_void SceneManager::addDebugTriangle(const float3& p0,
			const float3& p1,
			const float3& p2,
			e_uint32 color,
			e_float life)
		{
			DebugTriangle& tri = m_debug_triangles.emplace();
			tri.p0 = p0;
			tri.p1 = p1;
			tri.p2 = p2;
			tri.color = ARGBToABGR(color);
			tri.life = life;
		}


		e_void SceneManager::addDebugCapsule(const float3& position,
			e_float height,
			e_float radius,
			e_uint32 color,
			e_float life)
		{
			addDebugHalfSphere(position + float3(0, radius, 0), radius, false, color, life);
			addDebugHalfSphere(position + float3(0, radius + height, 0), radius, true, color, life);

			float3 z_vec(0, 0, 1.0f);
			float3 x_vec(1.0f, 0, 0);
			z_vec.normalize();
			x_vec.normalize();
			float3 bottom = position + float3(0, radius, 0);
			float3 top = bottom + float3(0, height, 0);
			for (e_int32 i = 1; i <= 32; ++i)
			{
				e_float a = i / 32.0f * 2 * Math::C_Pi;
				e_float x = cosf(a) * radius;
				e_float z = sinf(a) * radius;
				addDebugLine(bottom + x_vec * x + z_vec * z,
					top + x_vec * x + z_vec * z,
					color,
					life);
			}
		}


		e_void SceneManager::addDebugCapsule(const float4x4& transform,
			e_float height,
			e_float radius,
			e_uint32 color,
			e_float life)
		{
			float3 x_vec = transform.getXVector();
			float3 y_vec = transform.getYVector();
			float3 z_vec = transform.getZVector();
			float3 position = transform.getTranslation();

			float4x4 tmp = transform;
			tmp.setTranslation(transform.getTranslation() + y_vec * radius);
			addDebugHalfSphere(tmp, radius, false, color, life);
			tmp.setTranslation(transform.getTranslation() + y_vec * (radius + height));
			addDebugHalfSphere(tmp, radius, true, color, life);

			float3 bottom = position + y_vec * radius;
			float3 top = bottom + y_vec * height;
			for (e_int32 i = 1; i <= 32; ++i)
			{
				e_float a = i / 32.0f * 2 * Math::C_Pi;
				e_float x = cosf(a) * radius;
				e_float z = sinf(a) * radius;
				addDebugLine(bottom + x_vec * x + z_vec * z, top + x_vec * x + z_vec * z, color, life);
			}
		}



		e_void SceneManager::addDebugCylinder(const float3& position,
									  const float3& up,
									  e_float radius,
									  e_uint32 color,
									  e_float life)
		{
			float3 z_vec(-up.y, up.x, 0);
			float3 x_vec = crossProduct(up, z_vec);
			e_float prevx = radius;
			e_float prevz = 0;
			z_vec.normalize();
			x_vec.normalize();
			float3 top = position + up;
			for (e_int32 i = 1; i <= 32; ++i)
			{
				e_float a = i / 32.0f * 2 * Math::C_Pi;
				e_float x = cosf(a) * radius;
				e_float z = sinf(a) * radius;
				addDebugLine(position + x_vec * x + z_vec * z,
							 position + x_vec * prevx + z_vec * prevz,
							 color,
							 life);
				addDebugLine(top + x_vec * x + z_vec * z,
							 top + x_vec * prevx + z_vec * prevz,
							 color,
							 life);
				addDebugLine(position + x_vec * x + z_vec * z,
							 top + x_vec * x + z_vec * z,
							 color,
							 life);
				prevx = x;
				prevz = z;
			}
		}


		e_void SceneManager::addDebugCube(const float3& pos,
			const float3& dir,
			const float3& up,
			const float3& right,
			e_uint32 color,
			e_float life)
		{
			addDebugLine(pos + dir + up + right, pos + dir + up - right, color, life);
			addDebugLine(pos - dir + up + right, pos - dir + up - right, color, life);
			addDebugLine(pos + dir + up + right, pos - dir + up + right, color, life);
			addDebugLine(pos + dir + up - right, pos - dir + up - right, color, life);

			addDebugLine(pos + dir - up + right, pos + dir - up - right, color, life);
			addDebugLine(pos - dir - up + right, pos - dir - up - right, color, life);
			addDebugLine(pos + dir - up + right, pos - dir - up + right, color, life);
			addDebugLine(pos + dir - up - right, pos - dir - up - right, color, life);

			addDebugLine(pos + dir + up + right, pos + dir - up + right, color, life);
			addDebugLine(pos + dir + up - right, pos + dir - up - right, color, life);
			addDebugLine(pos - dir + up + right, pos - dir - up + right, color, life);
			addDebugLine(pos - dir + up - right, pos - dir - up - right, color, life);

		}


		e_void SceneManager::addDebugCubeSolid(const float3& min,
			const float3& max,
			e_uint32 color,
			e_float life)
		{
			float3 a = min;
			float3 b = min;
			float3 c = max;

			b.x = max.x;
			c.z = min.z;
			addDebugTriangle(a, c, b, color, life);
			b.x = min.x;
			b.y = max.y;
			addDebugTriangle(a, b, c, color, life);

			b = max;
			c = max;
			a.z = max.z;
			b.y = min.y;
			addDebugTriangle(a, b, c, color, life);
			b.x = min.x;
			b.y = max.y;
			addDebugTriangle(a, c, b, color, life);

			a = min;
			b = min;
			c = max;

			b.x = max.x;
			c.y = min.y;
			addDebugTriangle(a, b, c, color, life);
			b.x = min.x;
			b.z = max.z;
			addDebugTriangle(a, c, b, color, life);

			b = max;
			c = max;
			a.y = max.y;
			b.z = min.z;
			addDebugTriangle(a, c, b, color, life);
			b.x = min.x;
			b.z = max.z;
			addDebugTriangle(a, b, c, color, life);

			a = min;
			b = min;
			c = max;

			b.y = max.y;
			c.x = min.x;
			addDebugTriangle(a, c, b, color, life);
			b.y = min.y;
			b.z = max.z;
			addDebugTriangle(a, b, c, color, life);

			b = max;
			c = max;
			a.x = max.x;
			b.z = min.z;
			addDebugTriangle(a, b, c, color, life);
			b.y = min.y;
			b.z = max.z;
			addDebugTriangle(a, c, b, color, life);
		}



		e_void SceneManager::addDebugCube(const float3& min,
								  const float3& max,
								  e_uint32 color,
								  e_float life)
		{
			float3 a = min;
			float3 b = min;
			b.x = max.x;
			addDebugLine(a, b, color, life);
			a.set(b.x, b.y, max.z);
			addDebugLine(a, b, color, life);
			b.set(min.x, a.y, a.z);
			addDebugLine(a, b, color, life);
			a.set(b.x, b.y, min.z);
			addDebugLine(a, b, color, life);

			a = min;
			a.y = max.y;
			b = a;
			b.x = max.x;
			addDebugLine(a, b, color, life);
			a.set(b.x, b.y, max.z);
			addDebugLine(a, b, color, life);
			b.set(min.x, a.y, a.z);
			addDebugLine(a, b, color, life);
			a.set(b.x, b.y, min.z);
			addDebugLine(a, b, color, life);

			a = min;
			b = a;
			b.y = max.y;
			addDebugLine(a, b, color, life);
			a.x = max.x;
			b.x = max.x;
			addDebugLine(a, b, color, life);
			a.z = max.z;
			b.z = max.z;
			addDebugLine(a, b, color, life);
			a.x = min.x;
			b.x = min.x;
			addDebugLine(a, b, color, life);
		}


		e_void SceneManager::addDebugFrustum(const Frustum& frustum, e_uint32 color, e_float life)
		{
			addDebugLine(frustum.points[0], frustum.points[1], color, life);
			addDebugLine(frustum.points[1], frustum.points[2], color, life);
			addDebugLine(frustum.points[2], frustum.points[3], color, life);
			addDebugLine(frustum.points[3], frustum.points[0], color, life);

			addDebugLine(frustum.points[4], frustum.points[5], color, life);
			addDebugLine(frustum.points[5], frustum.points[6], color, life);
			addDebugLine(frustum.points[6], frustum.points[7], color, life);
			addDebugLine(frustum.points[7], frustum.points[4], color, life);

			addDebugLine(frustum.points[0], frustum.points[4], color, life);
			addDebugLine(frustum.points[1], frustum.points[5], color, life);
			addDebugLine(frustum.points[2], frustum.points[6], color, life);
			addDebugLine(frustum.points[3], frustum.points[7], color, life);
		}


		e_void SceneManager::addDebugCircle(const float3& center, const float3& up, e_float radius, e_uint32 color, e_float life)
		{
			float3 z_vec(-up.y, up.x, 0);
			float3 x_vec = crossProduct(up, z_vec);
			e_float prevx = radius;
			e_float prevz = 0;
			z_vec.normalize();
			x_vec.normalize();
			for (e_int32 i = 1; i <= 64; ++i)
			{
				e_float a = i / 64.0f * 2 * Math::C_Pi;
				e_float x = cosf(a) * radius;
				e_float z = sinf(a) * radius;
				addDebugLine(center + x_vec * x + z_vec * z, center + x_vec * prevx + z_vec * prevz, color, life);
				prevx = x;
				prevz = z;
			}
		}


		e_void SceneManager::addDebugCross(const float3& center, e_float size, e_uint32 color, e_float life)
		{
			addDebugLine(center, float3(center.x - size, center.y, center.z), color, life);
			addDebugLine(center, float3(center.x + size, center.y, center.z), color, life);
			addDebugLine(center, float3(center.x, center.y - size, center.z), color, life);
			addDebugLine(center, float3(center.x, center.y + size, center.z), color, life);
			addDebugLine(center, float3(center.x, center.y, center.z - size), color, life);
			addDebugLine(center, float3(center.x, center.y, center.z + size), color, life);
		}


		e_void SceneManager::addDebugPoint(const float3& pos, e_uint32 color, e_float life)
		{
			DebugPoint& point = m_debug_points.emplace();
			point.pos = pos;
			point.color = ARGBToABGR(color);
			point.life = life;
		}


		e_void SceneManager::addDebugCone(const float3& vertex,
			const float3& dir,
			const float3& axis0,
			const float3& axis1,
			e_uint32 color,
			e_float life)
		{
			float3 base_center = vertex + dir;
			float3 prev_p = base_center + axis0;
			for (e_int32 i = 1; i <= 32; ++i)
			{
				e_float angle = i / 32.0f * 2 * Math::C_Pi;
				float3 x = cosf(angle) * axis0;
				float3 z = sinf(angle) * axis1;
				float3 p = base_center + x + z;
				addDebugLine(p, prev_p, color, life);
				addDebugLine(vertex, p, color, life);
				prev_p = p;
			}
		}





		e_void SceneManager::addDebugLine(const float3& from, const float3& to, e_uint32 color, e_float life)
		{
			DebugLine& line = m_debug_lines.emplace();
			line.from = from;
			line.to = to;
			line.color = ARGBToABGR(color);
			line.life = life;
		}


		RayCastEntityHit SceneManager::castRayTerrain(ComponentHandle cmp, const float3& origin, const float3& dir)
		{
			RayCastEntityHit hit;
			hit.m_is_hit = false;
			//auto iter = m_terrains.find({cmp.index});
			//if (!iter.isValid()) return hit;

			//auto* terrain = iter.value();
			//hit = terrain->castRay(origin, dir);
			//hit.m_component = cmp;
			//hit.m_component_type = TERRAIN_TYPE;
			//hit.m_entity = terrain->getEntity();
			return hit;
		}


		RayCastEntityHit SceneManager::castRay(const float3& origin, const float3& dir, ComponentHandle ignored_entity_instance)
		{
			PROFILE_FUNCTION();
			RayCastEntityHit hit;
			hit.m_is_hit = false;
			hit.m_origin = origin;
			hit.m_dir = dir;
			e_float cur_dist = FLT_MAX;
			ComponentManager& com_man = getComponentManager();
			for (e_int32 i = 0; i < m_entity_instances.size(); ++i)
			{
				auto& r = m_entity_instances[i];
				if (ignored_entity_instance.index == i || !r.entity) continue;

				const float3& pos = r.matrix.getTranslation();
				e_float scale = com_man.getScale(r.game_object);
				e_float radius = r.entity->getBoundingRadius() * scale;
				e_float dist = (pos - origin).length();
				if (dist - radius > cur_dist) continue;
			
				float3 intersection;
				if (Math::getRaySphereIntersection(origin, dir, pos, radius, intersection))
				{
					RayCastEntityHit new_hit = r.entity->castRay(origin, dir, r.matrix, r.pose);
					if (new_hit.m_is_hit && (!hit.m_is_hit || new_hit.m_t < hit.m_t))
					{
						new_hit.m_component = {i};
						new_hit.m_game_object = r.game_object;
						new_hit.m_component_type = COMPONENT_ENTITY_INSTANCE_TYPE;
						hit = new_hit;
						hit.m_is_hit = true;
						cur_dist = dir.length() * hit.m_t;
					}
				}
			}

			//for (auto* terrain : m_terrains)
			//{
			//	RayCastEntityHit terrain_hit = terrain->castRay(origin, dir);
			//	if (terrain_hit.m_is_hit && (!hit.m_is_hit || terrain_hit.m_t < hit.m_t))
			//	{
			//		terrain_hit.m_component = {terrain->getEntity().index};
			//		terrain_hit.m_component_type = TERRAIN_TYPE;
			//		terrain_hit.m_entity = terrain->getEntity();
			//		hit = terrain_hit;
			//	}
			//}

			return hit;
		}

	
		float4 SceneManager::getShadowmapCascades(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_cascades;
		}


		e_void SceneManager::setShadowmapCascades(ComponentHandle cmp, const float4& value)
		{
			float4 valid_value = value;
			valid_value.x = Math::maximum(valid_value.x, 0.02f);
			valid_value.y = Math::maximum(valid_value.x + 0.01f, valid_value.y);
			valid_value.z = Math::maximum(valid_value.y + 0.01f, valid_value.z);
			valid_value.w = Math::maximum(valid_value.z + 0.01f, valid_value.w);

			m_global_lights[{cmp.index}].m_cascades = valid_value;
		}


		e_void SceneManager::setFogDensity(ComponentHandle cmp, e_float density)
		{
			m_global_lights[{cmp.index}].m_fog_density = density;
		}


		e_void SceneManager::setFogColor(ComponentHandle cmp, const float3& color)
		{
			m_global_lights[{cmp.index}].m_fog_color = color;
		}


		e_float SceneManager::getFogDensity(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_fog_density;
		}


		e_float SceneManager::getFogBottom(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_fog_bottom;
		}


		e_void SceneManager::setFogBottom(ComponentHandle cmp, e_float bottom)
		{
			m_global_lights[{cmp.index}].m_fog_bottom = bottom;
		}


		e_float SceneManager::getFogHeight(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_fog_height;
		}


		e_void SceneManager::setFogHeight(ComponentHandle cmp, e_float height)
		{
			m_global_lights[{cmp.index}].m_fog_height = height;
		}


		float3 SceneManager::getFogColor(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_fog_color;
		}


		e_float SceneManager::getLightAttenuation(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_attenuation_param;
		}


		e_void SceneManager::setLightAttenuation(ComponentHandle cmp, e_float attenuation)
		{
			e_int32 index = m_point_lights_map[cmp];
			m_point_lights[index].m_attenuation_param = attenuation;
		}


		e_float SceneManager::getLightRange(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_range;
		}


		e_void SceneManager::setLightRange(ComponentHandle cmp, e_float value)
		{
			m_point_lights[m_point_lights_map[cmp]].m_range = value;
		}


		e_void SceneManager::setPointLightIntensity(ComponentHandle cmp, e_float intensity)
		{
			m_point_lights[m_point_lights_map[cmp]].m_diffuse_intensity = intensity;
		}


		e_void SceneManager::setGlobalLightIntensity(ComponentHandle cmp, e_float intensity)
		{
			m_global_lights[{cmp.index}].m_diffuse_intensity = intensity;
		}


		e_void SceneManager::setGlobalLightIndirectIntensity(ComponentHandle cmp, e_float intensity)
		{
			m_global_lights[{cmp.index}].m_indirect_intensity = intensity;
		}


		e_void SceneManager::setPointLightColor(ComponentHandle cmp, const float3& color)
		{
			m_point_lights[m_point_lights_map[cmp]].m_diffuse_color = color;
		}


		e_void SceneManager::setGlobalLightColor(ComponentHandle cmp, const float3& color)
		{
			m_global_lights[{cmp.index}].m_diffuse_color = color;
		}

	
		e_float SceneManager::getPointLightIntensity(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_diffuse_intensity;
		}


		e_float SceneManager::getGlobalLightIntensity(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_diffuse_intensity;
		}


		e_float SceneManager::getGlobalLightIndirectIntensity(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_indirect_intensity;
		}


		float3 SceneManager::getPointLightColor(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_diffuse_color;
		}


		e_void SceneManager::setPointLightSpecularColor(ComponentHandle cmp, const float3& color)
		{
			m_point_lights[m_point_lights_map[cmp]].m_specular_color = color;
		}


		float3 SceneManager::getPointLightSpecularColor(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_specular_color;
		}


		e_void SceneManager::setPointLightSpecularIntensity(ComponentHandle cmp, e_float intensity)
		{
			m_point_lights[m_point_lights_map[cmp]].m_specular_intensity = intensity;
		}


		e_float SceneManager::getPointLightSpecularIntensity(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_specular_intensity;
		}


		float3 SceneManager::getGlobalLightColor(ComponentHandle cmp)
		{
			return m_global_lights[{cmp.index}].m_diffuse_color;
		}


		e_void SceneManager::setActiveGlobalLight(ComponentHandle cmp)
		{
			m_active_global_light_cmp = cmp;
		}


		ComponentHandle SceneManager::getActiveGlobalLight()
		{
			return m_active_global_light_cmp;
		}


		GameObject SceneManager::getPointLightGameObject(ComponentHandle cmp) const
		{
			return m_point_lights[m_point_lights_map[cmp]].m_game_object;
		}


		GameObject SceneManager::getGlobalLightGameObject(ComponentHandle cmp) const
		{
			return m_global_lights[{cmp.index}].m_game_object;
		}


		e_void SceneManager::reloadEnvironmentProbe(ComponentHandle cmp)
		{
			GameObject entity = {cmp.index};
			auto& probe = m_environment_probes[entity];
			auto* texture_manager = m_engine.getResourceManager().get( RESOURCE_TEXTURE_TYPE);
			if (probe.texture) texture_manager->unload(*probe.texture);
			StaticString<MAX_PATH_LENGTH> path("com_mans/", m_com_man.getName(), "/probes/", probe.guid, ".dds");
			probe.texture = static_cast<Texture*>(texture_manager->load(ArchivePath(path)));
			probe.texture->setFlag(BGFX_TEXTURE_SRGB, true);
			path = "com_mans/";
			path << m_com_man.getName() << "/probes/" << probe.guid << "_irradiance.dds";
			probe.irradiance = static_cast<Texture*>(texture_manager->load(ArchivePath(path)));
			probe.irradiance->setFlag(BGFX_TEXTURE_SRGB, true);
			probe.irradiance->setFlag(BGFX_TEXTURE_MIN_ANISOTROPIC, true);
			probe.irradiance->setFlag(BGFX_TEXTURE_MAG_ANISOTROPIC, true);
			path = "com_mans/";
			path << m_com_man.getName() << "/probes/" << probe.guid << "_radiance.dds";
			probe.radiance = static_cast<Texture*>(texture_manager->load(ArchivePath(path)));
			probe.radiance->setFlag(BGFX_TEXTURE_SRGB, true);
			probe.radiance->setFlag(BGFX_TEXTURE_MIN_ANISOTROPIC, true);
			probe.radiance->setFlag(BGFX_TEXTURE_MAG_ANISOTROPIC, true);
		}


		ComponentHandle SceneManager::getNearestEnvironmentProbe(const float3& pos) const
		{
			e_float nearest_dist_squared = FLT_MAX;
			GameObject nearest = INVALID_GAME_OBJECT;
			for (e_int32 i = 0, c = m_environment_probes.size(); i < c; ++i)
			{
				GameObject probe_entity = m_environment_probes.getKey(i);
				float3 probe_pos = m_com_man.getPosition(probe_entity);
				e_float dist_squared = (pos - probe_pos).squaredLength();
				if (dist_squared < nearest_dist_squared)
				{
					nearest = probe_entity;
					nearest_dist_squared = dist_squared;
				}
			}
			if (!nearest.isValid()) return INVALID_COMPONENT;
			return {nearest.index};
		}


		Texture* SceneManager::getEnvironmentProbeTexture(ComponentHandle cmp) const
		{
			GameObject entity = {cmp.index};
			return m_environment_probes[entity].texture;
		}


		Texture* SceneManager::getEnvironmentProbeIrradiance(ComponentHandle cmp) const
		{
			GameObject entity = {cmp.index};
			return m_environment_probes[entity].irradiance;
		}


		Texture* SceneManager::getEnvironmentProbeRadiance(ComponentHandle cmp) const
		{
			GameObject entity = {cmp.index};
			return m_environment_probes[entity].radiance;
		}


		e_uint64 SceneManager::getEnvironmentProbeGUID(ComponentHandle cmp) const
		{
			GameObject entity = { cmp.index };
			return m_environment_probes[entity].guid;
		}


		ComponentHandle SceneManager::getCameraInSlot(const e_char* slot)
		{
			for (const auto& camera : m_cameras)
			{
				if (StringUnitl::equalStrings(camera.slot, slot))
				{
					return {camera.game_object.index};
				}
			}
			return INVALID_COMPONENT;
		}


		e_float SceneManager::getTime() const { return m_time; }


		e_void SceneManager::entityUnloaded(Entity*, ComponentHandle component)
		{
			auto& r = m_entity_instances[component.index];
			if (!hasCustomMeshes(r))
			{
				r.meshes = nullptr;
				r.mesh_count = 0;
			}
			_delete(m_allocator, r.pose);
			r.pose = nullptr;

			for (e_int32 i = 0; i < m_point_lights.size(); ++i)
			{
				m_light_influenced_geometry[i].eraseItemFast(component);
			}
			m_culling_system->removeStatic(component);
		}


		e_void SceneManager::freeCustomMeshes(EntityInstance& r, MaterialManager* manager)
		{
			if (!hasCustomMeshes(r)) return;
			for (e_int32 i = 0; i < r.mesh_count; ++i)
			{
				manager->unload(*r.meshes[i].material);
				r.meshes[i].~Mesh();
			}
			m_allocator.deallocate(r.meshes);
			r.meshes = nullptr;
			r.flags = r.flags & ~(e_uint8)EntityInstance::CUSTOM_MESHES;
			r.mesh_count = 0;
		}


		e_void SceneManager::entityLoaded(Entity* entity, ComponentHandle component)
		{
			auto& rm = m_engine.getResourceManager();
			auto* material_manager = static_cast<MaterialManager*>(rm.get(RESOURCE_MATERIAL_TYPE));

			auto& r = m_entity_instances[component.index];

			e_float bounding_radius = r.entity->getBoundingRadius();
			e_float scale = m_com_man.getScale(r.game_object);
			Sphere sphere(r.matrix.getTranslation(), bounding_radius * scale);
			m_culling_system->addStatic(component, sphere, getLayerMask(r));
			ASSERT(!r.pose);
			if (entity->getBoneCount() > 0)
			{
				r.pose = _aligned_new(m_allocator, Pose)(m_allocator);
				r.pose->resize(entity->getBoneCount());
				entity->getPose(*r.pose);
				e_int32 skinned_define_idx = m_renderer.getShaderDefineIdx("SKINNED");
				for (e_int32 i = 0; i < entity->getMeshCount(); ++i)
				{
					Mesh& mesh = entity->getMesh(i);
					mesh.material->setDefine(skinned_define_idx, !mesh.skin.empty());
				}
			}
			r.matrix = m_com_man.getMatrix(r.game_object);
			ASSERT(!r.meshes || hasCustomMeshes(r));
			if (r.meshes)
			{
				allocateCustomMeshes(r, entity->getMeshCount());
				for (e_int32 i = 0; i < r.mesh_count; ++i)
				{
					auto& src = entity->getMesh(i);
					if (!r.meshes[i].material)
					{
						material_manager->load(*src.material);
						r.meshes[i].material = src.material;
					}
					r.meshes[i].set(src);
				}
			}
			else
			{
				r.meshes = &r.entity->getMesh(0);
				r.mesh_count = r.entity->getMeshCount();
			}

			if ((r.flags & EntityInstance::IS_BONE_ATTACHMENT_PARENT) != 0)
			{
				updateBoneAttachment(m_bone_attachments[r.game_object]);
			}

			for (e_int32 i = 0; i < m_point_lights.size(); ++i)
			{
				PointLight& light = m_point_lights[i];
				float3 t = r.matrix.getTranslation();
				e_float radius = r.entity->getBoundingRadius();
				if ((t - m_com_man.getPosition(light.m_game_object)).squaredLength() <
					(radius + light.m_range) * (radius + light.m_range))
				{
					m_light_influenced_geometry[i].push_back(component);
				}
			}
		}


		e_void SceneManager::EntityUnloaded(Entity* entity)
		{
			for (e_int32 i = 0, c = m_entity_instances.size(); i < c; ++i)
			{
				if (m_entity_instances[i].game_object != INVALID_GAME_OBJECT && m_entity_instances[i].entity == entity)
				{
					entityUnloaded(entity, {i});
				}
			}
		}


		e_void SceneManager::EntityLoaded(Entity* entity)
		{
			for (e_int32 i = 0, c = m_entity_instances.size(); i < c; ++i)
			{
				if (m_entity_instances[i].game_object != INVALID_GAME_OBJECT && m_entity_instances[i].entity == entity)
				{
					entityLoaded(entity, {i});
				}
			}
		}


		SceneManager::EntityLoadedCallback& SceneManager::getEntityLoadedCallback(Entity* entity)
		{
			e_int32 idx = m_entity_loaded_callbacks.find(entity);
			if (idx >= 0) return m_entity_loaded_callbacks.at(idx);
			return m_entity_loaded_callbacks.emplace(entity, *this, entity);
		}


		e_void SceneManager::allocateCustomMeshes(EntityInstance& r, e_int32 count)
		{
			if (hasCustomMeshes(r) && r.mesh_count == count) return;

			ASSERT(r.entity);
			auto& rm = r.entity->getResourceManager();
			auto* material_manager = static_cast<MaterialManager*>(rm.getOwner().get(RESOURCE_MATERIAL_TYPE));

			auto* new_meshes = (Mesh*)m_allocator.allocate(count * sizeof(Mesh));
			if (r.meshes)
			{
				for (e_int32 i = 0; i < r.mesh_count; ++i)
				{
					_new (new_meshes + i) Mesh(r.meshes[i]);
				}

				if (hasCustomMeshes(r))
				{
					for (e_int32 i = count; i < r.mesh_count; ++i)
					{
						material_manager->unload(*r.meshes[i].material);
					}
					for (e_int32 i = 0; i < r.mesh_count; ++i)
					{
						r.meshes[i].~Mesh();
					}
					m_allocator.deallocate(r.meshes);
				}
				else
				{
					for (e_int32 i = 0; i < r.mesh_count; ++i)
					{
						material_manager->load(*r.meshes[i].material);
					}
				}
			}

			for (e_int32 i = r.mesh_count; i < count; ++i)
			{
				_new (new_meshes + i) Mesh(nullptr, bgfx::VertexDecl(), "", m_allocator);
			}
			r.meshes = new_meshes;
			r.mesh_count = count;
			r.flags |= (e_uint8)EntityInstance::CUSTOM_MESHES;
		}


		e_bool SceneManager::getEntityInstanceKeepSkin(ComponentHandle cmp)
		{
			auto& r = m_entity_instances[cmp.index];
			return keepSkin(r);
		}


		e_void SceneManager::setEntityInstanceKeepSkin(ComponentHandle cmp, e_bool keep)
		{
			auto& r = m_entity_instances[cmp.index];
			if (keep)
			{
				r.flags |= (e_uint8)EntityInstance::KEEP_SKIN;
			}
			else
			{
				r.flags &= ~(e_uint8)EntityInstance::KEEP_SKIN;
			}
		}


		e_void SceneManager::setEntityInstanceMaterial(ComponentHandle cmp, e_int32 index, const ArchivePath& path)
		{
			auto& r = m_entity_instances[cmp.index];
			if (r.meshes && r.mesh_count > index && r.meshes[index].material && path == r.meshes[index].material->getPath()) 
				return;

			auto& rm = r.entity->getResourceManager();
			auto* material_manager = static_cast<MaterialManager*>(rm.getOwner().get(RESOURCE_MATERIAL_TYPE));

			e_int32 new_count = Math::maximum(e_int8(index + 1), r.mesh_count);
			allocateCustomMeshes(r, new_count);
			ASSERT(r.meshes);

			auto* new_material = static_cast<Material*>(material_manager->load(path));
			r.meshes[index].setMaterial(new_material, *r.entity, m_renderer);
		}


		ArchivePath SceneManager::getEntityInstanceMaterial(ComponentHandle cmp, e_int32 index)
		{
			auto& r = m_entity_instances[cmp.index];
			if (!r.meshes) return ArchivePath("");

			return r.meshes[index].material->getPath();
		}


		e_void SceneManager::setEntity(ComponentHandle component, Entity* entity)
		{
			auto& entity_instance = m_entity_instances[component.index];
			ASSERT(entity_instance.game_object.isValid());
			Entity* old_entity = entity_instance.entity;
			e_bool no_change = entity == old_entity && old_entity;
			if (no_change)
			{
				old_entity->getResourceManager().unload(*old_entity);
				return;
			}
			if (old_entity)
			{
				auto& rm = old_entity->getResourceManager();
				auto* material_manager = static_cast<MaterialManager*>(rm.getOwner().get(RESOURCE_MATERIAL_TYPE));
				freeCustomMeshes(entity_instance, material_manager);
				EntityLoadedCallback& callback = getEntityLoadedCallback(old_entity);
				--callback.m_ref_count;
				if (callback.m_ref_count == 0)
				{
					m_entity_loaded_callbacks.erase(old_entity);
				}

				if (old_entity->isReady())
				{
					m_culling_system->removeStatic(component);
				}
				old_entity->getResourceManager().unload(*old_entity);
			}
			entity_instance.entity = entity;
			entity_instance.meshes = nullptr;
			entity_instance.mesh_count = 0;
			_delete(m_allocator, entity_instance.pose);
			entity_instance.pose = nullptr;
			if (entity)
			{
				if (keepSkin(entity_instance)) 
					entity->setKeepSkin();
				EntityLoadedCallback& callback = getEntityLoadedCallback(entity);
				++callback.m_ref_count;

				if (entity->isReady())
				{
					entityLoaded(entity, component);
				}
			}
		}

		IAllocator& SceneManager::getAllocator() { return m_allocator; }


		e_void SceneManager::detectLightInfluencedGeometry(ComponentHandle cmp)
		{
			e_int32 light_idx = m_point_lights_map[cmp];
			Frustum frustum = getPointLightFrustum(light_idx);
			const CullingSystem::Results& results = m_culling_system->cull(frustum, ~0ULL);
			auto& influenced_geometry = m_light_influenced_geometry[light_idx];
			influenced_geometry.clear();
			for (e_int32 i = 0; i < results.size(); ++i)
			{
				const CullingSystem::Subresults& subresult = results[i];
				influenced_geometry.reserve(influenced_geometry.size() + subresult.size());
				for (e_int32 j = 0, c = subresult.size(); j < c; ++j)
				{
					influenced_geometry.push_back(subresult[j]);
				}
			}
		}

		e_float SceneManager::getLightFOV(ComponentHandle cmp)
		{
			return m_point_lights[m_point_lights_map[cmp]].m_fov;
		}


		e_void SceneManager::setLightFOV(ComponentHandle cmp, e_float fov)
		{
			m_point_lights[m_point_lights_map[cmp]].m_fov = fov;
		}


		ComponentHandle SceneManager::createGlobalLight(GameObject entity)
		{
			GlobalLight light;
			light.m_game_object = entity;
			light.m_diffuse_color.set(1, 1, 1);
			light.m_diffuse_intensity = 0;
			light.m_indirect_intensity = 1;
			light.m_fog_color.set(1, 1, 1);
			light.m_fog_density = 0;
			light.m_cascades.set(3, 8, 100, 300);
			light.m_fog_bottom = 0.0f;
			light.m_fog_height = 10.0f;

			ComponentHandle cmp = {entity.index};
			if (m_global_lights.empty()) m_active_global_light_cmp = cmp;

			m_global_lights.insert(entity, light);
			m_com_man.addComponent(entity, COMPONENT_GLOBAL_LIGHT_TYPE, this, cmp);
			return cmp;
		}


		ComponentHandle SceneManager::createPointLight(GameObject entity)
		{
			PointLight& light = m_point_lights.emplace();
			m_light_influenced_geometry.emplace(m_allocator);
			light.m_game_object = entity;
			light.m_diffuse_color.set(1, 1, 1);
			light.m_diffuse_intensity = 1;
			light.m_fov = Math::degreesToRadians(360);
			light.m_specular_color.set(1, 1, 1);
			light.m_specular_intensity = 1;
			light.m_cast_shadows = false;
			light.m_attenuation_param = 2;
			light.m_range = 10;
			ComponentHandle cmp = { entity.index };
			m_point_lights_map.insert(cmp, m_point_lights.size() - 1);

			m_com_man.addComponent(entity, COMPONENT_POINT_LIGHT_TYPE, this, cmp);

			detectLightInfluencedGeometry(cmp);

			return cmp;
		}


		e_void SceneManager::updateDecalInfo(Decal& decal) const
		{
			decal.position = m_com_man.getPosition(decal.game_object);
			decal.radius = decal.scale.length();
			decal.mtx = m_com_man.getMatrix(decal.game_object);
			decal.mtx.setXVector(decal.mtx.getXVector() * decal.scale.x);
			decal.mtx.setYVector(decal.mtx.getYVector() * decal.scale.y);
			decal.mtx.setZVector(decal.mtx.getZVector() * decal.scale.z);
			decal.inv_mtx = decal.mtx;
			decal.inv_mtx.inverse();
		}


		ComponentHandle SceneManager::createDecal(GameObject entity)
		{
			Decal& decal = m_decals.insert(entity);
			decal.material = nullptr;
			decal.game_object = entity;
			decal.scale.set(1, 1, 1);
			updateDecalInfo(decal);

			ComponentHandle cmp = {entity.index};
			m_com_man.addComponent(entity, COMPONENT_DECAL_TYPE, this, cmp);
			return cmp;
		}


		ComponentHandle SceneManager::createEnvironmentProbe(GameObject entity)
		{
			EnvironmentProbe& probe = m_environment_probes.insert(entity);
			auto* texture_manager = m_engine.getResourceManager().get( RESOURCE_TEXTURE_TYPE);
			probe.texture = static_cast<Texture*>(texture_manager->load(ArchivePath("pipelines/pbr/default_probe.dds")));
			probe.texture->setFlag(BGFX_TEXTURE_SRGB, true);
			probe.irradiance = static_cast<Texture*>(texture_manager->load(ArchivePath("pipelines/pbr/default_probe.dds")));
			probe.irradiance->setFlag(BGFX_TEXTURE_SRGB, true);
			probe.radiance = static_cast<Texture*>(texture_manager->load(ArchivePath("pipelines/pbr/default_probe.dds")));
			probe.radiance->setFlag(BGFX_TEXTURE_SRGB, true);
			probe.guid = Math::randGUID();

			ComponentHandle cmp = {entity.index};
			m_com_man.addComponent(entity, COMPONENT_ENVIRONMENT_PROBE_TYPE, this, cmp);
			return cmp;
		}


		ComponentHandle SceneManager::createBoneAttachment(GameObject game_object)
		{
			BoneAttachment& attachment = m_bone_attachments.emplace(game_object);
			attachment.game_object = game_object;
			attachment.parent_game_object = INVALID_GAME_OBJECT;
			attachment.bone_index = -1;

			ComponentHandle cmp = { game_object.index };
			m_com_man.addComponent(game_object, COMPONENT_BONE_ATTACHMENT_TYPE, this, cmp);
			return cmp;
		}


		ComponentHandle SceneManager::createEntityInstance(GameObject game_object)
		{
			while(game_object.index >= m_entity_instances.size())
			{
				auto& r = m_entity_instances.emplace();
				r.game_object = INVALID_GAME_OBJECT;
				r.entity = nullptr;
				r.pose = nullptr;
			}
			auto& r = m_entity_instances[game_object.index];
			r.game_object = game_object;
			r.entity = nullptr;
			r.meshes = nullptr;
			r.pose = nullptr;
			r.flags = 0;
			r.mesh_count = 0;
			r.matrix = m_com_man.getMatrix(game_object);
			ComponentHandle cmp = { game_object.index};
			m_com_man.addComponent(game_object, COMPONENT_ENTITY_INSTANCE_TYPE, this, cmp);
			return cmp;
		}

	ComponentHandle SceneManager::createComponent(ComponentType type, GameObject entity)
	{
		for (auto& i : COMPONENT_INFOS)
		{
			if (i.type == type)
			{
				return (this->*i.creator)(entity);
			}
		}

		return INVALID_COMPONENT;
	}


	e_void SceneManager::destroyComponent(ComponentHandle component, ComponentType type)
	{
		for (auto& i : COMPONENT_INFOS)
		{
			if (i.type == type)
			{
				(this->*i.destroyer)(component);
				return;
			}
		}
		ASSERT(false);
	}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
	SceneManager* SceneManager::createInstance(Renderer& renderer,
		EngineRoot& engine,
		ComponentManager& com_man,
		IAllocator& allocator)
	{
		return _aligned_new(allocator, SceneManager)(renderer, engine, com_man, allocator);
	}

	e_void SceneManager::destroyInstance(SceneManager* scene)
	{
		_delete(scene->getAllocator(), static_cast<SceneManager*>(scene));
	}
}
