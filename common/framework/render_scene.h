#ifndef _render_scene_h_
#define _render_scene_h_
#pragma once
#include "common/egal-d.h"

struct lua_State;
namespace egal
{
	struct RayCastModelHit;
	struct Mesh;
	struct Material;
	class Universe;
	class Texture;

	struct TerrainInfo
	{
		float4x4 m_world_matrix;
		Shader* m_shader;
		//Terrain* m_terrain;
		float3 m_morph_const;
		e_float m_size;
		float3 m_min;
		e_int32 m_index;
	};


	struct DecalInfo
	{
		float4x4 mtx;
		float4x4 inv_mtx;
		Material* material;
		float3 position;
		e_float radius;
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

		float4x4 float4x4;
		Entity* model;
		Pose* pose;
		GameObject entity;
		Mesh* meshes;
		e_uint8 flags;
		e_int8 mesh_count;
	};


	struct EntityInstanceMesh
	{
		ComponentHandle model_instance;
		Mesh* mesh;
		e_float depth;
	};


	struct GrassInfo
	{
		struct InstanceData
		{
			float4x4 float4x4;
			float4 normal;
		};
		Entity* model;
		const InstanceData* instance_data;
		e_int32 instance_count;
		e_float type_distance;
	};


	struct DebugTriangle
	{
		float3 p0;
		float3 p1;
		float3 p2;
		e_uint32 color;
		e_float life;
	};


	struct DebugLine
	{
		float3 from;
		float3 to;
		e_uint32 color;
		e_float life;
	};


	struct DebugPoint
	{
		float3 pos;
		e_uint32 color;
		e_float life;
	};


	class RenderScene
	{
	public:
		static RenderScene* createInstance(Renderer& renderer,
			Engine& engine,
			Universe& universe,
			IAllocator& allocator);
		static e_void destroyInstance(RenderScene* scene);
		static e_void registerLuaAPI(lua_State* L);

		virtual RayCastModelHit castRay(const float3& origin, const float3& dir, ComponentHandle ignore) = 0;
		virtual RayCastModelHit castRayTerrain(ComponentHandle terrain, const float3& origin, const float3& dir) = 0;
		virtual e_void getRay(ComponentHandle camera, const float2& screen_pos, float3& origin, float3& dir) = 0;

		virtual Frustum getCameraFrustum(ComponentHandle camera) const = 0;
		virtual Frustum getCameraFrustum(ComponentHandle camera, const float2& a, const float2& b) const = 0;
		virtual e_float getTime() const = 0;
		virtual Engine& getEngine() const = 0;
		virtual IAllocator& getAllocator() = 0;

		virtual Pose* lockPose(ComponentHandle cmp) = 0;
		virtual e_void unlockPose(ComponentHandle cmp, e_bool changed) = 0;
		virtual ComponentHandle getActiveGlobalLight() = 0;
		virtual e_void setActiveGlobalLight(ComponentHandle cmp) = 0;
		virtual float4 getShadowmapCascades(ComponentHandle cmp) = 0;
		virtual e_void setShadowmapCascades(ComponentHandle cmp, const float4& value) = 0;

		virtual e_void addDebugTriangle(const float3& p0,
			const float3& p1,
			const float3& p2,
			e_uint32 color,
			e_float life) = 0;
		virtual e_void addDebugPoint(const float3& pos, e_uint32 color, e_float life) = 0;
		virtual e_void addDebugCone(const float3& vertex,
			const float3& dir,
			const float3& axis0,
			const float3& axis1,
			e_uint32 color,
			e_float life) = 0;

		virtual e_void addDebugLine(const float3& from, const float3& to, e_uint32 color, e_float life) = 0;
		virtual e_void addDebugCross(const float3& center, e_float size, e_uint32 color, e_float life) = 0;
		virtual e_void addDebugCube(const float3& pos,
			const float3& dir,
			const float3& up,
			const float3& right,
			e_uint32 color,
			e_float life) = 0;
		virtual e_void addDebugCube(const float3& from, const float3& max, e_uint32 color, e_float life) = 0;
		virtual e_void addDebugCubeSolid(const float3& from, const float3& max, e_uint32 color, e_float life) = 0;
		virtual e_void addDebugCircle(const float3& center,
			const float3& up,
			e_float radius,
			e_uint32 color,
			e_float life) = 0;
		virtual e_void addDebugSphere(const float3& center, e_float radius, e_uint32 color, e_float life) = 0;
		virtual e_void addDebugFrustum(const Frustum& frustum, e_uint32 color, e_float life) = 0;

