#ifndef _render_scene_h_
#define _render_scene_h_
#pragma once
#include "common/egal-d.h"
#include "common/resource/entity_manager.h"
#include "runtime/EngineFramework/plugin_manager.h"
#include "runtime/EngineFramework/component_manager.h"
#include "runtime/EngineFramework/engine_root.h"

#include "runtime/EngineFramework/camera.h"

struct lua_State;
namespace egal
{
	struct RayCastEntityHit;
	struct Mesh;
	class Material;
	class MaterialManager;
	class Texture;
	class Shader;
	class CullingSystem;



	enum class RenderSceneVersion : e_int32
	{
		GRASS_ROTATION_MODE,
		GLOBAL_LIGHT_REFACTOR,
		EMITTER_MATERIAL,
		BONE_ATTACHMENT_TRANSFORM,
		MODEL_INSTNACE_FLAGS,
		INDIRECT_INTENSITY,
		SCRIPTED_PARTICLES,
		POINT_LIGHT_NO_COMPONENT,

		LATEST
	};

	struct DecalInfo
	{
		float4x4	mtx;
		float4x4	inv_mtx;
		Material*	material;
		float3		position;
		e_float		radius;
	};

	struct Decal : public DecalInfo
	{
		GameObject	game_object;
		float3		scale;
	};

	struct PointLight
	{
		float3		m_diffuse_color;
		float3		m_specular_color;
		e_float		m_diffuse_intensity;
		e_float		m_specular_intensity;
		GameObject	m_game_object;
		e_float		m_fov;
		e_float		m_attenuation_param;
		e_float		m_range;
		e_bool		m_cast_shadows;
	};

	struct GlobalLight
	{
		float3		m_diffuse_color;
		e_float		m_diffuse_intensity;
		e_float		m_indirect_intensity;
		float3		m_fog_color;
		e_float		m_fog_density;
		e_float		m_fog_bottom;
		e_float		m_fog_height;
		GameObject	m_game_object;
		float4		m_cascades;
	};



	struct EnvironmentProbe
	{
		Texture* texture;
		Texture* irradiance;
		Texture* radiance;
		e_uint64 guid;
	};

	struct BoneAttachment
	{
		GameObject		game_object;
		GameObject		parent_game_object;
		e_int32			bone_index;
		RigidTransform	relative_transform;
	};

	struct TerrainInfo
	{
		float4x4	m_world_matrix;
		Shader*		m_shader;
		//Terrain* m_terrain;
		float3		m_morph_const;
		e_float		m_size;
		float3		m_min;
		e_int32		m_index;
	};

	struct EntityInstance
	{
		enum Flags : e_uint8
		{
			CUSTOM_MESHES = 1 << 0,
			KEEP_SKIN = 1 << 1,
			IS_BONE_ATTACHMENT_PARENT = 1 << 2,

			RUNTIME_FLAGS = CUSTOM_MESHES,
			PERSISTENT_FLAGS = e_uint8(~RUNTIME_FLAGS)
		};

		float4x4	matrix;
		Entity*		entity;
		Pose*		pose;
		GameObject	game_object;
		Mesh*		meshes;
		e_uint8		flags;
		e_int8		mesh_count;
	};

	struct EntityInstanceMesh
	{
		ComponentHandle entity_instance;
		Mesh*			mesh;
		e_float			depth;
	};

	struct GrassInfo
	{
		struct InstanceData
		{
			float4x4	matrix;
			float4		normal;
		};
		Entity*				entity;
		const InstanceData* instance_data;
		e_int32				instance_count;
		e_float				type_distance;
	};

	struct DebugTriangle
	{
		float3		p0;
		float3		p1;
		float3		p2;
		e_uint32	color;
		e_float		life;
	};

	struct DebugLine
	{
		float3		from;
		float3		to;
		e_uint32	color;
		e_float		life;
	};


	struct DebugPoint
	{
		float3	 pos;
		e_uint32 color;
		e_float  life;
	};

	class SceneManager : public Singleton<SceneManager>
	{
	public:
		static SceneManager* createInstance(Renderer& renderer,
			EngineRoot& engine,
			ComponentManager& com_man,
			IAllocator& allocator);
		static e_void destroyInstance(SceneManager* scene);
	private:
		struct EntityLoadedCallback
		{
			EntityLoadedCallback(SceneManager& scene, Entity* entity)
				: m_scene(scene)
				, m_ref_count(0)
				, m_entity(entity)
			{
				m_entity->getObserverCb().bind<SceneManager, &SceneManager::entityStateChanged>(&scene);
			}

