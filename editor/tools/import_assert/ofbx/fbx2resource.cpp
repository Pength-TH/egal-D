#include "editor/tools/import_assert/ofbx/fbx2resource.h"
#include "common/egal-d.h"
#include "common/filesystem/binary.h"

#include "common/thread/task.h"

#include "editor/tools/base/platform_interface.h"
#include "editor/tools/import_assert/import_asset_dialog.h"
#include "ofbx.h"

namespace egal
{
	typedef StaticString<MAX_PATH_LENGTH> PathBuilder;

	static e_uint32 packF4u(const float3& vec)
	{
		const e_uint8 xx = e_uint8(vec.x * 127.0f + 128.0f);
		const e_uint8 yy = e_uint8(vec.y * 127.0f + 128.0f);
		const e_uint8 zz = e_uint8(vec.z * 127.0f + 128.0f);
		const e_uint8 ww = e_uint8(0);

		union
		{
			e_uint32 ui32;
			e_uint8 arr[4];
		} un;

		un.arr[0] = xx;
		un.arr[1] = yy;
		un.arr[2] = zz;
		un.arr[3] = ww;

		return un.ui32;
	}
	
	struct BillboardSceneData
	{
#pragma pack(1)
		struct Vertex
		{
			float3 pos;
			e_uint8 normal[4];
			e_uint8 tangent[4];
			float2 uv;
		};
#pragma pack()

		static const int TEXTURE_SIZE = 512;

		int width;
		int height;
		float ortho_size;
		float3 position;

		static int ceilPowOf2(int value)
		{
			ASSERT(value > 0);
			int ret = value - 1;
			ret |= ret >> 1;
			ret |= ret >> 2;
			ret |= ret >> 3;
			ret |= ret >> 8;
			ret |= ret >> 16;
			return ret + 1;
		}

		BillboardSceneData(const AABB& aabb, int texture_size)
		{
			float3 size = aabb._max - aabb._min;
			float right = aabb._max.x + size.z + size.x + size.z;
			float left = aabb._min.x;
			position.set((right + left) * 0.5f, (aabb._max.y + aabb._min.y) * 0.5f, aabb._max.z + 5);

			if (2 * size.x + 2 * size.z > size.y)
			{
				width = texture_size;
				int nonceiled_height = int(width / (2 * size.x + 2 * size.z) * size.y);
				height = ceilPowOf2(nonceiled_height);
				ortho_size = size.y * height / nonceiled_height * 0.5f;
			}
			else
			{
				height = texture_size;
				width = ceilPowOf2(int(height * (2 * size.x + 2 * size.z) / size.y));
				ortho_size = size.y * 0.5f;
			}
		}

		Matrix computeMVPMatrix()
		{
			Matrix mvp = Matrix::IDENTITY;

			float ratio = height > 0 ? (float)width / height : 1.0f;
			Matrix proj;
			proj.setOrtho(-ortho_size * ratio,
				ortho_size * ratio,
				-ortho_size,
				ortho_size,
				0.0001f,
				10000.0f,
				false /* we do not care for z value, so both true and false are correct*/);

			mvp.setTranslation(position);
			mvp.fastInverse();
			mvp = proj * mvp;

			return mvp;
		}
	};

	enum class VertexAttributeDef : e_uint32
	{
		POSITION,
		FLOAT1,
		FLOAT2,
		FLOAT3,
		FLOAT4,
		INT1,
		INT2,
		INT3,
		INT4,
		SHORT2,
		SHORT4,
		BYTE4,
		NONE
	};

	static void preprocessBillboardNormalmap(e_uint32* pixels, int width, int height, IAllocator& allocator)
	{
		union {
			e_uint32 ui32;
			e_uint8 arr[4];
		} un;
		for (int j = 0; j < height; ++j)
		{
			for (int i = 0; i < width; ++i)
			{
				un.ui32 = pixels[i + j * width];
				e_uint8 tmp = un.arr[1];
				un.arr[1] = un.arr[2];
				un.arr[2] = tmp;
				pixels[i + j * width] = un.ui32;
			}
		}
	}

	static void preprocessBillboard(e_uint32* pixels, int width, int height, IAllocator& allocator)
	{
		struct DistanceFieldCell
		{
			e_uint32 distance;
			e_uint32 color;
		};

		TArrary<DistanceFieldCell> distance_field(allocator);
		distance_field.resize(width * height);

		static const e_uint32 ALPHA_MASK = 0xff000000;

		for (int j = 0; j < height; ++j)
		{
			for (int i = 0; i < width; ++i)
			{
				distance_field[i + j * width].color = pixels[i + j * width];
				distance_field[i + j * width].distance = 0xffffFFFF;
			}
		}

		for (int j = 1; j < height; ++j)
		{
			for (int i = 1; i < width; ++i)
			{
				int idx = i + j * width;
				if ((pixels[idx] & ALPHA_MASK) != 0)
				{
					distance_field[idx].distance = 0;
				}
				else
				{
					if (distance_field[idx - 1].distance < distance_field[idx - width].distance)
					{
						distance_field[idx].distance = distance_field[idx - 1].distance + 1;
						distance_field[idx].color =
							(distance_field[idx - 1].color & ~ALPHA_MASK) | (distance_field[idx].color & ALPHA_MASK);
					}
					else
					{
						distance_field[idx].distance = distance_field[idx - width].distance + 1;
						distance_field[idx].color =
							(distance_field[idx - width].color & ~ALPHA_MASK) | (distance_field[idx].color & ALPHA_MASK);
					}
				}
			}
		}

		for (int j = height - 2; j >= 0; --j)
		{
			for (int i = width - 2; i >= 0; --i)
			{
				int idx = i + j * width;
				if (distance_field[idx + 1].distance < distance_field[idx + width].distance &&
					distance_field[idx + 1].distance < distance_field[idx].distance)
				{
					distance_field[idx].distance = distance_field[idx + 1].distance + 1;
					distance_field[idx].color =
						(distance_field[idx + 1].color & ~ALPHA_MASK) | (distance_field[idx].color & ALPHA_MASK);
				}
				else if (distance_field[idx + width].distance < distance_field[idx].distance)
				{
					distance_field[idx].distance = distance_field[idx + width].distance + 1;
					distance_field[idx].color =
						(distance_field[idx + width].color & ~ALPHA_MASK) | (distance_field[idx].color & ALPHA_MASK);
				}
			}
		}

		for (int j = 0; j < height; ++j)
		{
			for (int i = 0; i < width; ++i)
			{
				pixels[i + j * width] = distance_field[i + j*width].color;
			}
		}

	}

	//**************************************************************************************
	//**************************************************************************************
	//**************************************************************************************
	ResourceSerializer::ResourceSerializer(ImportAssetDialog& _dialog)
		: dialog(_dialog)
		, scenes(*g_allocator)
		, materials(*g_allocator)
		, meshes(*g_allocator)
		, animations(*g_allocator)
		, bones(*g_allocator)
	{
		open = false;
		lods_distances[0] = -10;
		lods_distances[1] = -100;
		lods_distances[2] = -1000;
		lods_distances[3] = -10000;
		mesh_scale = 1.0f;
		time_scale = 1.0f;
		position_error = 0.1f;
		rotation_error = 0.01f;
		bounding_shape_scale = 1.0f;
		to_dds = true;
		center_mesh = false;
		ignore_skeleton = false;
		import_vertex_colors = true;
		make_convex = false;
		create_billboard_lod = false;
		orientation = Orientation::Y_UP;
		root_orientation = Orientation::Y_UP;
	}

	ResourceSerializer::~ResourceSerializer()
	{

	}