		virtual e_void addDebugCapsule(const float3& position,
			e_float height,
			e_float radius,
			e_uint32 color,
			e_float life) = 0;

		virtual e_void addDebugCapsule(const float4x4& transform,
			e_float height,
			e_float radius,
			e_uint32 color,
			e_float life) = 0;

		virtual e_void addDebugCylinder(const float3& position,
			const float3& up,
			e_float radius,
			e_uint32 color,
			e_float life) = 0;

		virtual GameObject getBoneAttachmentParent(ComponentHandle cmp) = 0;
		virtual e_void setBoneAttachmentParent(ComponentHandle cmp, GameObject entity) = 0;
		virtual e_void setBoneAttachmentBone(ComponentHandle cmp, e_int32 value) = 0;
		virtual e_int32 getBoneAttachmentBone(ComponentHandle cmp) = 0;
		virtual float3 getBoneAttachmentPosition(ComponentHandle cmp) = 0;
		virtual e_void setBoneAttachmentPosition(ComponentHandle cmp, const float3& pos) = 0;
		virtual float3 getBoneAttachmentRotation(ComponentHandle cmp) = 0;
		virtual e_void setBoneAttachmentRotation(ComponentHandle cmp, const float3& rot) = 0;
		virtual e_void setBoneAttachmentRotationQuat(ComponentHandle cmp, const Quaternion& rot) = 0;

		virtual const TVector<DebugTriangle>& getDebugTriangles() const = 0;
		virtual const TVector<DebugLine>& getDebugLines() const = 0;
		virtual const TVector<DebugPoint>& getDebugPoints() const = 0;

		virtual float4x4 getCameraProjection(ComponentHandle camera) = 0;
		virtual float4x4 getCameraViewProjection(ComponentHandle camera) = 0;
		virtual GameObject getCameraEntity(ComponentHandle camera) const = 0;
		virtual ComponentHandle getCameraInSlot(const e_char* slot) = 0;
		virtual e_float getCameraFOV(ComponentHandle camera) = 0;
		virtual e_void setCameraFOV(ComponentHandle camera, e_float fov) = 0;
		virtual e_void setCameraFarPlane(ComponentHandle camera, e_float far) = 0;
		virtual e_void setCameraNearPlane(ComponentHandle camera, e_float near) = 0;
		virtual e_float getCameraFarPlane(ComponentHandle camera) = 0;
		virtual e_float getCameraNearPlane(ComponentHandle camera) = 0;
		virtual e_float getCameraScreenWidth(ComponentHandle camera) = 0;
		virtual e_float getCameraScreenHeight(ComponentHandle camera) = 0;
		virtual e_void setCameraSlot(ComponentHandle camera, const e_char* slot) = 0;
		virtual const e_char* getCameraSlot(ComponentHandle camera) = 0;
		virtual e_void setCameraScreenSize(ComponentHandle camera, e_int32 w, e_int32 h) = 0;
		virtual e_bool isCameraOrtho(ComponentHandle camera) = 0;
		virtual e_void setCameraOrtho(ComponentHandle camera, e_bool is_ortho) = 0;
		virtual e_float getCameraOrthoSize(ComponentHandle camera) = 0;
		virtual e_void setCameraOrthoSize(ComponentHandle camera, e_float value) = 0;
		virtual float2 getCameraScreenSize(ComponentHandle camera) = 0;