			~EntityLoadedCallback()
			{
				m_entity->getObserverCb().unbind<SceneManager, &SceneManager::entityStateChanged>(&m_scene);
			}

			Entity*				m_entity;
			e_int32				m_ref_count;
			SceneManager&		m_scene;
		};

	public:
		SceneManager(Renderer& renderer, EngineRoot& engine, ComponentManager& com_man, IAllocator& allocator);
		~SceneManager();

		ComponentHandle createComponent(ComponentType, GameObject);
		void destroyComponent(ComponentHandle component, ComponentType type);

		ComponentHandle createCamera(GameObject game_object);

		void serialize(WriteBinary& serializer);
		void serialize(ISerializer& serializer) {}
		e_void deserializeCameras(ReadBinary& serializer);
		e_void deserializeEntityInstances(ReadBinary& serializer);
		e_void deserializeLights(ReadBinary& serializer);
		e_void deserializeTerrains(ReadBinary& serializer);
		void deserialize(IDeserializer& serializer) {}
		void deserialize(ReadBinary& serializer);

		e_void destroyBoneAttachment(ComponentHandle component);
		e_void destroyEnvironmentProbe(ComponentHandle component);
		e_void destroyEntityInstance(ComponentHandle component);
		e_void destroyGlobalLight(ComponentHandle component);
		e_void destroyDecal(ComponentHandle component);
		e_void destroyPointLight(ComponentHandle component);
		e_void destroyCamera(ComponentHandle component);
		e_void destroyTerrain(ComponentHandle component);
		e_void destroyParticleEmitter(ComponentHandle component);

		e_void frame(e_float time_delta, e_bool paused);
		e_void lateUpdate(e_float time_delta, e_bool paused);

		e_void serializeEntityInstance(ISerializer& serialize, ComponentHandle cmp);
		e_void deserializeEntityInstance(IDeserializer& serializer, GameObject _game_object, e_int32 scene_version);
		e_void serializeGlobalLight(ISerializer& serializer, ComponentHandle cmp);
		e_void deserializeGlobalLight(IDeserializer& serializer, GameObject _game_object, e_int32 scene_version);
		e_void serializePointLight(ISerializer& serializer, ComponentHandle cmp);
		e_void deserializePointLight(IDeserializer& serializer, GameObject entity, e_int32 scene_version);
		e_void serializeDecal(ISerializer& serializer, ComponentHandle cmp);
		e_void deserializeDecal(IDeserializer& serializer, GameObject entity, e_int32 /*scene_version*/);
		e_void serializeCamera(ISerializer& serialize, ComponentHandle cmp);
		e_void deserializeCamera(IDeserializer& serializer, GameObject game_object, e_int32 /*scene_version*/);
		e_void serializeBoneAttachment(ISerializer& serializer, ComponentHandle cmp);
		e_void deserializeBoneAttachment(IDeserializer& serializer, GameObject game_object, e_int32 /*scene_version*/);
		e_void serializeTerrain(ISerializer& serializer, ComponentHandle cmp);
		e_void deserializeTerrain(IDeserializer& serializer, GameObject entity, e_int32 version);
		e_void serializeEnvironmentProbe(ISerializer& serializer, ComponentHandle cmp);


		ComponentHandle getComponent(GameObject entity, ComponentType type);
		ComponentManager& getComponentManager();

		void startGame();
		void stopGame();
		int getVersion() const;
		e_void deserializeEnvironmentProbe(IDeserializer& serializer, GameObject entity, e_int32 /*scene_version*/);
		e_void serializeBoneAttachments(WriteBinary& serializer);
		e_void serializeCameras(WriteBinary& serializer);
		e_void serializeLights(WriteBinary& serializer);
		e_void serializeEntityInstances(WriteBinary& serializer);
		e_void serializeTerrains(WriteBinary& serializer);
		e_void deserializeDecals(ReadBinary& serializer);
		e_void serializeDecals(WriteBinary& serializer);
		e_void serializeEnvironmentProbes(WriteBinary& serializer);
		e_void deserializeEnvironmentProbes(ReadBinary& serializer);
		e_void deserializeBoneAttachments(ReadBinary& serializer);
		void clear();

		RayCastEntityHit castRay(const float3& origin, const float3& dir, ComponentHandle ignore);
		RayCastEntityHit castRayTerrain(ComponentHandle terrain, const float3& origin, const float3& dir);
		e_void getRay(ComponentHandle camera, const float2& screen_pos, float3& origin, float3& dir);

		Frustum getCameraFrustum(ComponentHandle camera) const;
		Frustum getCameraFrustum(ComponentHandle camera, const float2& a, const float2& b) const;

