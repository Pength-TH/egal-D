#ifndef _import_option_h_
#define _import_option_h_

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/egal_string.h"
#include "common/utils/logger.h"
#include "common/math/egal_math.h"

#include "common/animation/io/endianness.h"
#include <vector>

#include "common/animation/maths/simd_math.h"

namespace egal
{
	enum class Orientation
	{
		Y_UP,
		Z_UP,
		Z_MINUS_UP,
		X_MINUS_UP,
		X_UP
	};

	struct import_texture
	{
	public:
		import_texture()
		{
			texture_type = diffuse_texture;
			path[0] = '\0';
			is_valid = true;
			m_todds = false;
			m_import = true;
		}

		enum import_texture_type
		{
			//texture
			diffuse_texture,
			normal_texture,
			roughness_texture,
			metallic_texture,
			bump_texture,
			ambient_occlusion_texture,
			wind_texture,
		};

		import_texture_type texture_type;
		char				path[1024];
		bool				is_valid;
		bool				m_todds;
		bool				m_import;
	};

	struct import_material
	{
	public:
		import_material()
		{
			name[0] = '\0';
			index = 0;
			Ambient = { 0,0,0 };
			Diffuse = { 0,0,0 };
			Emissive = { 0,0,0 };
			Specular = { 0,0,0 };
			TransparencyFactor = 0;
			Shininess = 0;
			ReflectionFactor = 0;

			alpha_cutout = false;

			m_import = true;

			m_textures.clear();
		}

		char name[1024];
		int32_t index;

		//Phong
		float3 Ambient;
		float3 Diffuse;

		float3 Emissive;
		float3 Specular;
		float TransparencyFactor;

		float Shininess;
		float ReflectionFactor;

		bool alpha_cutout;

		std::vector<import_texture>	 m_textures;

		bool            m_import;
	};

	struct import_mesh
	{
	public:
		import_mesh()
		{
			m_name[0] = '\0';
			m_import = true;
			m_import_physics = false;
			m_general_lod = 0;
		}
		~import_mesh() {}

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

		bool colored() const
		{
			for (size_t i = 0; i < parts.size(); ++i)
			{
				//if (parts[i].colors().size() != 0)
				{
					return true;
				}
			}
			return false;
		}

		bool tangentsed() const
		{
			for (size_t i = 0; i < parts.size(); ++i)
			{
				//if (parts[i].tangents().size() != 0)
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
		public:
			Part() {}
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

			typedef std::vector<float> Positions;
			Positions positions;
			enum
			{
				kPositionsCpnts = 3
			};  // x, y, z components

			typedef std::vector<float> Normals;
			Normals normals;
			enum
			{
				kNormalsCpnts = 3
			};  // x, y, z components

			typedef std::vector<float> Tangents;
			Tangents tangents;
			enum
			{
				kTangentsCpnts = 4
			};  // x, y, z, right or left handed.

			typedef std::vector<float> UVs;
			UVs uvs;  // u, v components
			enum
			{
				kUVsCpnts = 2
			};

			typedef std::vector<uint8_t> Colors;
			Colors colors;
			enum
			{
				kColorsCpnts = 4
			};  // r, g, b, a components

			typedef std::vector<uint16_t> JointIndices;
			JointIndices joint_indices;  // Stride equals influences_count

			typedef std::vector<float> JointWeights;
			JointWeights joint_weights;  // Stride equals influences_count - 1
		};
		typedef std::vector<Part> Parts;
		Parts parts;

		// Triangles indices. Indices are shared across all parts.
		typedef std::vector<uint16_t> TriangleIndices;
		TriangleIndices triangle_indices;

		// Inverse bind-pose matrices. These are only available for skinned meshes.
		typedef std::vector<egal::math::Float4x4> InversBindPoses;
		InversBindPoses inverse_bind_poses;

		char			m_name[256];
		import_material m_material;

		bool            m_import;
		bool			m_import_physics;
		e_int32			m_general_lod;
	};

	//导入选项
	struct ImportOption
	{
	public:
		ImportOption()
		{
			clear();
		}

		void clear()
		{
			file_path_name[0] = '\0';
			out_mesh_path_name[0] = '\0';
			out_skeleton_path_name[0] = '\0';
			out_animation_path_name[0] = '\0';
			out_dir[0] = '\0';
			out_texture_dir[0] = '\0';

			m_endian = native;
			m_raw = false;
			m_influences = 0;
			m_additive = false;
			m_split = true;
			m_hierarchical = 0.001;
			m_optimize = true;
			m_rotation = 0.00174533;
			m_sampling_rate = 0;
			m_scale = 0.001;
			m_translation = 0.001;

			lods_distances[0] = -10;
			lods_distances[1] = -100;
			lods_distances[2] = -1000;
			lods_distances[3] = -10000;

			mesh_scale = 1.0f;
			time_scale = 1.0f;
			position_error = 0.1f;
			rotation_error = 0.01f;
			bounding_shape_scale = 1.0f;
			to_dds = false;
			center_mesh = false;
			ignore_skeleton = false;
			import_vertex_colors = true;
			make_convex = false;
			create_billboard_lod = false;
			orientation = Orientation::Y_UP;
			root_orientation = Orientation::Y_UP;

			m_meshs.clear();
		}

		char file_path_name[1024];

		char out_dir[1024];
		char out_texture_dir[1024];

		char out_mesh_path_name[1024];
		char out_skeleton_path_name[1024];
		char out_animation_path_name[1024];

		Endianness m_endian; //native
		
		e_bool m_raw; // false
		e_bool m_split; // ttue
		e_int32 m_influences; // 0
 	    
							  /**animation*/
		e_bool  m_additive; //false
		e_float m_hierarchical; //0.001;
		e_bool  m_optimize; //true
		e_float m_rotation; //0.00174533
		e_float m_sampling_rate; //0
		e_float m_scale;// 0.001
		e_float m_translation;// 0.001

		std::vector<import_mesh>	 m_meshs;
		std::vector<import_material> m_materials;

		e_float lods_distances[4];
		e_float mesh_scale;
		e_float time_scale;
		e_float position_error;
		e_float rotation_error;
		e_float bounding_shape_scale;
		e_bool  to_dds;
		e_bool  center_mesh;
		e_bool  ignore_skeleton;
		e_bool  import_vertex_colors;
		e_bool  make_convex;
		e_bool  create_billboard_lod;
		Orientation orientation;
		Orientation root_orientation;
	};



}
#endif