	ofbx::Matrix ResourceSerializer::makeOFBXIdentity() { return { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }; }

	const char* ResourceSerializer::getImportMeshName(const ImportMesh& mesh)
	{
		const char* name = mesh.fbx->name;
		const ofbx::Material* material = mesh.fbx_mat;

		if (name[0] == '\0' && mesh.fbx->getParent()) name = mesh.fbx->getParent()->name;
		if (name[0] == '\0' && material) name = material->name;
		return name;
	}

	void ResourceSerializer::insertHierarchy(TArrary<const ofbx::Object*>& bones, const ofbx::Object* node)
	{
		if (!node) return;
		if (bones.indexOf(node) >= 0) return;
		ofbx::Object* parent = node->getParent();
		insertHierarchy(bones, parent);
		bones.push_back(node);
	}

	ofbx::Matrix ResourceSerializer::getBindPoseMatrix(const ofbx::Mesh* mesh, const ofbx::Object* node)
	{
		if (!mesh) return makeOFBXIdentity();

		auto* skin = mesh->getGeometry()->getSkin();

		for (int i = 0, c = skin->getClusterCount(); i < c; ++i)
		{
			const ofbx::Cluster* cluster = skin->getCluster(i);
			if (cluster->getLink() == node)
			{
				return cluster->getTransformLinkMatrix();
			}
		}
		ASSERT(false);
		return makeOFBXIdentity();
	}

