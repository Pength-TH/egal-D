#ifndef _fbx2resource_h_
#define _fbx2resource_h_
#pragma once
#include "common/egal-d.h"
#include "ofbx.h"
namespace egal
{
	class ImportAssetDialog;
	class ResourceSerializer
	{
	protected:
		enum class Orientation
		{
			Y_UP,
			Z_UP,
			Z_MINUS_UP,
			X_MINUS_UP,
			X_UP
		};

		struct RotationKey
		{
			Quaternion rot;
			float time;
			e_uint16 frame;
		};

		struct TranslationKey
		{
			float3 pos;
			float time;
			e_uint16 frame;
		};

		struct Skin
		{
			float weights[4];
			e_int16 joints[4];
			int count = 0;
		};

		struct ImportAnimation
		{
			struct Split
			{
				int from_frame = 0;
				int to_frame = 0;
				StaticString<32> name;
			};

			explicit ImportAnimation(IAllocator& allocator)
				: splits(allocator)
			{}

			const ofbx::AnimationStack* fbx = nullptr;
			const ofbx::IScene* scene = nullptr;
			TArrary<Split> splits;
			StaticString<MAX_PATH_LENGTH> output_filename;
			bool import = true;
			int root_motion_bone_idx = -1;
		};

		struct ImportTexture
		{
			enum Type
			{
				DIFFUSE,
				NORMAL,
				COUNT
			};

			const ofbx::Texture* fbx = nullptr;
			bool import = true;
			bool to_dds = true;
			bool is_valid = false;
			StaticString<MAX_PATH_LENGTH> path;
			StaticString<MAX_PATH_LENGTH> src;
		};

		struct ImportMaterial
		{
			const ofbx::Material* fbx = nullptr;
			bool import = true;
			bool alpha_cutout = false;
			ImportTexture textures[ImportTexture::COUNT];
			char shader[20];
		};

		struct ImportMesh
		{
			ImportMesh(IAllocator& allocator)
				: vertex_data(allocator)
				, indices(allocator)
			{
			}

			const ofbx::Mesh* fbx = nullptr;
			const ofbx::Material* fbx_mat = nullptr;
			bool import = true;
			bool import_physics = false;
			int lod = 0;
			WriteBinary vertex_data;
			TArrary<int> indices;
			AABB aabb;
			float radius_squared;
		};


	public:
		ResourceSerializer(ImportAssetDialog& _dialog);
		~ResourceSerializer();

		static ofbx::Matrix makeOFBXIdentity();
		static const char* getImportMeshName(const ImportMesh& mesh);
		static void insertHierarchy(TArrary<const ofbx::Object*>& bones, const ofbx::Object* node);
		static ofbx::Matrix getBindPoseMatrix(const ofbx::Mesh* mesh, const ofbx::Object* node);
		static void makeValidFilename(char* filename);
		static int findSubblobIndex(const WriteBinary& haystack, const WriteBinary& needle, const TArrary<int>& subblobs, int first_subblob);
		static void writeUV(const ofbx::Vec2& uv, WriteBinary* blob);
		static void writeColor(const ofbx::Vec4& color, WriteBinary* blob);
		static void writeSkin(const Skin& skin, WriteBinary* blob);
		static int getMaterialIndex(const ofbx::Mesh& mesh, const ofbx::Material& material);
		static int detectMeshLOD(const ImportMesh& mesh);
		static float3 toLumixfloat3(const ofbx::Vec4& v);
		static float3 toLumixfloat3(const ofbx::Vec3& v);
		static Quaternion toLumix(const ofbx::Quat& q);
		static Matrix toLumix(const ofbx::Matrix& mtx);
		static void getRelativePath(char* relative_path, int max_length, const char* source);
		static float3 getTranslation(const ofbx::Matrix& mtx);
		static Quaternion getRotation(const ofbx::Matrix& mtx);
		static void compressPositions(TArrary<TranslationKey>& out, int from_frame, int to_frame, float sample_period, const ofbx::AnimationCurveNode* curve_node, const ofbx::Object& bone, float error, float parent_scale);
		static void compressRotations(TArrary<RotationKey>& out, int from_frame, int to_frame, float sample_period, const ofbx::AnimationCurveNode* curve_node, const ofbx::Object& bone, float error);
		static float getScaleX(const ofbx::Matrix& mtx);
		static int getDepth(const ofbx::Object* bone);
		static void getMaterialName(const ofbx::Material* material, char(&out)[128]);


		const ofbx::Mesh* getAnyMeshFromBone(const ofbx::Object* node) const;
		void gatherMaterials(const ofbx::Object* node, const char* src_dir);
		void sortBones();
		void gatherBones(const ofbx::IScene& scene);
		void gatherAnimations(const ofbx::IScene& scene);
		void writePackedfloat3(const ofbx::Vec3& vec, const Matrix& mtx, WriteBinary* blob) const;
		void postprocessMeshes();
		void gatherMeshes(ofbx::IScene* scene);
		bool addSource(const char* filename);
		
		template <typename T> 
		void write(const T& obj);
		
		void write(const void* ptr, size_t size);
		void writeString(const char* str);
		bool writeBillboardMaterial(const char* output_dir, const char* texture_output_dir, const char* mesh_output_filename);
		void writeMaterials(const char* output_dir, const char* texture_output_dir, const char* mesh_output_filename);
		void writeAnimations(const char* output_dir);
		bool isSkinned(const ofbx::Mesh& mesh) const;

		int getVertexSize(const ofbx::Mesh& mesh) const;
		void fillSkinInfo(TArrary<Skin>& skinning, const ofbx::Mesh* mesh) const;

		float3 fixRootOrientation(const float3& v) const;

		Quaternion fixRootOrientation(const Quaternion& v) const;

		float3 fixOrientation(const float3& v) const;

		Quaternion fixOrientation(const Quaternion& v) const;

		void writeBillboardVertices(const AABB& aabb);

		void writeGeometry();

		void writeBillboardMesh(e_int32 attribute_array_offset, e_int32 indices_offset, const char* mesh_output_filename);

		void writeMeshes(const char* mesh_output_filename);

		void writeSkeleton();

		void writeLODs();

		int getAttributeCount(const ofbx::Mesh& mesh) const;

		bool areIndices16Bit(const ImportMesh& mesh) const;

		void writeModelHeader();

		void writePhysicsHeader(FS::OsFile& file) const;

		void writePhysicsTriMesh(FS::OsFile& file);

		bool writePhysics();

		bool save(const char* output_dir, const char* output_mesh_filename, const char* texture_output_dir);

		void writeModel(const char* output_dir, const char* output_mesh_filename);

		void clearSources();

		void toggleOpen();
		bool isOpen() const;
	public:
		ImportAssetDialog& dialog;
		bool open;
		TArrary<ImportMaterial> materials;
		TArrary<ImportMesh> meshes;
		TArrary<ImportAnimation> animations;
		TArrary<const ofbx::Object*> bones;
		TArrary<ofbx::IScene*> scenes;
		float lods_distances[4];
		FS::OsFile out_file;
		float mesh_scale;
		float time_scale;
		float position_error;
		float rotation_error;
		float bounding_shape_scale;
		bool to_dds;
		bool center_mesh;
		bool ignore_skeleton;
		bool import_vertex_colors;
		bool make_convex;
		bool create_billboard_lod;
		Orientation orientation;
		Orientation root_orientation;
	};
}
#endif