		e_void camera_navigate(GameObject cmp, e_float forward, e_float right, e_float up, e_float speed);
		e_void camera_rotate(GameObject cmp, e_float x, e_float y);

		e_void updateBoneAttachment(const BoneAttachment& bone_attachment);
		e_float getTime() const;

		e_void entityUnloaded(Entity*, ComponentHandle component);
		e_void freeCustomMeshes(EntityInstance& r, MaterialManager* manager);
		e_void entityLoaded(Entity* entity, ComponentHandle component);
		e_void EntityUnloaded(Entity* entity);
		e_void EntityLoaded(Entity* entity);
		EntityLoadedCallback& getEntityLoadedCallback(Entity* entity);
		e_void allocateCustomMeshes(EntityInstance& r, e_int32 count);
		IPlugin& getPlugin() const;
		Renderer& getRenderer();
		EngineRoot& getEngine() const;
		IAllocator& getAllocator();

		e_void detectLightInfluencedGeometry(ComponentHandle cmp);
		Pose* lockPose(ComponentHandle cmp);
		e_void unlockPose(ComponentHandle cmp, e_bool changed);

		ComponentHandle getActiveGlobalLight();
		e_void setActiveGlobalLight(ComponentHandle cmp);

		float4 getShadowmapCascades(ComponentHandle cmp);
		e_void setShadowmapCascades(ComponentHandle cmp, const float4& value);

		e_void addDebugHalfSphere(const float4x4& transform, e_float radius, e_bool top, e_uint32 color, e_float life);
		e_void addDebugHalfSphere(const float3& center, e_float radius, e_bool top, e_uint32 color, e_float life);
		e_void addDebugTriangle(const float3& p0,
			const float3& p1,
			const float3& p2,
			e_uint32 color,
			e_float life);
		e_void addDebugPoint(const float3& pos, e_uint32 color, e_float life);
		e_void addDebugCone(const float3& vertex,
			const float3& dir,
			const float3& axis0,
			const float3& axis1,
			e_uint32 color,
			e_float life);

		e_void addDebugLine(const float3& from, const float3& to, e_uint32 color, e_float life);
		e_void addDebugCross(const float3& center, e_float size, e_uint32 color, e_float life);
		e_void addDebugCube(const float3& pos,
			const float3& dir,
			const float3& up,
			const float3& right,
			e_uint32 color,
			e_float life);
		e_void addDebugCube(const float3& from, const float3& max, e_uint32 color, e_float life);
		e_void addDebugCubeSolid(const float3& from, const float3& max, e_uint32 color, e_float life);
		e_void addDebugCircle(const float3& center,
			const float3& up,
			e_float radius,
			e_uint32 color,
			e_float life);
		e_void addDebugSphere(const float3& center, e_float radius, e_uint32 color, e_float life);
		e_void addDebugFrustum(const Frustum& frustum, e_uint32 color, e_float life);

		e_void addDebugCapsule(const float3& position,
			e_float height,
			e_float radius,
			e_uint32 color,
			e_float life);

		e_void addDebugCapsule(const float4x4& transform,
			e_float height,
			e_float radius,
			e_uint32 color,
			e_float life);

		e_void addDebugCylinder(const float3& position,
			const float3& up,
			e_float radius,
			e_uint32 color,
			e_float life);

		GameObject getBoneAttachmentParent(ComponentHandle cmp);
		e_void updateRelativeMatrix(BoneAttachment& attachment);
		e_void setBoneAttachmentParent(ComponentHandle cmp, GameObject entity);
		e_void setBoneAttachmentBone(ComponentHandle cmp, e_int32 value);
		e_int32 getBoneAttachmentBone(ComponentHandle cmp);
		float3 getBoneAttachmentPosition(ComponentHandle cmp);
		e_void setBoneAttachmentPosition(ComponentHandle cmp, const float3& pos);
		float3 getBoneAttachmentRotation(ComponentHandle cmp);
		e_void setBoneAttachmentRotation(ComponentHandle cmp, const float3& rot);
		e_void setBoneAttachmentRotationQuat(ComponentHandle cmp, const Quaternion& rot);

		const TArrary<DebugTriangle>& getDebugTriangles() const;
		const TArrary<DebugLine>& getDebugLines() const;
		const TArrary<DebugPoint>& getDebugPoints() const;
		e_void setGlobalLODMultiplier(e_float multiplier);
		e_float getGlobalLODMultiplier() const;