		virtual e_void showModelInstance(ComponentHandle cmp) = 0;
		virtual e_void hideModelInstance(ComponentHandle cmp) = 0;
		virtual ComponentHandle getModelInstanceComponent(GameObject entity) = 0;
		virtual EntityInstance* getModelInstance(ComponentHandle cmp) = 0;
		virtual EntityInstance* getModelInstances() = 0;
		virtual e_bool getModelInstanceKeepSkin(ComponentHandle cmp) = 0;
		virtual e_void setModelInstanceKeepSkin(ComponentHandle cmp, e_bool keep) = 0;
		virtual ArchivePath getModelInstancePath(ComponentHandle cmp) = 0;
		virtual e_void setModelInstanceMaterial(ComponentHandle cmp, e_int32 index, const ArchivePath& path) = 0;
		virtual ArchivePath getModelInstanceMaterial(ComponentHandle cmp, e_int32 index) = 0;
		virtual e_int32 getModelInstanceMaterialsCount(ComponentHandle cmp) = 0;
		virtual e_void setModelInstancePath(ComponentHandle cmp, const ArchivePath& path) = 0;
		virtual TVector<TVector<EntityInstanceMesh>>& getModelInstanceInfos(const Frustum& frustum, const float3& lod_ref_point,	ComponentHandle camera,	e_uint64 layer_mask) = 0;
		virtual e_void getModelInstanceEntities(const Frustum& frustum, TVector<GameObject>& entities) = 0;
		virtual GameObject getModelInstanceEntity(ComponentHandle cmp) = 0;
		virtual ComponentHandle getFirstModelInstance() = 0;
		virtual ComponentHandle getNextModelInstance(ComponentHandle cmp) = 0;
		virtual Entity* getModelInstanceModel(ComponentHandle cmp) = 0;

		virtual e_void setDecalMaterialPath(ComponentHandle cmp, const ArchivePath& path) = 0;
		virtual ArchivePath getDecalMaterialPath(ComponentHandle cmp) = 0;
		virtual e_void setDecalScale(ComponentHandle cmp, const float3& value) = 0;
		virtual float3 getDecalScale(ComponentHandle cmp) = 0;
		virtual e_void getDecals(const Frustum& frustum, TVector<DecalInfo>& decals) = 0;

		virtual e_void getGrassInfos(const Frustum& frustum, ComponentHandle camera, TVector<GrassInfo>& infos) = 0;
		virtual e_void forceGrassUpdate(ComponentHandle cmp) = 0;
		virtual e_void getTerrainInfos(const Frustum& frustum, const float3& lod_ref_point, TVector<TerrainInfo>& infos) = 0;
		virtual e_float getTerrainHeightAt(ComponentHandle cmp, e_float x, e_float z) = 0;
		virtual float3 getTerrainNormalAt(ComponentHandle cmp, e_float x, e_float z) = 0;
		virtual e_void setTerrainMaterialPath(ComponentHandle cmp, const ArchivePath& path) = 0;
		virtual ArchivePath getTerrainMaterialPath(ComponentHandle cmp) = 0;
		virtual Material* getTerrainMaterial(ComponentHandle cmp) = 0;
		virtual e_void setTerrainXZScale(ComponentHandle cmp, e_float scale) = 0;
		virtual e_float getTerrainXZScale(ComponentHandle cmp) = 0;
		virtual e_void setTerrainYScale(ComponentHandle cmp, e_float scale) = 0;
		virtual e_float getTerrainYScale(ComponentHandle cmp) = 0;
		virtual float2 getTerrainSize(ComponentHandle cmp) = 0;
		virtual AABB getTerrainAABB(ComponentHandle cmp) = 0;
		virtual ComponentHandle getTerrainComponent(GameObject entity) = 0;
		virtual GameObject getTerrainEntity(ComponentHandle cmp) = 0;
		virtual float2 getTerrainResolution(ComponentHandle cmp) = 0;
		virtual ComponentHandle getFirstTerrain() = 0;
		virtual ComponentHandle getNextTerrain(ComponentHandle cmp) = 0;

		virtual e_bool isGrassEnabled() const = 0;
		virtual e_int32 getGrassRotationMode(ComponentHandle cmp, e_int32 index) = 0;
		virtual e_void setGrassRotationMode(ComponentHandle cmp, e_int32 index, e_int32 value) = 0;
		virtual e_float getGrassDistance(ComponentHandle cmp, e_int32 index) = 0;
		virtual e_void setGrassDistance(ComponentHandle cmp, e_int32 index, e_float value) = 0;
		virtual e_void enableGrass(e_bool enabled) = 0;
		virtual e_void setGrassPath(ComponentHandle cmp, e_int32 index, const ArchivePath& path) = 0;
		virtual ArchivePath getGrassPath(ComponentHandle cmp, e_int32 index) = 0;
		virtual e_void setGrassDensity(ComponentHandle cmp, e_int32 index, e_int32 density) = 0;
		virtual e_int32 getGrassDensity(ComponentHandle cmp, e_int32 index) = 0;
		virtual e_int32 getGrassCount(ComponentHandle cmp) = 0;
		virtual e_void addGrass(ComponentHandle cmp, e_int32 index) = 0;
		virtual e_void removeGrass(ComponentHandle cmp, e_int32 index) = 0;

