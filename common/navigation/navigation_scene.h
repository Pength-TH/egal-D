#pragma once
#include "engine/lumix.h"
#include "engine/iplugin.h"


struct dtCrowdAgent;


namespace Lumix
{


struct IAllocator;
template <typename T> class DelegateList;


class NavigationScene : public IScene
{
public:
	static NavigationScene* create(Engine& engine, IPlugin& system, Universe& universe, IAllocator& allocator);
	static void destroy(NavigationScene& scene);

	virtual bool isFinished(ComponentHandle cmp) = 0;
	virtual bool navigate(ComponentHandle cmp, const struct Vec3& dest, float speed, float stop_distance) = 0;
	virtual void cancelNavigation(ComponentHandle cmp) = 0;
	virtual void setActorActive(ComponentHandle cmp, bool active) = 0;
	virtual float getAgentSpeed(ComponentHandle cmp) = 0;
	virtual float getAgentYawDiff(ComponentHandle cmp) = 0;
	virtual void setAgentRadius(ComponentHandle cmp, float radius) = 0;
	virtual float getAgentRadius(ComponentHandle cmp) = 0;
	virtual void setAgentHeight(ComponentHandle cmp, float height) = 0;
	virtual float getAgentHeight(ComponentHandle cmp) = 0;
	virtual void setAgentRootMotion(ComponentHandle cmp, const Vec3& root_motion) = 0;
	virtual bool useAgentRootMotion(ComponentHandle cmp) = 0;
	virtual void setUseAgentRootMotion(ComponentHandle cmp, bool use_root_motion) = 0;
	virtual bool isGettingRootMotionFromAnim(ComponentHandle cmp) = 0;
	virtual void setIsGettingRootMotionFromAnim(ComponentHandle cmp, bool is) = 0;
	virtual bool generateNavmesh() = 0;
	virtual bool generateTile(int x, int z, bool keep_data) = 0;
	virtual bool generateTileAt(const Vec3& pos, bool keep_data) = 0;
	virtual bool load(const char* path) = 0;
	virtual bool save(const char* path) = 0;
	virtual int getPolygonCount() = 0;
	virtual void debugDrawNavmesh(const Vec3& pos, bool inner_boundaries, bool outer_boundaries, bool portals) = 0;
	virtual void debugDrawCompactHeightfield() = 0;
	virtual void debugDrawHeightfield() = 0;
	virtual void debugDrawContours() = 0;
	virtual void debugDrawPath(ComponentHandle cmp) = 0;
	virtual const dtCrowdAgent* getDetourAgent(ComponentHandle cmp) = 0;
	virtual bool isNavmeshReady() const = 0;
	virtual bool hasDebugDrawData() const = 0;
	virtual DelegateList<void(float)>& onUpdate() = 0;
	virtual void setGeneratorParams(float cell_size,
		float cell_height,
		float agent_radius,
		float agent_height,
		float walkable_angle,
		float max_climb) = 0;

};


} // namespace Lumix