	void ResourceSerializer::makeValidFilename(char* filename)
	{
		char* c = filename;
		while (*c)
		{
			bool is_valid = (*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9') || *c == '-' || *c == '_';
			if (!is_valid) *c = '_';
			++c;
		}
	}

	int ResourceSerializer::findSubblobIndex(const WriteBinary& haystack, const WriteBinary& needle, const TArrary<int>& subblobs, int first_subblob)
	{
		const e_uint8* data = (const e_uint8*)haystack.getData();
		const e_uint8* needle_data = (const e_uint8*)needle.getData();
		int step_size = needle.getPos();
		int idx = first_subblob;
		while (idx != -1)
		{
			if (StringUnitl::compareMemory(data + idx * step_size, needle_data, step_size) == 0) return idx;
			idx = subblobs[idx];
		}
		return -1;
	}

	void ResourceSerializer::writeUV(const ofbx::Vec2& uv, WriteBinary* blob)
	{
		float2 tex_cooords = { (float)uv.x, 1 - (float)uv.y };
		blob->write(tex_cooords);
	}

	void ResourceSerializer::writeColor(const ofbx::Vec4& color, WriteBinary* blob)
	{
		e_uint8 rgba[4];
		rgba[0] = e_uint8(color.x * 255);
		rgba[1] = e_uint8(color.y * 255);
		rgba[2] = e_uint8(color.z * 255);
		rgba[3] = e_uint8(color.w * 255);
		blob->write(rgba);
	}

	void ResourceSerializer::writeSkin(const Skin& skin, WriteBinary* blob)
	{
		blob->write(skin.joints);
		blob->write(skin.weights);
		float sum = skin.weights[0] + skin.weights[1] + skin.weights[2] + skin.weights[3];
		ASSERT(sum > 0.99f && sum < 1.01f);
	}

	int ResourceSerializer::getMaterialIndex(const ofbx::Mesh& mesh, const ofbx::Material& material)
	{
		for (int i = 0, c = mesh.getMaterialCount(); i < c; ++i)
		{
			if (mesh.getMaterial(i) == &material) return i;
		}
		return -1;
	}

	int ResourceSerializer::detectMeshLOD(const ImportMesh& mesh)
	{
		const char* node_name = mesh.fbx->name;
		const char* lod_str = StringUnitl::stristr(node_name, "_LOD");
		if (!lod_str)
		{
			const char* mesh_name = getImportMeshName(mesh);
			if (!mesh_name) return 0;

			const char* lod_str = StringUnitl::stristr(mesh_name, "_LOD");
			if (!lod_str) return 0;
		}

		lod_str += StringUnitl::stringLength("_LOD");

		int lod;
		StringUnitl::fromCString(lod_str, StringUnitl::stringLength(lod_str), &lod);

		return lod;
	}
	float3 ResourceSerializer::toLumixfloat3(const ofbx::Vec4& v) { return { (float)v.x, (float)v.y, (float)v.z }; }
	float3 ResourceSerializer::toLumixfloat3(const ofbx::Vec3& v) { return { (float)v.x, (float)v.y, (float)v.z }; }
	Quaternion ResourceSerializer::toLumix(const ofbx::Quat& q) { return { (float)q.x, (float)q.y, (float)q.z, (float)q.w }; }
	Matrix ResourceSerializer::toLumix(const ofbx::Matrix& mtx)
	{
		Matrix res;

		for (int i = 0; i < 16; ++i) (&res.m11)[i] = (float)mtx.m[i];

		return res;
	}
	void ResourceSerializer::getRelativePath(char* relative_path, int max_length, const char* source)
	{
		char tmp[MAX_PATH_LENGTH];
		StringUnitl::normalize(source, tmp, sizeof(tmp));

		const char* base_path = getDiskBasePath();
		if (StringUnitl::compareStringN(base_path, tmp, StringUnitl::stringLength(base_path)) == 0)
		{
			int base_path_length = StringUnitl::stringLength(base_path);
			const char* rel_path_start = tmp + base_path_length;
			if (rel_path_start[0] == '/')
			{
				++rel_path_start;
			}
			StringUnitl::copyString(relative_path, max_length, rel_path_start);
		}
		else
		{
			const char* base_path = getDiskBasePath();
			if (base_path && StringUnitl::compareStringN(base_path, tmp, StringUnitl::stringLength(base_path)) == 0)
			{
				int base_path_length = StringUnitl::stringLength(base_path);
				const char* rel_path_start = tmp + base_path_length;
				if (rel_path_start[0] == '/')
				{
					++rel_path_start;
				}
				StringUnitl::copyString(relative_path, max_length, rel_path_start);
			}
			else
			{
				StringUnitl::copyString(relative_path, max_length, tmp);
			}
		}
	}

	float3 ResourceSerializer::getTranslation(const ofbx::Matrix& mtx)
	{
		return { (float)mtx.m[12], (float)mtx.m[13], (float)mtx.m[14] };
	}

	Quaternion ResourceSerializer::getRotation(const ofbx::Matrix& mtx)
	{
		Matrix m = toLumix(mtx);
		m.normalizeScale();
		return m.getRotation();
	}

	// arg parent_scale - animated scale is not supported, but we can get rid of static scale if we ignore
	// it in writeSkeleton() and use parent_scale in this function
	void ResourceSerializer::compressPositions(TArrary<TranslationKey>& out,
		int from_frame,
		int to_frame,
		float sample_period,
		const ofbx::AnimationCurveNode* curve_node,
		const ofbx::Object& bone,
		float error,
		float parent_scale)
	{
		out.clear();
		if (!curve_node) return;
		if (to_frame == from_frame) return;

		ofbx::Vec3 lcl_rotation = bone.getLocalRotation();
		float3 pos = getTranslation(bone.evalLocal(curve_node->getNodeLocalTransform(from_frame * sample_period), lcl_rotation)) * parent_scale;
		TranslationKey last_written = { pos, 0, 0 };
		out.push_back(last_written);
		if (to_frame == from_frame + 1) return;

		pos = getTranslation(bone.evalLocal(curve_node->getNodeLocalTransform((from_frame + 1) * sample_period), lcl_rotation)) *
			parent_scale;
		float3 dif = (pos - last_written.pos) / sample_period;
		TranslationKey prev = { pos, sample_period, 1 };
		float dt;
		for (e_uint16 i = 2; i < e_uint16(to_frame - from_frame); ++i)
		{
			float t = i * sample_period;
			float3 cur =
				getTranslation(bone.evalLocal(curve_node->getNodeLocalTransform((from_frame + i) * sample_period), lcl_rotation)) * parent_scale;
			dt = t - last_written.time;
			float3 estimate = last_written.pos + dif * dt;
			if (fabs(estimate.x - cur.x) > error || fabs(estimate.y - cur.y) > error ||
				fabs(estimate.z - cur.z) > error)
			{
				last_written = prev;
				out.push_back(last_written);

				dt = sample_period;
				dif = (cur - last_written.pos) / dt;
			}
			prev = { cur, t, i };
		}

		float t = (to_frame - from_frame) * sample_period;
		last_written = {
			getTranslation(bone.evalLocal(curve_node->getNodeLocalTransform(to_frame * sample_period), lcl_rotation)) * parent_scale,
			t,
			e_uint16(to_frame - from_frame) };
		out.push_back(last_written);
	}

	void ResourceSerializer::compressRotations(TArrary<RotationKey>& out,
		int from_frame,
		int to_frame,
		float sample_period,
		const ofbx::AnimationCurveNode* curve_node,
		const ofbx::Object& bone,
		float error)
	{
		out.clear();
		if (!curve_node) return;
		if (to_frame == from_frame) return;

		ofbx::Vec3 lcl_translation = bone.getLocalTranslation();
		Quaternion rot = getRotation(bone.evalLocal(lcl_translation, curve_node->getNodeLocalTransform(from_frame * sample_period)));
		RotationKey last_written = { rot, 0, 0 };
		out.push_back(last_written);
		if (to_frame == from_frame + 1) return;

		rot = getRotation(bone.evalLocal(lcl_translation, curve_node->getNodeLocalTransform((from_frame + 1) * sample_period)));
		RotationKey after_last = { rot, sample_period, 1 };
		RotationKey prev = after_last;
		for (e_uint16 i = 2; i < e_uint16(to_frame - from_frame); ++i)
		{
			float t = i * sample_period;
			Quaternion cur = getRotation(bone.evalLocal(lcl_translation, curve_node->getNodeLocalTransform((from_frame + i) * sample_period)));
			Quaternion estimate;
			nlerp(cur, last_written.rot, &estimate, sample_period / (t - last_written.time));
			if (fabs(estimate.x - after_last.rot.x) > error || fabs(estimate.y - after_last.rot.y) > error ||
				fabs(estimate.z - after_last.rot.z) > error)
			{
				last_written = prev;
				out.push_back(last_written);

				after_last = { cur, t, i };
			}
			prev = { cur, t, i };
		}

		float t = (to_frame - from_frame) * sample_period;
		last_written = {
			getRotation(bone.evalLocal(lcl_translation, curve_node->getNodeLocalTransform(to_frame * sample_period))),
			t,
			e_uint16(to_frame - from_frame) };
		out.push_back(last_written);
	}

	float ResourceSerializer::getScaleX(const ofbx::Matrix& mtx)
	{
		float3 v(float(mtx.m[0]), float(mtx.m[4]), float(mtx.m[8]));

		return v.length();
	}

	int ResourceSerializer::getDepth(const ofbx::Object* bone)
	{
		int depth = 0;
		while (bone)
		{
			++depth;
			bone = bone->getParent();
		}
		return depth;
	}
	void ResourceSerializer::getMaterialName(const ofbx::Material* material, char(&out)[128])
	{
		StringUnitl::copyString(out, material ? material->name : "default");
		char* iter = out;
		while (*iter)
		{
			char c = *iter;
			if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
			{
				*iter = '_';
			}
			++iter;
		}
	}


	const ofbx::Mesh* ResourceSerializer::getAnyMeshFromBone(const ofbx::Object* node) const
	{
		for (int i = 0; i < meshes.size(); ++i)
		{
			const ofbx::Mesh* mesh = meshes[i].fbx;

			auto* skin = mesh->getGeometry()->getSkin();
			if (!skin) continue;

			for (int j = 0, c = skin->getClusterCount(); j < c; ++j)
			{
				if (skin->getCluster(j)->getLink() == node) return mesh;
			}
		}
		return nullptr;
	}

	void ResourceSerializer::gatherMaterials(const ofbx::Object* node, const char* src_dir)
	{
		for (ImportMesh& mesh : meshes)
		{
			const ofbx::Material* fbx_mat = mesh.fbx_mat;
			if (!fbx_mat) continue;

			ImportMaterial& mat = materials.emplace();
			mat.fbx = fbx_mat;

			auto gatherTexture = [&mat, src_dir](ofbx::Texture::TextureType type) {
				const ofbx::Texture* texture = mat.fbx->getTexture(type);
				if (!texture) return;

				ImportTexture& tex = mat.textures[type];
				tex.fbx = texture;
				ofbx::DataView filename = tex.fbx->getRelativeFileName();
				if (filename == "") filename = tex.fbx->getFileName();
				filename.toString(tex.path.data);
				tex.src = tex.path;
				tex.is_valid = PlatformInterface::fileExists(tex.src);

				if (!tex.is_valid)
				{
					FileInfo file_info(tex.path);
					tex.src = src_dir;
					tex.src << file_info.m_basename << "." << file_info.m_extension;
					tex.is_valid = PlatformInterface::fileExists(tex.src);

					if (!tex.is_valid)
					{
						tex.src = src_dir;
						tex.src << tex.path;
						tex.is_valid = PlatformInterface::fileExists(tex.src);
					}
				}

				tex.import = true;
				tex.to_dds = true;
			};

			gatherTexture(ofbx::Texture::DIFFUSE);
			gatherTexture(ofbx::Texture::NORMAL);
		}
	}

	void ResourceSerializer::sortBones()
	{
		int count = bones.size();
		for (int i = 0; i < count; ++i)
		{
			for (int j = i + 1; j < count; ++j)
			{
				if (bones[i]->getParent() == bones[j])
				{
					const ofbx::Object* bone = bones[j];
					bones.eraseFast(j);
					bones.insert(i, bone);
					--i;
					break;
				}
			}
		}
	}

	void ResourceSerializer::gatherBones(const ofbx::IScene& scene)
	{
		for (const ImportMesh& mesh : meshes)
		{
			const ofbx::Skin* skin = mesh.fbx->getGeometry()->getSkin();
			if (skin)
			{
				for (int i = 0; i < skin->getClusterCount(); ++i)
				{
					const ofbx::Cluster* cluster = skin->getCluster(i);
					insertHierarchy(bones, cluster->getLink());
				}
			}
		}

		bones.removeDuplicates();
		sortBones();
	}

	void ResourceSerializer::gatherAnimations(const ofbx::IScene& scene)
	{
		int anim_count = scene.getAnimationStackCount();
		for (int i = 0; i < anim_count; ++i)
		{
			IAllocator& allocator = *g_allocator;
			ImportAnimation& anim = animations.emplace(allocator);
			anim.scene = &scene;
			anim.fbx = (const ofbx::AnimationStack*)scene.getAnimationStack(i);
			anim.import = true;
			const ofbx::TakeInfo* take_info = scene.getTakeInfo(anim.fbx->name);
			if (take_info)
			{
				if (take_info->name.begin != take_info->name.end)
				{
					take_info->name.toString(anim.output_filename.data);
				}
				if (anim.output_filename.empty() && take_info->filename.begin != take_info->filename.end)
				{
					take_info->filename.toString(anim.output_filename.data);
				}
				if (anim.output_filename.empty()) anim.output_filename << "anim";
			}
			else
			{
				anim.output_filename = "anim";
			}

			makeValidFilename(anim.output_filename.data);
		}
	}

	void ResourceSerializer::writePackedfloat3(const ofbx::Vec3& vec, const Matrix& mtx, WriteBinary* blob) const
	{
		float3 v = toLumixfloat3(vec);
		v = mtx * float4(v, 0);
		v.normalize();
		v = fixOrientation(v);

		e_uint32 packed = packF4u(v);
		blob->write(packed);
	}

	void ResourceSerializer::postprocessMeshes()
	{
		for (int mesh_idx = 0; mesh_idx < meshes.size(); ++mesh_idx)
		{
			ImportMesh& import_mesh = meshes[mesh_idx];
			dialog.setImportMessage("Processing meshes...", (mesh_idx / (float)meshes.size()) * 0.4f);
			import_mesh.vertex_data.clear();
			import_mesh.indices.clear();

			const ofbx::Mesh& mesh = *import_mesh.fbx;
			const ofbx::Geometry* geom = import_mesh.fbx->getGeometry();
			int vertex_count = geom->getVertexCount();
			const ofbx::Vec3* vertices = geom->getVertices();
			const ofbx::Vec3* normals = geom->getNormals();
			const ofbx::Vec3* tangents = geom->getTangents();
			const ofbx::Vec4* colors = import_vertex_colors ? geom->getColors() : nullptr;
			const ofbx::Vec2* uvs = geom->getUVs();

			Matrix transform_matrix = Matrix::IDENTITY;
			Matrix geometry_matrix = toLumix(mesh.getGeometricMatrix());
			transform_matrix = toLumix(mesh.getGlobalTransform()) * geometry_matrix;
			if (center_mesh) transform_matrix.setTranslation({ 0, 0, 0 });

			IAllocator& allocator = *g_allocator;
			WriteBinary blob(allocator);
			int vertex_size = getVertexSize(mesh);
			import_mesh.vertex_data.reserve(vertex_count * vertex_size);

			TArrary<Skin> skinning(allocator);
			bool is_skinned = isSkinned(mesh);
			if (is_skinned) fillSkinInfo(skinning, &mesh);

			AABB aabb = { {0, 0, 0}, {0, 0, 0} };
			float radius_squared = 0;

			int material_idx = getMaterialIndex(mesh, *import_mesh.fbx_mat);
			assert(material_idx >= 0);

			int first_subblob[256];
			for (int& subblob : first_subblob) subblob = -1;
			TArrary<int> subblobs(allocator);
			subblobs.reserve(vertex_count);

			const int* materials = geom->getMaterials();
			for (int i = 0; i < vertex_count; ++i)
			{
				if (materials && materials[i / 3] != material_idx) continue;

				blob.clear();
				ofbx::Vec3 cp = vertices[i];

				// premultiply control points here, so we can have constantly-scaled meshes without scale in bones
				float3 pos = transform_matrix.transformPoint(toLumixfloat3(cp)) * mesh_scale;
				pos = fixOrientation(pos);
				blob.write(pos);

				float sq_len = pos.squaredLength();
				radius_squared = Math::maximum(radius_squared, sq_len);

				aabb._min.x = Math::minimum(aabb._min.x, pos.x);
				aabb._min.y = Math::minimum(aabb._min.y, pos.y);
				aabb._min.z = Math::minimum(aabb._min.z, pos.z);
				aabb._max.x = Math::maximum(aabb._max.x, pos.x);
				aabb._max.y = Math::maximum(aabb._max.y, pos.y);
				aabb._max.z = Math::maximum(aabb._max.z, pos.z);

				if (normals) writePackedfloat3(normals[i], transform_matrix, &blob);
				if (uvs) writeUV(uvs[i], &blob);
				if (colors) writeColor(colors[i], &blob);
				if (tangents) writePackedfloat3(tangents[i], transform_matrix, &blob);
				if (is_skinned) writeSkin(skinning[i], &blob);

				e_uint8 first_byte = ((const e_uint8*)blob.getData())[0];

				int idx = findSubblobIndex(import_mesh.vertex_data, blob, subblobs, first_subblob[first_byte]);
				if (idx == -1)
				{
					subblobs.push_back(first_subblob[first_byte]);
					first_subblob[first_byte] = subblobs.size() - 1;
					import_mesh.indices.push_back(import_mesh.vertex_data.getPos() / vertex_size);
					import_mesh.vertex_data.write(blob.getData(), vertex_size);
				}
				else
				{
					import_mesh.indices.push_back(idx);
				}
			}

			import_mesh.aabb = aabb;
			import_mesh.radius_squared = radius_squared;
		}
		for (int mesh_idx = meshes.size() - 1; mesh_idx >= 0; --mesh_idx)
		{
			if (meshes[mesh_idx].indices.empty()) meshes.eraseFast(mesh_idx);
		}
	}

	void ResourceSerializer::gatherMeshes(ofbx::IScene* scene)
	{
		int min_lod = 2;
		int c = scene->getMeshCount();
		IAllocator& allocator = *g_allocator;
		int start_index = meshes.size();
		for (int i = 0; i < c; ++i)
		{
			const ofbx::Mesh* fbx_mesh = (const ofbx::Mesh*)scene->getMesh(i);
			if (fbx_mesh->getGeometry()->getVertexCount() == 0) continue;
			for (int j = 0; j < fbx_mesh->getMaterialCount(); ++j)
			{
				ImportMesh& mesh = meshes.emplace(allocator);
				mesh.fbx = fbx_mesh;
				mesh.fbx_mat = fbx_mesh->getMaterial(j);
				mesh.lod = detectMeshLOD(mesh);
				min_lod = Math::minimum(min_lod, mesh.lod);
			}
		}
		if (min_lod != 1) return;
		for (int i = start_index, n = meshes.size(); i < n; ++i)
		{
			--meshes[i].lod;
		}
	}

	bool ResourceSerializer::addSource(const char* filename)
	{
		IAllocator& allocator = *g_allocator;

		FS::OsFile file;
		if (!file.open(filename, FS::Mode::OPEN_AND_READ)) return false;

		TArrary<e_uint8> data(allocator);
		data.resize((int)file.size());

		if (!file.read(&data[0], data.size()))
		{
			file.close();
			return false;
		}
		file.close();

		ofbx::IScene* scene = ofbx::load(&data[0], data.size());
		if (!scene)
		{
			log_error("FBX Failed to import %s:%s", filename, ofbx::getError());
			return false;
		}

		const ofbx::Object* root = scene->getRoot();
		char src_dir[MAX_PATH_LENGTH];
		StringUnitl::getDir(src_dir, TlengthOf(src_dir), filename);
		gatherMeshes(scene);
		gatherMaterials(root, src_dir);
		materials.removeDuplicates([](const ImportMaterial& a, const ImportMaterial& b) { return a.fbx == b.fbx; });
		gatherBones(*scene);
		gatherAnimations(*scene);

		scenes.push_back(scene);
		return true;
	}

	template <typename T> void ResourceSerializer::write(const T& obj) { out_file.write(&obj, sizeof(obj)); }
	void ResourceSerializer::write(const void* ptr, size_t size) { out_file.write(ptr, size); }
	void ResourceSerializer::writeString(const char* str) { out_file.write(str, strlen(str)); }

	bool ResourceSerializer::writeBillboardMaterial(const char* output_dir, const char* texture_output_dir, const char* mesh_output_filename)
	{
		if (!create_billboard_lod) return true;

		FS::OsFile file;
		PathBuilder output_material_name(output_dir, "/", mesh_output_filename, "_billboard.mat");
		if (!file.open(output_material_name, FS::Mode::CREATE_AND_WRITE))
		{
			log_error("FBX Failed to create %s", output_material_name);
			return false;
		}
		file << "{\n\t\"shader\" : \"pipelines/rigid/rigid.shd\"\n";
		file << "\t, \"defines\" : [\"ALPHA_CUTOUT\"]\n";
		file << "\t, \"texture\" : {\n\t\t\"source\" : \"";

		if (texture_output_dir[0])
		{
			char from_root_path[MAX_PATH_LENGTH];
			getRelativePath(from_root_path, TlengthOf(from_root_path), texture_output_dir);
			PathBuilder relative_texture_path(from_root_path, mesh_output_filename, "_billboard.dds");
			PathBuilder texture_path(texture_output_dir, mesh_output_filename, "_billboard.dds");
			copyFile("models/utils/cube/default.dds", texture_path);

			file << "/" << relative_texture_path.data << "\"}\n\t, \"texture\" : {\n\t\t\"source\" : \"";

			PathBuilder relative_normal_path_n(from_root_path, mesh_output_filename, "_billboard_normal.dds");
			PathBuilder normal_path(texture_output_dir, mesh_output_filename, "_billboard_normal.dds");

			copyFile("models/utils/cube/default.dds", normal_path);

			file << "/" << relative_normal_path_n.data;

		}
		else
		{
			file << mesh_output_filename << "_billboard.dds\"}\n\t, \"texture\" : {\n\t\t\"source\" : \"";

			PathBuilder texture_path(output_dir, "/", mesh_output_filename, "_billboard.dds");
			copyFile("models/utils/cube/default.dds", texture_path);

			file << mesh_output_filename << "_billboard_normal.dds";
			PathBuilder normal_path(output_dir, "/", mesh_output_filename, "_billboard_normal.dds");

			copyFile("models/utils/cube/default.dds", normal_path);
		}

		file << "\"}\n}";
		file.close();
		return true;
	}

	void ResourceSerializer::writeMaterials(const char* output_dir, const char* texture_output_dir, const char* mesh_output_filename)
	{
		dialog.setImportMessage("Writing materials...", 0.9f);

		for (const ImportMaterial& material : materials)
		{
			if (!material.import) continue;

			char mat_name[128];
			getMaterialName(material.fbx, mat_name);
			StaticString<MAX_PATH_LENGTH> path(output_dir, mat_name, ".mat");
			if (!out_file.open(path, FS::Mode::CREATE_AND_WRITE))
			{
				log_error("FBX Failed to create %s.", path);
				continue;
			}

			writeString("{\n\t\"shader\" : \"pipelines/rigid/rigid.shd\"");
			if (material.alpha_cutout) writeString(",\n\t\"defines\" : [\"ALPHA_CUTOUT\"]");
			auto writeTexture = [this, texture_output_dir](const ImportTexture& texture, bool srgb) {
				if (texture.fbx)
				{
					writeString(",\n\t\"texture\" : { \"source\" : \"");
					FileInfo info(texture.src);
					writeString(texture_output_dir);
					writeString(info.m_basename);
					writeString(".");
					writeString(texture.to_dds ? "dds" : info.m_extension);
					writeString("\"");
					if (srgb) writeString(", \"srgb\" : true ");
					writeString("}");
				}
				else
				{
					writeString(",\n\t\"texture\" : {");
					if (srgb) writeString(" \"srgb\" : true ");
					writeString("}");
				}
			};

			writeTexture(material.textures[0], true);
			writeTexture(material.textures[1], false);

			ofbx::Color diffuse_color = material.fbx->getDiffuseColor();
			out_file << ",\n\t\"color\" : [" << diffuse_color.r
				<< "," << diffuse_color.g
				<< "," << diffuse_color.b
				<< ",1]";

			writeString("\n}");

			out_file.close();
		}
		writeBillboardMaterial(output_dir, texture_output_dir, mesh_output_filename);
	}

	void ResourceSerializer::writeAnimations(const char* output_dir)
	{
		for (int anim_idx = 0; anim_idx < animations.size(); ++anim_idx)
		{
			ImportAnimation& anim = animations[anim_idx];
			dialog.setImportMessage("Writing animation...", 0.6f + 0.2f * (anim_idx / (float)animations.size()));
			if (!anim.import) continue;
			const ofbx::AnimationStack* stack = anim.fbx;
			const ofbx::IScene& scene = *anim.scene;
			const ofbx::TakeInfo* take_info = scene.getTakeInfo(stack->name);

			float fbx_frame_rate = scene.getSceneFrameRate();
			if (fbx_frame_rate < 0) fbx_frame_rate = 24;
			float scene_frame_rate = fbx_frame_rate / time_scale;
			float sampling_period = 1.0f / scene_frame_rate;
			int all_frames_count = 0;
			if (take_info)
			{
				all_frames_count = int((take_info->local_time_to - take_info->local_time_from) / sampling_period + 0.5f);
			}
			else
			{
				ASSERT(false);
				// TODO
				// scene->GetGlobalSettings().GetTimelineDefaultTimeSpan(time_spawn);
			}

			// TODO
			/*FbxTime::EMode mode = scene->GetGlobalSettings().GetTimeMode();
			float scene_frame_rate =
			(float)((mode == FbxTime::eCustom) ? scene->GetGlobalSettings().GetCustomFrameRate()
			: FbxTime::GetFrameRate(mode));
			*/
			for (int i = 0; i < Math::maximum(1, anim.splits.size()); ++i)
			{
				ImportAnimation::Split whole_anim_split;
				whole_anim_split.to_frame = all_frames_count;
				auto* split = anim.splits.empty() ? &whole_anim_split : &anim.splits[i];

				int frame_count = split->to_frame - split->from_frame;

				StaticString<MAX_PATH_LENGTH> tmp(output_dir, anim.output_filename, split->name, ".ani");
				IAllocator& allocator = *g_allocator;
				if (!out_file.open(tmp, FS::Mode::CREATE_AND_WRITE))
				{
					log_error("FBX Failed to create %s.", tmp);
					continue;
				}
				/*Animation::Header header;
				header.magic = Animation::HEADER_MAGIC;
				header.version = 3;
				header.fps = (e_uint32)(scene_frame_rate + 0.5f);
				write(header);

				write(anim.root_motion_bone_idx);
				write(frame_count);
				int used_bone_count = 0;

				for (const ofbx::Object* bone : bones)
				{
					if (&bone->getScene() != &scene) continue;

					const ofbx::AnimationLayer* layer = stack->getLayer(0);
					const ofbx::AnimationCurveNode* translation_curve_node = layer->getCurveNode(*bone, "Lcl Translation");
					const ofbx::AnimationCurveNode* rotation_curve_node = layer->getCurveNode(*bone, "Lcl Rotation");
					if (translation_curve_node || rotation_curve_node) ++used_bone_count;
				}

				write(used_bone_count);
				TVector<TranslationKey> positions(allocator);
				TVector<RotationKey> rotations(allocator);
				for (const ofbx::Object* bone : bones)
				{
					if (&bone->getScene() != &scene) continue;
					const ofbx::Object* root_bone = anim.root_motion_bone_idx >= 0 ? bones[anim.root_motion_bone_idx] : nullptr;

					const ofbx::AnimationLayer* layer = stack->getLayer(0);
					const ofbx::AnimationCurveNode* translation_node = layer->getCurveNode(*bone, "Lcl Translation");
					const ofbx::AnimationCurveNode* rotation_node = layer->getCurveNode(*bone, "Lcl Rotation");
					if (!translation_node && !rotation_node) continue;

					e_uint32 name_hash = crc32(bone->name);
					write(name_hash);

					int depth = getDepth(bone);
					float parent_scale = bone->getParent() ? (float)getScaleX(bone->getParent()->getGlobalTransform()) : 1;
					compressPositions(positions, split->from_frame, split->to_frame, sampling_period, translation_node, *bone, position_error / depth, parent_scale);
					write(positions.size());

					for (TranslationKey& key : positions) write(key.frame);
					for (TranslationKey& key : positions)
					{
						if (bone == root_bone)
						{
							write(fixRootOrientation(key.pos * mesh_scale));
						}
						else
						{
							write(fixOrientation(key.pos * mesh_scale));
						}
					}

					compressRotations(rotations, split->from_frame, split->to_frame, sampling_period, rotation_node, *bone, rotation_error / depth);

					write(rotations.size());
					for (RotationKey& key : rotations) write(key.frame);
					for (RotationKey& key : rotations)
					{
						if (bone == root_bone)
						{
							write(fixRootOrientation(key.rot));
						}
						else
						{
							write(fixOrientation(key.rot));
						}
					}
				}*/
				out_file.close();
			}
		}
	}

	bool ResourceSerializer::isSkinned(const ofbx::Mesh& mesh) const
	{
		return !ignore_skeleton && mesh.getGeometry()->getSkin() != nullptr;
	}

	int ResourceSerializer::getVertexSize(const ofbx::Mesh& mesh) const
	{
		static const int POSITION_SIZE = sizeof(float) * 3;
		static const int NORMAL_SIZE = sizeof(e_uint8) * 4;
		static const int TANGENT_SIZE = sizeof(e_uint8) * 4;
		static const int UV_SIZE = sizeof(float) * 2;
		static const int COLOR_SIZE = sizeof(e_uint8) * 4;
		static const int BONE_INDICES_WEIGHTS_SIZE = sizeof(float) * 4 + sizeof(e_uint16) * 4;
		int size = POSITION_SIZE;

		if (mesh.getGeometry()->getNormals()) size += NORMAL_SIZE;
		if (mesh.getGeometry()->getUVs()) size += UV_SIZE;
		if (mesh.getGeometry()->getColors() && import_vertex_colors) size += COLOR_SIZE;
		if (mesh.getGeometry()->getTangents()) size += TANGENT_SIZE;
		if (isSkinned(mesh)) size += BONE_INDICES_WEIGHTS_SIZE;

		return size;
	}

	void ResourceSerializer::fillSkinInfo(TArrary<Skin>& skinning, const ofbx::Mesh* mesh) const
	{
		const ofbx::Geometry* geom = mesh->getGeometry();
		skinning.resize(geom->getVertexCount());

		auto* skin = mesh->getGeometry()->getSkin();
		for (int i = 0, c = skin->getClusterCount(); i < c; ++i)
		{
			const ofbx::Cluster* cluster = skin->getCluster(i);
			if (cluster->getIndicesCount() == 0) continue;
			int joint = bones.indexOf(cluster->getLink());
			ASSERT(joint >= 0);
			const int* cp_indices = cluster->getIndices();
			const double* weights = cluster->getWeights();
			for (int j = 0; j < cluster->getIndicesCount(); ++j)
			{
				int idx = cp_indices[j];
				float weight = (float)weights[j];
				Skin& s = skinning[idx];
				if (s.count < 4)
				{
					s.weights[s.count] = weight;
					s.joints[s.count] = joint;
					++s.count;
				}
				else
				{
					int min = 0;
					for (int m = 1; m < 4; ++m)
					{
						if (s.weights[m] < s.weights[min]) min = m;
					}

					if (s.weights[min] < weight)
					{
						s.weights[min] = weight;
						s.joints[min] = joint;
					}
				}
			}
		}

		for (Skin& s : skinning)
		{
			float sum = 0;
			for (float w : s.weights) sum += w;
			for (float& w : s.weights) w /= sum;
		}
	}

	float3 ResourceSerializer::fixRootOrientation(const float3& v) const
	{
		switch (root_orientation)
		{
		case Orientation::Y_UP: return float3(v.x, v.y, v.z);
		case Orientation::Z_UP: return float3(v.x, v.z, -v.y);
		case Orientation::Z_MINUS_UP: return float3(v.x, -v.z, v.y);
		case Orientation::X_MINUS_UP: return float3(v.y, -v.x, v.z);
		case Orientation::X_UP: return float3(-v.y, v.x, v.z);
		}
		ASSERT(false);
		return float3(v.x, v.y, v.z);
	}

	Quaternion ResourceSerializer::fixRootOrientation(const Quaternion& v) const
	{
		switch (root_orientation)
		{
		case Orientation::Y_UP: return Quaternion(v.x, v.y, v.z, v.w);
		case Orientation::Z_UP: return Quaternion(v.x, v.z, -v.y, v.w);
		case Orientation::Z_MINUS_UP: return Quaternion(v.x, -v.z, v.y, v.w);
		case Orientation::X_MINUS_UP: return Quaternion(v.y, -v.x, v.z, v.w);
		case Orientation::X_UP: return Quaternion(-v.y, v.x, v.z, v.w);
		}
		ASSERT(false);
		return Quaternion(v.x, v.y, v.z, v.w);
	}

	float3 ResourceSerializer::fixOrientation(const float3& v) const
	{
		switch (orientation)
		{
		case Orientation::Y_UP: return float3(v.x, v.y, v.z);
		case Orientation::Z_UP: return float3(v.x, v.z, -v.y);
		case Orientation::Z_MINUS_UP: return float3(v.x, -v.z, v.y);
		case Orientation::X_MINUS_UP: return float3(v.y, -v.x, v.z);
		case Orientation::X_UP: return float3(-v.y, v.x, v.z);
		}
		ASSERT(false);
		return float3(v.x, v.y, v.z);
	}

	Quaternion ResourceSerializer::fixOrientation(const Quaternion& v) const
	{
		switch (orientation)
		{
		case Orientation::Y_UP: return Quaternion(v.x, v.y, v.z, v.w);
		case Orientation::Z_UP: return Quaternion(v.x, v.z, -v.y, v.w);
		case Orientation::Z_MINUS_UP: return Quaternion(v.x, -v.z, v.y, v.w);
		case Orientation::X_MINUS_UP: return Quaternion(v.y, -v.x, v.z, v.w);
		case Orientation::X_UP: return Quaternion(-v.y, v.x, v.z, v.w);
		}
		ASSERT(false);
		return Quaternion(v.x, v.y, v.z, v.w);
	}

	void ResourceSerializer::writeBillboardVertices(const AABB& aabb)
	{
		if (!create_billboard_lod) return;

		bool has_tangents = false;
		for (auto& mesh : meshes)
		{
			if (mesh.import)
			{
				has_tangents = mesh.fbx->getGeometry()->getTangents() != nullptr;
				break;
			}
		}

		float3 max = aabb._max;
		float3 min = aabb._min;
		float3 size = max - min;
		BillboardSceneData data({ min, max }, BillboardSceneData::TEXTURE_SIZE);
		Matrix mtx = data.computeMVPMatrix();
		float3 uv0_min = mtx.transformPoint(min);
		float3 uv0_max = mtx.transformPoint(max);
		float x1_max = 0.0f;
		float x2_max = mtx.transformPoint(float3(max.x + size.z + size.x, 0, 0)).x;
		float x3_max = mtx.transformPoint(float3(max.x + size.z + size.x + size.z, 0, 0)).x;

		auto fixUV = [](float x, float y) -> float2 { return float2(x * 0.5f + 0.5f, y * 0.5f + 0.5f); };

		BillboardSceneData::Vertex vertices[] = {
			{{min.x, min.y, 0}, {128, 255, 128, 0}, {255, 128, 128, 0}, fixUV(uv0_min.x, uv0_max.y)},
			{{max.x, min.y, 0}, {128, 255, 128, 0}, {255, 128, 128, 0}, fixUV(uv0_max.x, uv0_max.y)},
			{{max.x, max.y, 0}, {128, 255, 128, 0}, {255, 128, 128, 0}, fixUV(uv0_max.x, uv0_min.y)},
			{{min.x, max.y, 0}, {128, 255, 128, 0}, {255, 128, 128, 0}, fixUV(uv0_min.x, uv0_min.y)},

			{{0, min.y, min.z}, {128, 255, 128, 0}, {128, 128, 255, 0}, fixUV(uv0_max.x, uv0_max.y)},
			{{0, min.y, max.z}, {128, 255, 128, 0}, {128, 128, 255, 0}, fixUV(x1_max, uv0_max.y)},
			{{0, max.y, max.z}, {128, 255, 128, 0}, {128, 128, 255, 0}, fixUV(x1_max, uv0_min.y)},
			{{0, max.y, min.z}, {128, 255, 128, 0}, {128, 128, 255, 0}, fixUV(uv0_max.x, uv0_min.y)},

			{{max.x, min.y, 0}, {128, 255, 128, 0}, {0, 128, 128, 0}, fixUV(x1_max, uv0_max.y)},
			{{min.x, min.y, 0}, {128, 255, 128, 0}, {0, 128, 128, 0}, fixUV(x2_max, uv0_max.y)},
			{{min.x, max.y, 0}, {128, 255, 128, 0}, {0, 128, 128, 0}, fixUV(x2_max, uv0_min.y)},
			{{max.x, max.y, 0}, {128, 255, 128, 0}, {0, 128, 128, 0}, fixUV(x1_max, uv0_min.y)},

			{{0, min.y, max.z}, {128, 255, 128, 0}, {128, 128, 0, 0}, fixUV(x2_max, uv0_max.y)},
			{{0, min.y, min.z}, {128, 255, 128, 0}, {128, 128, 0, 0}, fixUV(x3_max, uv0_max.y)},
			{{0, max.y, min.z}, {128, 255, 128, 0}, {128, 128, 0, 0}, fixUV(x3_max, uv0_min.y)},
			{{0, max.y, max.z}, {128, 255, 128, 0}, {128, 128, 0, 0}, fixUV(x2_max, uv0_min.y)} };

		int vertex_data_size = sizeof(BillboardSceneData::Vertex);
		if (!has_tangents) vertex_data_size -= 4;
		vertex_data_size *= TlengthOf(vertices);
		write(vertex_data_size);
		for (const BillboardSceneData::Vertex& vertex : vertices)
		{
			write(vertex.pos);
			write(vertex.normal);
			if (has_tangents) write(vertex.tangent);
			write(vertex.uv);
		}
	}

	void ResourceSerializer::writeGeometry()
	{
		AABB aabb = { {0, 0, 0}, {0, 0, 0} };
		float radius_squared = 0;
		IAllocator& allocator = *g_allocator;

		WriteBinary vertices_blob(allocator);
		for (const ImportMesh& import_mesh : meshes)
		{
			if (!import_mesh.import) continue;
			bool are_indices_16_bit = areIndices16Bit(import_mesh);
			if (are_indices_16_bit)
			{
				int index_size = sizeof(e_uint16);
				write(index_size);
				write(import_mesh.indices.size());
				for (int i : import_mesh.indices)
				{
					assert(i <= (1 << 16));
					e_uint16 index = (e_uint16)i;
					write(index);
				}
			}
			else
			{
				int index_size = sizeof(import_mesh.indices[0]);
				write(index_size);
				write(import_mesh.indices.size());
				write(&import_mesh.indices[0], sizeof(import_mesh.indices[0]) * import_mesh.indices.size());
			}
			aabb.merge(import_mesh.aabb);
			radius_squared = Math::maximum(radius_squared, import_mesh.radius_squared);
		}

		if (create_billboard_lod)
		{
			e_uint16 indices[] = { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15 };
			write(indices, sizeof(indices));
		}

		for (const ImportMesh& import_mesh : meshes)
		{
			if (!import_mesh.import) continue;
			write(import_mesh.vertex_data.getPos());
			write(import_mesh.vertex_data.getData(), import_mesh.vertex_data.getPos());
		}
		writeBillboardVertices(aabb);

		write(sqrtf(radius_squared) * bounding_shape_scale);
		aabb._min *= bounding_shape_scale;
		aabb._max *= bounding_shape_scale;
		write(aabb);
	}

	void ResourceSerializer::writeBillboardMesh(e_int32 attribute_array_offset, e_int32 indices_offset, const char* mesh_output_filename)
	{
		if (!create_billboard_lod) return;

		StaticString<MAX_PATH_LENGTH + 10> material_name(mesh_output_filename, "_billboard");
		e_int32 length = StringUnitl::stringLength(material_name);
		write((const char*)&length, sizeof(length));
		write(material_name, length);

		const char* mesh_name = "billboard";
		length = StringUnitl::stringLength(mesh_name);

		write((const char*)&length, sizeof(length));
		write(mesh_name, length);
	}

	void ResourceSerializer::writeMeshes(const char* mesh_output_filename)
	{
		e_int32 mesh_count = 0;
		for (ImportMesh& mesh : meshes)
			if (mesh.import) ++mesh_count;
		if (create_billboard_lod) ++mesh_count;
		write(mesh_count);

		e_int32 attr_offset = 0;
		e_int32 indices_offset = 0;
		for (ImportMesh& import_mesh : meshes)
		{
			if (!import_mesh.import) continue;

			const ofbx::Mesh& mesh = *import_mesh.fbx;

			e_int32 attribute_count = getAttributeCount(mesh);
			write(attribute_count);

			e_int32 pos_attr = 0;
			write(pos_attr);
			const ofbx::Geometry* geom = mesh.getGeometry();
			if (geom->getNormals())
			{
				e_int32 nrm_attr = 1;
				write(nrm_attr);
			}
			if (geom->getUVs())
			{
				e_int32 uv0_attr = 8;
				write(uv0_attr);
			}
			if (geom->getColors() && import_vertex_colors)
			{
				e_int32 color_attr = 4;
				write(color_attr);
			}
			if (geom->getTangents())
			{
				e_int32 color_attr = 2;
				write(color_attr);
			}
			if (isSkinned(mesh))
			{
				e_int32 indices_attr = 6;
				write(indices_attr);
				e_int32 weight_attr = 7;
				write(weight_attr);
			}

			const ofbx::Material* material = import_mesh.fbx_mat;
			char mat[128];
			getMaterialName(material, mat);
			e_int32 mat_len = (e_int32)strlen(mat);
			write(mat_len);
			write(mat, strlen(mat));

			const char* name = getImportMeshName(import_mesh);
			e_int32 name_len = (e_int32)strlen(name);
			write(name_len);
			write(name, strlen(name));
		}

		writeBillboardMesh(attr_offset, indices_offset, mesh_output_filename);
	}

	void ResourceSerializer::writeSkeleton()
	{
		if (ignore_skeleton)
		{
			write((int)0);
			return;
		}

		write(bones.size());

		for (const ofbx::Object* node : bones)
		{
			const char* name = node->name;
			int len = (int)strlen(name);
			write(len);
			writeString(name);

			ofbx::Object* parent = node->getParent();
			if (!parent)
			{
				write((int)0);
			}
			else
			{
				const char* parent_name = parent->name;
				len = (int)strlen(parent_name);
				write(len);
				writeString(parent_name);
			}

			const ofbx::Mesh* mesh = getAnyMeshFromBone(node);
			Matrix tr = toLumix(getBindPoseMatrix(mesh, node));
			tr.normalizeScale();

			Quaternion q = fixOrientation(tr.getRotation());
			float3 t = fixOrientation(tr.getTranslation());
			write(t * mesh_scale);
			write(q);
		}
	}

	void ResourceSerializer::writeLODs()
	{
		e_int32 lod_count = 1;
		e_int32 last_mesh_idx = -1;
		e_int32 lods[8] = {};
		for (auto& mesh : meshes)
		{
			if (!mesh.import) continue;

			++last_mesh_idx;
			if (mesh.lod >= TlengthOf(lods_distances)) continue;
			lod_count = mesh.lod + 1;
			lods[mesh.lod] = last_mesh_idx;
		}

		for (int i = 1; i < TlengthOf(lods); ++i)
		{
			if (lods[i] < lods[i - 1]) lods[i] = lods[i - 1];
		}

		if (create_billboard_lod)
		{
			lods[lod_count] = last_mesh_idx + 1;
			++lod_count;
		}

		write((const char*)&lod_count, sizeof(lod_count));

		for (int i = 0; i < lod_count; ++i)
		{
			e_int32 to_mesh = lods[i];
			write((const char*)&to_mesh, sizeof(to_mesh));
			float factor = lods_distances[i] < 0 ? FLT_MAX : lods_distances[i] * lods_distances[i];
			write((const char*)&factor, sizeof(factor));
		}
	}

	int ResourceSerializer::getAttributeCount(const ofbx::Mesh& mesh) const
	{
		int count = 1; // position
		if (mesh.getGeometry()->getNormals()) ++count;
		if (mesh.getGeometry()->getUVs()) ++count;
		if (mesh.getGeometry()->getColors() && import_vertex_colors) ++count;
		if (mesh.getGeometry()->getTangents()) ++count;
		if (isSkinned(mesh)) count += 2;
		return count;
	}

	bool ResourceSerializer::areIndices16Bit(const ImportMesh& mesh) const
	{
		int vertex_size = getVertexSize(*mesh.fbx);
		return !(mesh.import && mesh.vertex_data.getPos() / vertex_size > (1 << 16));
	}

	void ResourceSerializer::writeModelHeader()
	{
		Entity::FileHeader header;
		header.magic = 0x5f4c4d4f; // == '_LMO';
		header.version = (e_uint32)Entity::FileVersion::LATEST;
		write(header);
		e_uint32 flags = 0;
		write(flags);
	}

	void ResourceSerializer::writePhysicsHeader(FS::OsFile& file) const
	{
		//PhysicsGeometry::Header header;
		//header.m_magic = PhysicsGeometry::HEADER_MAGIC;
		//header.m_version = (e_uint32)PhysicsGeometry::Versions::LAST;
		//header.m_convex = (e_uint32)make_convex;
		//file.write((const char*)&header, sizeof(header));
	}

	void ResourceSerializer::writePhysicsTriMesh(FS::OsFile& file)
	{
		e_int32 count = 0;
		for (auto& mesh : meshes)
		{
			if (mesh.import_physics) count += mesh.indices.size();
		}
		file.write((const char*)&count, sizeof(count));
		int offset = 0;
		for (auto& mesh : meshes)
		{
			if (!mesh.import_physics) continue;
			for (unsigned int j = 0, c = mesh.indices.size(); j < c; ++j)
			{
				e_uint32 index = mesh.indices[j] + offset;
				file.write((const char*)&index, sizeof(index));
			}
			int vertex_size = getVertexSize(*mesh.fbx);
			int vertex_count = (e_int32)(mesh.vertex_data.getPos() / vertex_size);
			offset += vertex_count;
		}
	}

	bool ResourceSerializer::writePhysics()
	{
		bool any = false;
		for (const ImportMesh& m : meshes)
		{
			if (m.import_physics)
			{
				any = true;
				break;
			}
		}

		if (!any) return true;

		dialog.setImportMessage("Importing physics...", -1);
		char filename[MAX_PATH_LENGTH];
		StringUnitl::getBasename(filename, sizeof(filename), dialog.m_source);
		StringUnitl::catString(filename, ".phy");
		PathBuilder phy_path(dialog.m_output_dir);
		PlatformInterface::makePath(phy_path);
		phy_path << "/" << filename;
		FS::OsFile file;
		if (!file.open(phy_path, FS::Mode::CREATE_AND_WRITE))
		{
			log_error("Could not create file %s.", phy_path);
			return false;
		}

		writePhysicsHeader(file);
		e_int32 count = 0;
		for (auto& mesh : meshes)
		{
			if (mesh.import_physics) count += (e_int32)(mesh.vertex_data.getPos() / getVertexSize(*mesh.fbx));
		}
		file.write((const char*)&count, sizeof(count));
		for (auto& mesh : meshes)
		{
			if (mesh.import_physics)
			{
				int vertex_size = getVertexSize(*mesh.fbx);
				int vertex_count = (e_int32)(mesh.vertex_data.getPos() / vertex_size);

				const e_uint8* verts = (const e_uint8*)mesh.vertex_data.getData();

				for (int i = 0; i < vertex_count; ++i)
				{
					float3 v = *(float3*)(verts + i * vertex_size);
					file.write(&v, sizeof(v));
				}
			}
		}

		if (!make_convex) writePhysicsTriMesh(file);
		file.close();

		return true;
	}

	bool ResourceSerializer::save(const char* output_dir, const char* output_mesh_filename, const char* texture_output_dir)
	{
		writeModel(output_dir, output_mesh_filename);
		writeAnimations(output_dir);
		writeMaterials(output_dir, texture_output_dir, output_mesh_filename);
		writePhysics();

		return true;
	}

	void ResourceSerializer::writeModel(const char* output_dir, const char* output_mesh_filename)
	{
		postprocessMeshes();

		auto cmpMeshes = [](const void* a, const void* b) -> int {
			auto a_mesh = static_cast<const ImportMesh*>(a);
			auto b_mesh = static_cast<const ImportMesh*>(b);
			return a_mesh->lod - b_mesh->lod;
		};

		bool import_any_mesh = false;
		for (const ImportMesh& m : meshes)
		{
			if (m.import) import_any_mesh = true;
		}
		if (!import_any_mesh) return;

		qsort(&meshes[0], meshes.size(), sizeof(meshes[0]), cmpMeshes);
		StaticString<MAX_PATH_LENGTH> out_path(output_dir, output_mesh_filename, ".msh");
		PlatformInterface::makePath(output_dir);
		if (!out_file.open(out_path, FS::Mode::CREATE_AND_WRITE))
		{
			log_error("Failed to create %s.", out_path);
			return;
		}

		dialog.setImportMessage("Writing model...", 0.5f);
		writeModelHeader();
		writeMeshes(output_mesh_filename);
		writeGeometry();
		writeSkeleton();
		writeLODs();
		out_file.close();
	}

	void ResourceSerializer::clearSources()
	{
		for (ofbx::IScene* scene : scenes) scene->destroy();
		scenes.clear();
		meshes.clear();
		materials.clear();
		animations.clear();
		bones.clear();
	}

	void ResourceSerializer::toggleOpen() { open = !open; }
	bool ResourceSerializer::isOpen() const { return open; }

}