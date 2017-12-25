#include "runtime/Render/culling_system.h"

#include "common/Multithreading/sync.h"
#include "common/Multithreading/thread.h"

namespace egal
{
	typedef Array<e_uint64> LayerMasks;
	typedef Array<e_int32> ModelInstancetoSphereMap;
	typedef Array<ComponentHandle> SphereToModelInstanceMap;

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
		float4 px = f4Load(frustum->xs);
		float4 py = f4Load(frustum->ys);
		float4 pz = f4Load(frustum->zs);
		float4 pd = f4Load(frustum->ds);
		float4 px2 = f4Load(&frustum->xs[4]);
		float4 py2 = f4Load(&frustum->ys[4]);
		float4 pz2 = f4Load(&frustum->zs[4]);
		float4 pd2 = f4Load(&frustum->ds[4]);

		for (const Sphere *sphere = start; sphere <= end; sphere++, ++i)
		{
			float4 cx = f4Splat(sphere->position.x);
			float4 cy = f4Splat(sphere->position.y);
			float4 cz = f4Splat(sphere->position.z);
			float4 r = f4Splat(-sphere->radius);

			float4 t = f4Mul(cx, px);
			t = f4Add(t, f4Mul(cy, py));
			t = f4Add(t, f4Mul(cz, pz));
			t = f4Add(t, pd);
			t = f4Sub(t, r);
			if (f4MoveMask(t)) continue;

			t = f4Mul(cx, px2);
			t = f4Add(t, f4Mul(cy, py2));
			t = f4Add(t, f4Mul(cz, pz2));
			t = f4Add(t, pd2);
			t = f4Sub(t, r);
			if (f4MoveMask(t)) continue;

			if (layer_masks[i] & layer_mask) results.push(sphere_to_model_instance_map[i]);
		}
	}

	struct CullingJobData
	{
		const CullingSystem::InputSpheres* spheres;
		CullingSystem::Subresults* results;
		const LayerMasks* layer_masks;
		const SphereToModelInstanceMap* sphere_to_model_instance_map;
		e_uint64 layer_mask;
		e_int32 start;
		e_int32 end;
		const Frustum* frustum;
	};

	class CullingSystemImpl : public CullingSystem
	{
	public:
		explicit CullingSystemImpl(IAllocator& allocator)
			: m_allocator(allocator)
			, m_job_allocator(allocator)
			, m_spheres(allocator)
			, m_result(allocator)
			, m_layer_masks(m_allocator)
			, m_sphere_to_model_instance_map(m_allocator)
			, m_model_instance_to_sphere_map(m_allocator)
		{
			m_result.emplace(m_allocator);
			m_model_instance_to_sphere_map.reserve(5000);
			m_sphere_to_model_instance_map.reserve(5000);
			m_spheres.reserve(5000);
			e_int32 cpu_count = (e_int32)MT::getCPUsCount();
			while (m_result.size() < cpu_count)
			{
				m_result.emplace(m_allocator);
			}
		}

		e_void clear() override
		{
			m_spheres.clear();
			m_layer_masks.clear();
			m_model_instance_to_sphere_map.clear();
			m_sphere_to_model_instance_map.clear();
		}

		IAllocator& getAllocator() { return m_allocator; }

		const Results& getResult() override
		{
			return m_result;
		}

		static e_void cullTask(e_void* data)
		{
			CullingJobData* cull_data = (CullingJobData*)data;
			if (cull_data->end < cull_data->start) return;
			doCulling(cull_data->start
				, &(*cull_data->spheres)[cull_data->start]
				, &(*cull_data->spheres)[cull_data->end]
				, cull_data->frustum
				, &(*cull_data->layer_masks)[0]
				, &(*cull_data->sphere_to_model_instance_map)[0]
				, cull_data->layer_mask
				, *cull_data->results);
		}


		Results& cull(const Frustum& frustum, e_uint64 layer_mask) override
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

		e_void setLayerMask(ComponentHandle model_instance, e_uint64 layer) override
		{
			m_layer_masks[m_model_instance_to_sphere_map[model_instance.index]] = layer;
		}

		e_uint64 getLayerMask(ComponentHandle model_instance) override
		{
			return m_layer_masks[m_model_instance_to_sphere_map[model_instance.index]];
		}

		e_bool isAdded(ComponentHandle model_instance) override
		{
			return model_instance.index < m_model_instance_to_sphere_map.size() && m_model_instance_to_sphere_map[model_instance.index] != -1;
		}

		e_void addStatic(ComponentHandle model_instance, const Sphere& sphere, e_uint64 layer_mask) override
		{
			if (model_instance.index < m_model_instance_to_sphere_map.size() &&
				m_model_instance_to_sphere_map[model_instance.index] != -1)
			{
				ASSERT(false);
				return;
			}

			m_spheres.push(sphere);
			m_sphere_to_model_instance_map.push(model_instance);
			while (model_instance.index >= m_model_instance_to_sphere_map.size())
			{
				m_model_instance_to_sphere_map.push(-1);
			}
			m_model_instance_to_sphere_map[model_instance.index] = m_spheres.size() - 1;
			m_layer_masks.push(layer_mask);
		}

		e_void removeStatic(ComponentHandle model_instance) override
		{
			if (model_instance.index >= m_model_instance_to_sphere_map.size()) return;
			e_int32 index = m_model_instance_to_sphere_map[model_instance.index];
			if (index < 0) return;
			ASSERT(index < m_spheres.size());

			m_model_instance_to_sphere_map[m_sphere_to_model_instance_map.back().index] = index;
			m_spheres[index] = m_spheres.back();
			m_sphere_to_model_instance_map[index] = m_sphere_to_model_instance_map.back();
			m_layer_masks[index] = m_layer_masks.back();

			m_spheres.pop();
			m_sphere_to_model_instance_map.pop();
			m_layer_masks.pop();
			m_model_instance_to_sphere_map[model_instance.index] = -1;
		}

		e_void updateBoundingSphere(const Sphere& sphere, ComponentHandle model_instance) override
		{
			e_int32 idx = m_model_instance_to_sphere_map[model_instance.index];
			if (idx >= 0) m_spheres[idx] = sphere;
		}

		e_void insert(const InputSpheres& spheres, const Array<ComponentHandle>& model_instances) override
		{
			for (e_int32 i = 0; i < spheres.size(); i++)
			{
				m_spheres.push(spheres[i]);
				while (m_model_instance_to_sphere_map.size() <= model_instances[i].index)
				{
					m_model_instance_to_sphere_map.push(-1);
				}
				m_model_instance_to_sphere_map[model_instances[i].index] = m_spheres.size() - 1;
				m_sphere_to_model_instance_map.push(model_instances[i]);
				m_layer_masks.push(1);
			}
		}

		const Sphere& getSphere(ComponentHandle model_instance) override
		{
			return m_spheres[m_model_instance_to_sphere_map[model_instance.index]];
		}

	private:
		IAllocator& m_allocator;
		FreeList<CullingJobData, 16> m_job_allocator;
		InputSpheres m_spheres;
		Results m_result;
		LayerMasks m_layer_masks;
		ModelInstancetoSphereMap m_model_instance_to_sphere_map;
		SphereToModelInstanceMap m_sphere_to_model_instance_map;
		CullingJobData job_data[16];
		JobSystem::JobDecl jobs[16];
	};

	CullingSystem* CullingSystem::create(IAllocator& allocator)
	{
		return _aligned_new(allocator, CullingSystemImpl)(allocator);
	}

	e_void CullingSystem::destroy(CullingSystem& culling_system)
	{
		_delete(static_cast<CullingSystemImpl&>(culling_system).getAllocator(), &culling_system);
	}
}
