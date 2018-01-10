
#include "runtime/EngineFramework/culling_system.h"

namespace egal
{
	static e_void doCulling(e_int32 start_index,
		const Sphere* start,
		const Sphere* end,
		const Frustum* frustum,
		const e_uint64* layer_masks,
		const ComponentHandle* sphere_to_model_instance_map,
		e_uint64 layer_mask,
		CullingSystem::Subresults& results)
	{
		//PROFILE_FUNCTION();
		e_int32 i = start_index;
		assert(results.empty());
		//PROFILE_INT("objects", e_int32(end - start));
		simd4 px = f4Load(frustum->xs);
		simd4 py = f4Load(frustum->ys);
		simd4 pz = f4Load(frustum->zs);
		simd4 pd = f4Load(frustum->ds);
		simd4 px2 = f4Load(&frustum->xs[4]);
		simd4 py2 = f4Load(&frustum->ys[4]);
		simd4 pz2 = f4Load(&frustum->zs[4]);
		simd4 pd2 = f4Load(&frustum->ds[4]);

		for (const Sphere *sphere = start; sphere <= end; sphere++, ++i)
		{
			simd4 cx = f4Splat(sphere->position.x);
			simd4 cy = f4Splat(sphere->position.y);
			simd4 cz = f4Splat(sphere->position.z);
			simd4 r = f4Splat(-sphere->radius);

			simd4 t = f4Mul(cx, px);
			t = f4Add(t, f4Mul(cy, py));
			t = f4Add(t, f4Mul(cz, pz));
			t = f4Add(t, pd);
			t = f4Sub(t, r);
			if (f4MoveMask(t))
				continue;

			t = f4Mul(cx, px2);
			t = f4Add(t, f4Mul(cy, py2));
			t = f4Add(t, f4Mul(cz, pz2));
			t = f4Add(t, pd2);
			t = f4Sub(t, r);
			if (f4MoveMask(t))
				continue;

			if (layer_masks[i] & layer_mask)
				results.push_back(sphere_to_model_instance_map[i]);
		}
	}

	static e_void cullTask(e_void* data)
	{
		CullingSystem::CullingJobData* cull_data = (CullingSystem::CullingJobData*)data;

		if (cull_data->end < cull_data->start)
			return;
		
		doCulling(cull_data->start
			, &(*cull_data->spheres)[cull_data->start]
			, &(*cull_data->spheres)[cull_data->end]
			, cull_data->frustum
			, &(*cull_data->layer_masks)[0]
			, &(*cull_data->sphere_to_model_instance_map)[0]
			, cull_data->layer_mask
			, *cull_data->results);
	}

	CullingSystem* CullingSystem::create(IAllocator& allocator)
	{
		return _aligned_new(allocator, CullingSystem)(allocator);
	}

	e_void CullingSystem::destroy(CullingSystem& culling_system)
	{
		_delete(static_cast<CullingSystem&>(culling_system).getAllocator(), &culling_system);
	}

	/********************************************************************************************************/
	/********************************************************************************************************/
	/********************************************************************************************************/

	CullingSystem::CullingSystem(IAllocator& allocator)
		: m_allocator(allocator)
		, m_job_allocator(allocator)
		, m_spheres(allocator)
		, m_result(allocator)
		, m_layer_masks(allocator)
		, m_sphere_to_model_instance_map(allocator)
		, m_entity_instance_to_sphere_map(allocator)
	{
		m_result.emplace(m_allocator);
		m_entity_instance_to_sphere_map.reserve(RESERVED_ENTITIES_COUNT);
		m_sphere_to_model_instance_map.reserve(RESERVED_ENTITIES_COUNT);
		m_spheres.reserve(RESERVED_ENTITIES_COUNT);
		e_int32 cpu_count = (e_int32)MT::getCPUsCount();
		while (m_result.size() < cpu_count)
		{
			m_result.emplace(m_allocator);
		}
	}

	CullingSystem::~CullingSystem()
	{
		destroy(*this);
	}

	e_void CullingSystem::clear()
	{
		m_spheres.clear();
		m_layer_masks.clear();
		m_entity_instance_to_sphere_map.clear();
		m_sphere_to_model_instance_map.clear();
	}

