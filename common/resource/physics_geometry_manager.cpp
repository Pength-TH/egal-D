#include "runtime/Resource/physics_geometry_manager.h"
#include "physics/physics_system.h"
#include <PxPhysicsAPI.h>

namespace egal
{
	struct OutputStream : public physx::PxOutputStream
	{
		explicit OutputStream(IAllocator& allocator)
			: allocator(allocator)
		{
			data = (e_uint8*)allocator.allocate(sizeof(e_uint8) * 4096);
			capacity = 4096;
			size = 0;
		}

		~OutputStream() { allocator.deallocate(data); }


		physx::PxU32 write(const e_void* src, physx::PxU32 count) override
		{
			if (size + (e_int32)count > capacity)
			{
				e_int32 new_capacity = Math::maximum(size + (e_int32)count, capacity + 4096);
				e_uint8* new_data = (e_uint8*)allocator.allocate(sizeof(e_uint8) * new_capacity);
				copyMemory(new_data, data, size);
				allocator.deallocate(data);
				data = new_data;
				capacity = new_capacity;
			}
			copyMemory(data + size, src, count);
			size += count;
			return count;
		}

		e_uint8* data;
		IAllocator& allocator;
		e_int32 capacity;
		e_int32 size;
	};

	struct InputStream : public physx::PxInputStream
	{
		InputStream(unsigned e_char* data, e_int32 size)
		{
			this->data = data;
			this->size = size;
			pos = 0;
		}

		physx::PxU32 read(e_void* dest, physx::PxU32 count) override
		{
			if (pos + (e_int32)count <= size)
			{
				copyMemory(dest, data + pos, count);
				pos += count;
				return count;
			}
			else
			{
				copyMemory(dest, data + pos, size - pos);
				e_int32 real_count = size - pos;
				pos = size;
				return real_count;
			}
		}


		e_int32 pos;
		e_int32 size;
		unsigned e_char* data;
	};


	Resource* PhysicsGeometryManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, PhysicsGeometry)(path, *this, m_allocator);
	}


	e_void PhysicsGeometryManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<PhysicsGeometry*>(&resource));
	}


	PhysicsGeometry::PhysicsGeometry(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
		: Resource(path, resource_manager, allocator)
		, convex_mesh(nullptr)
		, tri_mesh(nullptr)
	{
	}

	PhysicsGeometry::~PhysicsGeometry() = default;


	e_bool PhysicsGeometry::load(FS::IFile& file)
	{
		Header header;
		file.read(&header, sizeof(header));
		if (header.m_magic != HEADER_MAGIC)
		{
			log_waring("Physics Corrupted geometry %s.", getPath().c_str());
			return false;
		}

		if (header.m_version > (e_uint32)Versions::LAST)
		{
			log_waring("Physics Unsupported version of geometry %s.", getPath().c_str());
			return false;
		}

		PhysicsSystem& system = static_cast<PhysicsGeometryManager&>(m_resource_manager).getSystem();

		e_int32 num_verts;
		Array<Vec3> verts(getAllocator());
		file.read(&num_verts, sizeof(num_verts));
		verts.resize(num_verts);
		file.read(&verts[0], sizeof(verts[0]) * verts.size());

		e_bool is_convex = header.m_convex != 0;
		if (is_convex)
		{
			physx::PxConvexMeshDesc meshDesc;
			meshDesc.points.count = verts.size();
			meshDesc.points.stride = sizeof(Vec3);
			meshDesc.points.data = &verts[0];
			meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

			OutputStream writeBuffer(getAllocator());
			e_bool status = system.getCooking()->cookConvexMesh(meshDesc, writeBuffer);
			if (!status)
			{
				convex_mesh = nullptr;
				return false;
			}

			InputStream readBuffer(writeBuffer.data, writeBuffer.size);
			convex_mesh = system.getPhysics()->createConvexMesh(readBuffer);
			tri_mesh = nullptr;
		}
		else
		{
			e_uint32 num_indices;
			Array<e_uint32> tris(getAllocator());
			file.read(&num_indices, sizeof(num_indices));
			tris.resize(num_indices);
			file.read(&tris[0], sizeof(tris[0]) * tris.size());

			physx::PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = num_verts;
			meshDesc.points.stride = sizeof(physx::PxVec3);
			meshDesc.points.data = &verts[0];

			meshDesc.triangles.count = num_indices / 3;
			meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
			meshDesc.triangles.data = &tris[0];

			OutputStream writeBuffer(getAllocator());
			if (!system.getCooking()->cookTriangleMesh(meshDesc, writeBuffer))
			{
				tri_mesh = nullptr;
				return false;
			}

			InputStream readBuffer(writeBuffer.data, writeBuffer.size);
			tri_mesh = system.getPhysics()->createTriangleMesh(readBuffer);
			convex_mesh = nullptr;
		}

		m_size = file.size();
		return true;
	}


	IAllocator& PhysicsGeometry::getAllocator()
	{
		return static_cast<PhysicsGeometryManager&>(m_resource_manager).getAllocator();
	}


	e_void PhysicsGeometry::unload()
	{
		if (convex_mesh) convex_mesh->release();
		if (tri_mesh) tri_mesh->release();
		convex_mesh = nullptr;
		tri_mesh = nullptr;
	}
}