		float4x4 getCameraProjection(ComponentHandle camera);
		float4x4 getCameraViewProjection(ComponentHandle camera);
		GameObject getCameraGameObject(ComponentHandle camera) const;
		ComponentHandle getCameraInSlot(const e_char* slot);
		e_float getCameraFOV(ComponentHandle camera);
		e_void setCameraFOV(ComponentHandle camera, e_float fov);
		e_void setCameraFarPlane(ComponentHandle camera, e_float far_flip);
		e_void setCameraNearPlane(ComponentHandle camera, e_float near);
		e_float getCameraFarPlane(ComponentHandle camera);
		e_float getCameraNearPlane(ComponentHandle camera);
		e_float getCameraScreenWidth(ComponentHandle camera);
		e_float getCameraScreenHeight(ComponentHandle camera);
		e_void setCameraSlot(ComponentHandle camera, const e_char* slot);
		ComponentHandle getCameraComponent(GameObject entity);
		const e_char* getCameraSlot(ComponentHandle camera);
		e_void setCameraScreenSize(ComponentHandle camera, e_int32 w, e_int32 h);
		e_bool isCameraOrtho(ComponentHandle camera);
		e_void setCameraOrtho(ComponentHandle camera, e_bool is_ortho);
		e_float getCameraOrthoSize(ComponentHandle camera);
		e_void setCameraOrthoSize(ComponentHandle camera, e_float value);
		float2 getCameraScreenSize(ComponentHandle camera);

		e_void showEntityInstance(ComponentHandle cmp);
		e_void hideEntityInstance(ComponentHandle cmp);
		ComponentHandle getEntityInstanceComponent(GameObject entity);
		float3 getPoseBonePosition(ComponentHandle entity_instance, e_int32 bone_index);
		Frustum getPointLightFrustum(e_int32 light_idx) const;
		e_void onEntityDestroyed(GameObject entity);
		e_void onEntityMoved(GameObject game_object);
		EntityInstance* getEntityInstance(ComponentHandle cmp);
		EntityInstance* getEntityInstances();
		e_bool getEntityInstanceKeepSkin(ComponentHandle cmp);
		e_void setEntityInstanceKeepSkin(ComponentHandle cmp, e_bool keep);
		ArchivePath getEntityInstancePath(ComponentHandle cmp);
		e_void setEntityInstanceMaterial(ComponentHandle cmp, e_int32 index, const ArchivePath& path);
		ArchivePath getEntityInstanceMaterial(ComponentHandle cmp, e_int32 index);
		e_void setEntity(ComponentHandle component, Entity* entity);
		e_int32 getEntityInstanceMaterialsCount(ComponentHandle cmp);
		e_void setEntityInstancePath(ComponentHandle cmp, const ArchivePath& path);
		e_void setTerrainHeightAt(ComponentHandle cmp, e_int32 x, e_int32 z, e_float height);
		TArrary<TArrary<EntityInstanceMesh>>& getEntityInstanceInfos(const Frustum& frustum, const float3& lod_ref_point, ComponentHandle camera, e_uint64 layer_mask);
		e_void getEntityInstanceGameObjects(const Frustum& frustum, TArrary<GameObject>& entities);
		e_float getCameraLODMultiplier(ComponentHandle camera);
		GameObject getEntityInstanceGameObject(ComponentHandle cmp);
		ComponentHandle getFirstEntityInstance();
		ComponentHandle getNextEntityInstance(ComponentHandle cmp);
		Entity* getEntityInstanceEntity(ComponentHandle cmp);

		e_void setDecalMaterialPath(ComponentHandle cmp, const ArchivePath& path);
		ArchivePath getDecalMaterialPath(ComponentHandle cmp);
		e_void setDecalScale(ComponentHandle cmp, const float3& value);
		float3 getDecalScale(ComponentHandle cmp);
		e_void getDecals(const Frustum& frustum, TArrary<DecalInfo>& decals);
		e_void updateDecalInfo(Decal& decal) const;
		ComponentHandle createDecal(GameObject entity);
		ComponentHandle createEnvironmentProbe(GameObject entity);

		ComponentHandle createBoneAttachment(GameObject game_object);
		ComponentHandle createEntityInstance(GameObject game_object);

		e_int32 getClosestPointLights(const float3& pos, ComponentHandle* lights, e_int32 max_lights);
		e_void getPointLights(const Frustum& frustum, TArrary<ComponentHandle>& lights);
		e_void getPointLightInfluencedGeometry(ComponentHandle light_cmp, TArrary<EntityInstanceMesh>& infos);
		e_void getPointLightInfluencedGeometry(ComponentHandle light_cmp, const Frustum& frustum, TArrary<EntityInstanceMesh>& infos);
		e_void setLightCastShadows(ComponentHandle cmp, e_bool cast_shadows);
		e_bool getLightCastShadows(ComponentHandle cmp);
		e_float getLightAttenuation(ComponentHandle cmp);
		e_void setLightAttenuation(ComponentHandle cmp, e_float attenuation);
		e_float getLightFOV(ComponentHandle cmp);
		e_void setLightFOV(ComponentHandle cmp, e_float fov);
		ComponentHandle createGlobalLight(GameObject entity);
		ComponentHandle createPointLight(GameObject entity);

