
#ifndef SAMPLES_FRAMEWORK_MESH_H_
#define SAMPLES_FRAMEWORK_MESH_H_
#include "common/animation/platform.h"

#include "common/animation/io/archive.h"
#include "common/animation/maths/simd_math.h"
#include "common/animation/maths/vec_float.h"

#include "common/egal-d.h"

namespace egal
{
	//namespace offline
	//{
	//	// Defines a mesh with skinning information (joint indices and weights).
	//	// The mesh is subdivided into parts that group vertices according to their
	//	// number of influencing joints. Triangle indices are shared across mesh parts.
	//	
	//}  // namespace sample

/*	namespace io
	{
		IO_TYPE_TAG("Mesh-Part", offline::Mesh::Part);
		IO_TYPE_VERSION(1, offline::Mesh::Part);

		IO_TYPE_TAG("Mesh", offline::Mesh);
		IO_TYPE_VERSION(1, offline::Mesh);

		template <>
		void Save(OArchive& _archive, const offline::Mesh* _meshes, size_t _count);

		template <>
		void Load(IArchive& _archive, offline::Mesh* _meshes, size_t _count,
			uint32_t _version);
	}*/  // namespace io
}  // namespace egal
#endif  // SAMPLES_FRAMEWORK_MESH_H_