	CullingSystem::Results& CullingSystem::cull(const Frustum& frustum, e_uint64 layer_mask)
	{
		e_int32 count = m_spheres.size();
		for (auto& i : m_result) i.clear();

		e_int32 step = count / m_result.size();
		assert(TlengthOf(jobs) >= m_result.size());
		for (e_int32 i = 0; i < m_result.size(); i++)
		{
			m_result[i].clear();
			job_data[i] = {
				&m_spheres,
				&m_result[i],
				&m_layer_masks,
				&m_sphere_to_model_instance_map,
				layer_mask,
				i * step,
				i == m_result.size() - 1 ? count - 1 : (i + 1) * step - 1,
				&frustum
			};
			jobs[i].data = &job_data[i];
			jobs[i].task = &cullTask;
		}
		volatile e_int32 job_counter = 0;
		JobSystem::runJobs(jobs, m_result.size(), &job_counter);
		JobSystem::wait(&job_counter);
		return m_result;
	}

	e_void CullingSystem::setLayerMask(ComponentHandle model_instance, e_uint64 layer)
	{
		m_layer_masks[m_entity_instance_to_sphere_map[model_instance.index]] = layer;
	}

	e_uint64 CullingSystem::getLayerMask(ComponentHandle model_instance)
	{
		return m_layer_masks[m_entity_instance_to_sphere_map[model_instance.index]];
	}

	e_bool CullingSystem::isAdded(ComponentHandle model_instance)
	{
		return model_instance.index < m_entity_instance_to_sphere_map.size() && m_entity_instance_to_sphere_map[model_instance.index] != -1;
	}

	e_void CullingSystem::addStatic(ComponentHandle model_instance, const Sphere& sphere, e_uint64 layer_mask)
	{
		if (model_instance.index < m_entity_instance_to_sphere_map.size() &&
			m_entity_instance_to_sphere_map[model_instance.index] != -1)
		{
			ASSERT(false);
			return;
		}

		m_spheres.push_back(sphere);
		m_sphere_to_model_instance_map.push_back(model_instance);
		while (model_instance.index >= m_entity_instance_to_sphere_map.size())
		{
			m_entity_instance_to_sphere_map.push_back(-1);
		}
		m_entity_instance_to_sphere_map[model_instance.index] = m_spheres.size() - 1;
		m_layer_masks.push_back(layer_mask);
	}

	e_void CullingSystem::removeStatic(ComponentHandle model_instance)
	{
		if (model_instance.index >= m_entity_instance_to_sphere_map.size())
			return;
		e_int32 index = m_entity_instance_to_sphere_map[model_instance.index];
		if (index < 0)
			return;
		ASSERT(index < m_spheres.size());

		m_entity_instance_to_sphere_map[m_sphere_to_model_instance_map.back().index] = index;
		m_spheres[index] = m_spheres.back();
		m_sphere_to_model_instance_map[index] = m_sphere_to_model_instance_map.back();
		m_layer_masks[index] = m_layer_masks.back();

		m_spheres.pop_back();
		m_sphere_to_model_instance_map.pop_back();
		m_layer_masks.pop_back();
		m_entity_instance_to_sphere_map[model_instance.index] = -1;
	}

	e_void CullingSystem::updateBoundingSphere(const Sphere& sphere, ComponentHandle model_instance)
	{
		e_int32 idx = m_entity_instance_to_sphere_map[model_instance.index];
		if (idx >= 0) m_spheres[idx] = sphere;
	}

	e_void CullingSystem::insert(const InputSpheres& spheres, const Subresults& model_instances)
	{
		for (e_int32 i = 0; i < spheres.size(); i++)
		{
			m_spheres.push_back(spheres[i]);
			while (m_entity_instance_to_sphere_map.size() <= model_instances[i].index)
			{
				m_entity_instance_to_sphere_map.push_back(-1);
			}
			m_entity_instance_to_sphere_map[model_instances[i].index] = m_spheres.size() - 1;
			m_sphere_to_model_instance_map.push_back(model_instances[i]);
			m_layer_masks.push_back(1);
		}
	}

	const Sphere& CullingSystem::getSphere(ComponentHandle model_instance)
	{
		return m_spheres[m_entity_instance_to_sphere_map[model_instance.index]];
	}
}