		e_float getLightRange(ComponentHandle cmp);
		e_void setLightRange(ComponentHandle cmp, e_float value);
		e_void setPointLightIntensity(ComponentHandle cmp, e_float intensity);
		e_void setGlobalLightIntensity(ComponentHandle cmp, e_float intensity);
		e_void setGlobalLightIndirectIntensity(ComponentHandle cmp, e_float intensity);
		e_void setPointLightColor(ComponentHandle cmp, const float3& color);
		e_void setGlobalLightColor(ComponentHandle cmp, const float3& color);

		e_float getPointLightIntensity(ComponentHandle cmp);
		GameObject getPointLightGameObject(ComponentHandle cmp) const;
		GameObject getGlobalLightGameObject(ComponentHandle cmp) const;
		e_float getGlobalLightIntensity(ComponentHandle cmp);
		e_float getGlobalLightIndirectIntensity(ComponentHandle cmp);
		float3 getPointLightColor(ComponentHandle cmp);
		float3 getGlobalLightColor(ComponentHandle cmp);

		float3 getPointLightSpecularColor(ComponentHandle cmp);
		e_void setPointLightSpecularColor(ComponentHandle cmp, const float3& color);
		e_float getPointLightSpecularIntensity(ComponentHandle cmp);
		e_void setPointLightSpecularIntensity(ComponentHandle cmp, e_float color);

		e_void setFogDensity(ComponentHandle cmp, e_float density);
		e_void setFogColor(ComponentHandle cmp, const float3& color);
		e_float getFogDensity(ComponentHandle cmp);
		e_float getFogBottom(ComponentHandle cmp);
		e_float getFogHeight(ComponentHandle cmp);
		e_void setFogBottom(ComponentHandle cmp, e_float value);
		e_void setFogHeight(ComponentHandle cmp, e_float value);
		float3 getFogColor(ComponentHandle cmp);

		Texture* getEnvironmentProbeTexture(ComponentHandle cmp) const;
		Texture* getEnvironmentProbeIrradiance(ComponentHandle cmp) const;
		Texture* getEnvironmentProbeRadiance(ComponentHandle cmp) const;
		e_void reloadEnvironmentProbe(ComponentHandle cmp);
		ComponentHandle getNearestEnvironmentProbe(const float3& pos) const;
		e_uint64 getEnvironmentProbeGUID(ComponentHandle cmp) const;
		e_void entityStateChanged(Resource::State old_state, Resource::State new_state, Resource& resource);
	private:
		IAllocator&					m_allocator;
		ComponentManager&			m_com_man;
		Renderer&					m_renderer;
		EngineRoot&					m_engine;
		CullingSystem*				m_culling_system;

		TArrary<TArrary<ComponentHandle>>	m_light_influenced_geometry;
		ComponentHandle						m_active_global_light_cmp;
		THashMap<ComponentHandle, e_int32>	m_point_lights_map;

		TArraryMap<GameObject, Decal>				m_decals;
		TArrary<EntityInstance>						m_entity_instances;

		THashMap<GameObject, GlobalLight>			m_global_lights;
		TArrary<PointLight>							m_point_lights;
		THashMap<GameObject, Camera>				m_cameras;
		TArraryMap<GameObject, BoneAttachment>		m_bone_attachments;
		TArraryMap<GameObject, EnvironmentProbe>	m_environment_probes;


		TArraryMap<Entity*, EntityLoadedCallback>		m_entity_loaded_callbacks;
		TArrary<TArrary<EntityInstanceMesh>>			m_temporary_infos;

		TArrary<DebugTriangle>	m_debug_triangles;
		TArrary<DebugLine>		m_debug_lines;
		TArrary<DebugPoint>		m_debug_points;

		e_float m_time;
		e_float m_lod_multiplier;
		e_bool	m_is_updating_attachments;
		e_bool	m_is_grass_enabled;
		e_bool	m_is_game_running;

		float2  m_mouse_sensitivity;
		float2  m_mouse_last;
		float2  m_mouse_now;
		e_float m_horizontalAngle;
		e_float m_verticalAngle;


	};
}

#endif
