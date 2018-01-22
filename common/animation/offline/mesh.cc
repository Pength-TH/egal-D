#include "common/animation/offline/mesh.h"

#include "common/animation/maths/math_archive.h"
#include "common/animation/maths/simd_math_archive.h"
#include "common/animation/io/archive.h"

namespace egal
{
	//namespace offline
	//{
	//	Mesh::Mesh()
	//	{

	//	}

	//	Mesh::~Mesh()
	//	{
	//	}
	//}  // namespace sample

	//namespace io
	//{

	//	template <>
	//	void Save(OArchive& _archive, const offline::Mesh::Part* _parts, size_t _count)
	//	{
	//		for (size_t i = 0; i < _count; ++i)
	//		{
	//			const offline::Mesh::Part& part = _parts[i];
	//			//_archive << part.positions;
	//			//_archive << part.normals;
	//			//_archive << part.tangents;
	//			//_archive << part.uvs;
	//			//_archive << part.colors;
	//			//_archive << part.joint_indices;
	//			//_archive << part.joint_weights;
	//		}
	//	}

	//	template <>
	//	void Load(IArchive& _archive, offline::Mesh::Part* _parts, size_t _count,
	//		uint32_t _version)
	//	{
	//		(void)_version;
	//		for (size_t i = 0; i < _count; ++i)
	//		{
	//			offline::Mesh::Part& part = _parts[i];
	//			//_archive >> part.positions;
	//			//_archive >> part.normals;
	//			//_archive >> part.tangents;
	//			//_archive >> part.uvs;
	//			//_archive >> part.colors;
	//			//_archive >> part.joint_indices;
	//			//_archive >> part.joint_weights;
	//		}
	//	}

	//	template <>
	//	void Save(OArchive& _archive, const offline::Mesh* _meshes, size_t _count)
	//	{
	//		for (size_t i = 0; i < _count; ++i)
	//		{
	//			const offline::Mesh& mesh = _meshes[i];
	//			//_archive << mesh.parts;
	//			//_archive << mesh.triangle_indices;
	//			//_archive << mesh.inverse_bind_poses;
	//		}
	//	}

	//	template <>
	//	void Load(IArchive& _archive, offline::Mesh* _meshes, size_t _count,
	//		uint32_t _version)
	//	{
	//		(void)_version;
	//		for (size_t i = 0; i < _count; ++i)
	//		{
	//			offline::Mesh& mesh = _meshes[i];
	//			//_archive >> mesh.parts;
	//			//_archive >> mesh.triangle_indices;
	//			//_archive >> mesh.inverse_bind_poses;
	//		}
	//	}
	//}  // namespace io
}  // namespace egal
