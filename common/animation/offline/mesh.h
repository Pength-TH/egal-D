
#ifndef SAMPLES_FRAMEWORK_MESH_H_
#define SAMPLES_FRAMEWORK_MESH_H_
#include "common/animation/platform.h"

#include "common/animation/maths/simd_math.h"
#include "common/animation/maths/vec_float.h"

#include "common/egal-d.h"

namespace egal
{
	namespace sample
	{
		// Defines a mesh with skinning information (joint indices and weights).
		// The mesh is subdivided into parts that group vertices according to their
		// number of influencing joints. Triangle indices are shared across mesh parts.
		struct Mesh
		{
		public:
			Mesh();
			~Mesh();

			// Number of triangle indices for the mesh.
			int triangle_index_count() const
			{
				return static_cast<int>(triangle_indices.size());
			}

			// Number of vertices for all mesh parts.
			int vertex_count() const
			{
				int vertex_count = 0;
				for (size_t i = 0; i < parts.size(); ++i)
				{
					vertex_count += parts[i].vertex_count();
				}
				return vertex_count;
			}

			// Maximum number of joints influences for all mesh parts.
			int max_influences_count() const
			{
				int max_influences_count = 0;
				for (size_t i = 0; i < parts.size(); ++i)
				{
					const int influences_count = parts[i].influences_count();
					max_influences_count = influences_count > max_influences_count
						? influences_count
						: max_influences_count;
				}
				return max_influences_count;
			}

			// Test if the mesh has skinning informations.
			bool skinned() const
			{
				for (size_t i = 0; i < parts.size(); ++i)
				{
					if (parts[i].influences_count() != 0)
					{
						return true;
					}
				}
				return false;
			}

			// Returns the number of joints used to skin the mesh.
			int num_joints()
			{
				return static_cast<int>(inverse_bind_poses.size());
			}

			// Defines a portion of the mesh. A mesh is subdivided in sets of vertices
			// with the same number of joint influences.
			struct Part
			{
				Part()
				: positions(*g_allocator)
				, normals(*g_allocator)
				, tangents(*g_allocator)
				, uvs(*g_allocator)
				, colors(*g_allocator)
				, joint_indices(*g_allocator)
				, joint_weights(*g_allocator)
				{

				}

				int vertex_count() const
				{
					return static_cast<int>(positions.size()) / 3;
				}

				int influences_count() const
				{
					const int _vertex_count = vertex_count();
					if (_vertex_count == 0)
					{
						return 0;
					}
					return static_cast<int>(joint_indices.size()) / _vertex_count;
				}

				typedef TVector<float> Positions;
				Positions positions;
				enum
				{
					kPositionsCpnts = 3
				};  // x, y, z components

				typedef TVector<float> Normals;
				Normals normals;
				enum
				{
					kNormalsCpnts = 3
				};  // x, y, z components

				typedef TVector<float> Tangents;
				Tangents tangents;
				enum
				{
					kTangentsCpnts = 4
				};  // x, y, z, right or left handed.

				typedef TVector<float> UVs;
				UVs uvs;  // u, v components
				enum
				{
					kUVsCpnts = 2
				};

				typedef TVector<uint8_t> Colors;
				Colors colors;
				enum
				{
					kColorsCpnts = 4
				};  // r, g, b, a components

				typedef TVector<uint16_t> JointIndices;
				JointIndices joint_indices;  // Stride equals influences_count

				typedef TVector<float> JointWeights;
				JointWeights joint_weights;  // Stride equals influences_count - 1
			};
			typedef TVector<Part> Parts;
			Parts parts;

			// Triangles indices. Indices are shared across all parts.
			typedef TVector<uint16_t> TriangleIndices;
			TriangleIndices triangle_indices;

			// Inverse bind-pose matrices. These are only available for skinned meshes.
			typedef TVector<egal::math::Float4x4> InversBindPoses;
			InversBindPoses inverse_bind_poses;
		};
	}  // namespace sample

	namespace io
	{
		//IO_TYPE_TAG("Mesh-Part", sample::Mesh::Part);
		//IO_TYPE_VERSION(1, sample::Mesh::Part);

		//IO_TYPE_TAG("Mesh", sample::Mesh);
		//IO_TYPE_VERSION(1, sample::Mesh);

		template <>
		void Save(OArchive& _archive, const sample::Mesh* _meshes, size_t _count);

		template <>
		void Load(IArchive& _archive, sample::Mesh* _meshes, size_t _count,
			uint32_t _version);
	}  // namespace io
}  // namespace egal
#endif  // SAMPLES_FRAMEWORK_MESH_H_
