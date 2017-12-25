#ifndef _culling_system_h_
#define _culling_system_h_
#pragma once

#include "common/egal-d.h"

namespace egal
{
	class CullingSystem
	{
	public:
		static CullingSystem* create(IAllocator& allocator);
		static e_void destroy(CullingSystem& culling_system);

	public:
		typedef TVector<Sphere>				InputSpheres;
		typedef TVector<ComponentHandle>	Subresults;
		typedef TVector<Subresults>			Results;

		CullingSystem() { }
		virtual ~CullingSystem() { }

		virtual e_void clear() = 0;
		virtual const Results& getResult() = 0;

		virtual Results& cull(const Frustum& frustum, e_uint64 layer_mask) = 0;

		virtual e_bool isAdded(ComponentHandle model_instance) = 0;
		virtual e_void addStatic(ComponentHandle model_instance, const Sphere& sphere, e_uint64 layer_mask) = 0;
		virtual e_void removeStatic(ComponentHandle model_instance) = 0;

		virtual e_void setLayerMask(ComponentHandle model_instance, e_uint64 layer) = 0;
		virtual e_uint64 getLayerMask(ComponentHandle model_instance) = 0;

		virtual e_void updateBoundingSphere(const Sphere& sphere, ComponentHandle model_instance) = 0;

		virtual e_void insert(const InputSpheres& spheres, const Subresults& model_instances) = 0;
		virtual const Sphere& getSphere(ComponentHandle model_instance) = 0;
	};
}
#endif
