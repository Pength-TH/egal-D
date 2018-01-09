#ifndef _culling_system_h_
#define _culling_system_h_
#pragma once

#include "common/egal-d.h"
#include "common/allocator/pool_allocator.h"

#include <vector>
namespace egal
{
	/**  */
	class CullingSystem
	{
	public:
		static CullingSystem* create(IAllocator& allocator);
		static e_void destroy(CullingSystem& culling_system);

	public:
		typedef TVector<Sphere>				InputSpheres;
		typedef TVector<ComponentHandle>	Subresults;
		typedef TVector<Subresults>			Results;
		typedef TVector<e_uint64>			LayerMasks;
		typedef TVector<e_int32>			EntityInstancetoSphereMap;
		typedef TVector<ComponentHandle>	SphereToModelInstanceMap;

		struct CullingJobData
		{
			const CullingSystem::InputSpheres*	spheres;
			CullingSystem::Subresults*			results;
			const LayerMasks*					layer_masks;
			const SphereToModelInstanceMap*		sphere_to_model_instance_map;
			e_uint64							layer_mask;
			e_int32								start;
			e_int32								end;
			const Frustum*						frustum;
		};

	public:
		CullingSystem(IAllocator& allocator);
		~CullingSystem();

		e_void clear();

		const Results& getResult() { return m_result; }
		IAllocator& getAllocator() { return m_allocator; }

		Results& cull(const Frustum& frustum, e_uint64 layer_mask);

		e_bool isAdded(ComponentHandle model_instance);
		e_void addStatic(ComponentHandle model_instance, const Sphere& sphere, e_uint64 layer_mask);
		e_void removeStatic(ComponentHandle model_instance);

		e_void setLayerMask(ComponentHandle model_instance, e_uint64 layer);
		e_uint64 getLayerMask(ComponentHandle model_instance);

		e_void updateBoundingSphere(const Sphere& sphere, ComponentHandle model_instance);

		e_void insert(const InputSpheres& spheres, const Subresults& model_instances);
		const Sphere& getSphere(ComponentHandle model_instance);
	private:
		PoolAllocator<CullingJobData, 16>	m_job_allocator;
		InputSpheres						m_spheres;
		Results								m_result;
		LayerMasks							m_layer_masks;
		EntityInstancetoSphereMap			m_entity_instance_to_sphere_map;
		SphereToModelInstanceMap			m_sphere_to_model_instance_map;
		CullingJobData						job_data[16];
		JobSystem::JobDecl					jobs[16];
		IAllocator&							m_allocator;
	};
}
#endif
