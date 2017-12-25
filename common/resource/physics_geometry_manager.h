#ifndef _physics_geometry_manager_h_
#define _physics_geometry_manager_h_
#pragma once

#include "runtime/Resource/resource_define.h"
#include "runtime/Resource/resource_public.h"
#include "runtime/Resource/resource_manager.h"

namespace physx
{
	class PxTriangleMesh;
	class PxConvexMesh;
}

namespace egal
{
	class PhysicsSystem;
	class PhysicsGeometryManager : public ResourceManagerBase
	{
	public:
		PhysicsGeometryManager(PhysicsSystem& system, IAllocator& allocator)
			: ResourceManagerBase(allocator)
			, m_allocator(allocator)
			, m_system(system)
		{}
		~PhysicsGeometryManager() {}
		IAllocator& getAllocator() { return m_allocator; }
		PhysicsSystem& getSystem() { return m_system; }

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
		PhysicsSystem& m_system;
	};


	class PhysicsGeometry : public Resource
	{
	public:
		static const e_uint32 HEADER_MAGIC = 0x5f4c5046; // '_LPF'
		struct Header
		{
			e_uint32 m_magic;
			e_uint32 m_version;
			e_uint32 m_convex;
		};

		enum class Versions : e_uint32
		{
			FIRST,

			LAST
		};

	public:
		PhysicsGeometry(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
		~PhysicsGeometry();

	public:
		physx::PxTriangleMesh* tri_mesh;
		physx::PxConvexMesh* convex_mesh;

	private:
		IAllocator& getAllocator();

		e_void unload() override;
		e_bool load(FS::IFile& file) override;

	};

}

#endif