		virtual e_int32 getClosestPointLights(const float3& pos, ComponentHandle* lights, e_int32 max_lights) = 0;
		virtual e_void getPointLights(const Frustum& frustum, TVector<ComponentHandle>& lights) = 0;
		virtual e_void getPointLightInfluencedGeometry(ComponentHandle light_cmp, TVector<EntityInstanceMesh>& infos) = 0;
		virtual e_void getPointLightInfluencedGeometry(ComponentHandle light_cmp, const Frustum& frustum, TVector<EntityInstanceMesh>& infos) = 0;
		virtual e_void setLightCastShadows(ComponentHandle cmp, e_bool cast_shadows) = 0;
		virtual e_bool getLightCastShadows(ComponentHandle cmp) = 0;
		virtual e_float getLightAttenuation(ComponentHandle cmp) = 0;
		virtual e_void setLightAttenuation(ComponentHandle cmp, e_float attenuation) = 0;
		virtual e_float getLightFOV(ComponentHandle cmp) = 0;
		virtual e_void setLightFOV(ComponentHandle cmp, e_float fov) = 0;
		virtual e_float getLightRange(ComponentHandle cmp) = 0;
		virtual e_void setLightRange(ComponentHandle cmp, e_float value) = 0;
		virtual e_void setPointLightIntensity(ComponentHandle cmp, e_float intensity) = 0;
		virtual e_void setGlobalLightIntensity(ComponentHandle cmp, e_float intensity) = 0;
		virtual e_void setGlobalLightIndirectIntensity(ComponentHandle cmp, e_float intensity) = 0;
		virtual e_void setPointLightColor(ComponentHandle cmp, const float3& color) = 0;
		virtual e_void setGlobalLightColor(ComponentHandle cmp, const float3& color) = 0;
		virtual e_void setFogDensity(ComponentHandle cmp, e_float density) = 0;
		virtual e_void setFogColor(ComponentHandle cmp, const float3& color) = 0;
		virtual e_float getPointLightIntensity(ComponentHandle cmp) = 0;
		virtual GameObject getPointLightEntity(ComponentHandle cmp) const = 0;
		virtual GameObject getGlobalLightEntity(ComponentHandle cmp) const = 0;
		virtual e_float getGlobalLightIntensity(ComponentHandle cmp) = 0;
		virtual e_float getGlobalLightIndirectIntensity(ComponentHandle cmp) = 0;
		virtual float3 getPointLightColor(ComponentHandle cmp) = 0;
		virtual float3 getGlobalLightColor(ComponentHandle cmp) = 0;
		virtual e_float getFogDensity(ComponentHandle cmp) = 0;
		virtual e_float getFogBottom(ComponentHandle cmp) = 0;
		virtual e_float getFogHeight(ComponentHandle cmp) = 0;
		virtual e_void setFogBottom(ComponentHandle cmp, e_float value) = 0;
		virtual e_void setFogHeight(ComponentHandle cmp, e_float value) = 0;
		virtual float3 getFogColor(ComponentHandle cmp) = 0;
		virtual float3 getPointLightSpecularColor(ComponentHandle cmp) = 0;
		virtual e_void setPointLightSpecularColor(ComponentHandle cmp, const float3& color) = 0;
		virtual e_float getPointLightSpecularIntensity(ComponentHandle cmp) = 0;
		virtual e_void setPointLightSpecularIntensity(ComponentHandle cmp, e_float color) = 0;

		virtual Texture* getEnvironmentProbeTexture(ComponentHandle cmp) const = 0;
		virtual Texture* getEnvironmentProbeIrradiance(ComponentHandle cmp) const = 0;
		virtual Texture* getEnvironmentProbeRadiance(ComponentHandle cmp) const = 0;
		virtual e_void reloadEnvironmentProbe(ComponentHandle cmp) = 0;
		virtual ComponentHandle getNearestEnvironmentProbe(const float3& pos) const = 0;
		virtual e_uint64 getEnvironmentProbeGUID(ComponentHandle cmp) const = 0;

	protected:
		virtual ~RenderScene() {}
	};
}

#endif
