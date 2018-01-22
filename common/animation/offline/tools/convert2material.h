
#include "common/animation/offline/mesh.h"
#include "common/animation/offline/fbx/fbx.h"
#include "common/animation/offline/fbx/fbx_base.h"
#include "common/animation/skeleton.h"
#include "common/animation/maths/math_ex.h"
#include "common/animation/maths/simd_math.h"

#include <algorithm>
#include <limits>

#include "common/animation/offline/fbx/fbx_base.h"
#include "common/animation/offline/fbx/fbx.h"
#include "common/animation/offline/fbx/fbx_animation.h"
#include "common/animation/offline/fbx/fbx_skeleton.h"
#include "fbxsdk/utils/fbxgeometryconverter.h"

#include <vector>

#include "common/resource/material_manager.h"
#include "runtime/EngineFramework/engine_root.h"

#include "common/egal_const.h"
#include "fbxsdk/utils/fbxgeometryconverter.h"

#include "common/animation/offline/tools/import_option.h"

namespace egal
{

	void LoadMaterialAttribute(FbxSurfaceMaterial* pSurfaceMaterial, import_material& pMaterial)
	{
		// Get the name of material 
		const char* materialName = pSurfaceMaterial->GetName();
		StringUnitl::copyString(pMaterial.name, TlengthOf(pMaterial.name), materialName);

		FbxPropertyT<FbxDouble3> fbxDouble3;
		FbxPropertyT<FbxDouble> fbxDouble1;
		FbxColor theColor;

		//Get the implementation to see if it's a hardware shader.
		const FbxImplementation* lImplementation = GetImplementation(pSurfaceMaterial, FBXSDK_IMPLEMENTATION_HLSL);
		FbxString lImplemenationType = "HLSL";
		if (!lImplementation)
		{
			lImplementation = GetImplementation(pSurfaceMaterial, FBXSDK_IMPLEMENTATION_CGFX);
			lImplemenationType = "CGFX";
		}
		if (lImplementation)
		{
			//Now we have a hardware shader, let's read it
			log_info("            Hardware Shader Type: %s\n", lImplemenationType.Buffer());
			const FbxBindingTable* lRootTable = lImplementation->GetRootTable();
			FbxString lFileName = lRootTable->DescAbsoluteURL.Get();
			FbxString lTechniqueName = lRootTable->DescTAG.Get();

			const FbxBindingTable* lTable = lImplementation->GetRootTable();
			size_t lEntryNum = lTable->GetEntryCount();

			for (int i = 0; i < (int)lEntryNum; ++i)
			{
				const FbxBindingTableEntry& lEntry = lTable->GetEntry(i);
				const char* lEntrySrcType = lEntry.GetEntryType(true);
				FbxProperty lFbxProp;


				FbxString lTest = lEntry.GetSource();
				log_info("            Entry: %s\n", lTest.Buffer());


				if (strcmp(FbxPropertyEntryView::sEntryType, lEntrySrcType) == 0)
				{
					lFbxProp = pSurfaceMaterial->FindPropertyHierarchical(lEntry.GetSource());
					if (!lFbxProp.IsValid())
					{
						lFbxProp = pSurfaceMaterial->RootProperty.FindHierarchical(lEntry.GetSource());
					}


				}
				else if (strcmp(FbxConstantEntryView::sEntryType, lEntrySrcType) == 0)
				{
					lFbxProp = lImplementation->GetConstants().FindHierarchical(lEntry.GetSource());
				}
				if (lFbxProp.IsValid())
				{
					if (lFbxProp.GetSrcObjectCount<FbxTexture>() > 0)
					{
						//do what you want with the textures
						for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxFileTexture>(); ++j)
						{
							FbxFileTexture *lTex = lFbxProp.GetSrcObject<FbxFileTexture>(j);
							log_info("           File Texture: %s\n", lTex->GetFileName());
						}
						for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxLayeredTexture>(); ++j)
						{
							FbxLayeredTexture *lTex = lFbxProp.GetSrcObject<FbxLayeredTexture>(j);
							log_info("        Layered Texture: %s\n", lTex->GetName());
						}
						for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxProceduralTexture>(); ++j)
						{
							FbxProceduralTexture *lTex = lFbxProp.GetSrcObject<FbxProceduralTexture>(j);
							log_info("     Procedural Texture: %s\n", lTex->GetName());
						}
					}
				}
			}
		}
		// Phong material 
		else if (pSurfaceMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			// Ambient Color 
			fbxDouble3 = ((FbxSurfacePhong*)pSurfaceMaterial)->Ambient;
			theColor.Set(fbxDouble3.Get()[0], fbxDouble3.Get()[1], fbxDouble3.Get()[2]);
			pMaterial.Ambient = { (float)theColor.mRed, (float)theColor.mGreen, (float)theColor.mBlue};

			// Diffuse Color 
			fbxDouble3 = ((FbxSurfacePhong*)pSurfaceMaterial)->Diffuse;
			theColor.Set(fbxDouble3.Get()[0], fbxDouble3.Get()[1], fbxDouble3.Get()[2]);
			pMaterial.Diffuse = { (float)theColor.mRed, (float)theColor.mGreen, (float)theColor.mBlue };

			// Specular Color 
			fbxDouble3 = ((FbxSurfacePhong*)pSurfaceMaterial)->Specular;
			theColor.Set(fbxDouble3.Get()[0], fbxDouble3.Get()[1], fbxDouble3.Get()[2]);
			pMaterial.Specular = { (float)theColor.mRed, (float)theColor.mGreen, (float)theColor.mBlue };

			// Emissive Color 
			fbxDouble3 = ((FbxSurfacePhong*)pSurfaceMaterial)->Emissive;
			theColor.Set(fbxDouble3.Get()[0], fbxDouble3.Get()[1], fbxDouble3.Get()[2]);
			pMaterial.Emissive = { (float)theColor.mRed, (float)theColor.mGreen, (float)theColor.mBlue };

			// Opacity 
			fbxDouble1 = ((FbxSurfacePhong*)pSurfaceMaterial)->TransparencyFactor;
			pMaterial.TransparencyFactor = (1.0 - fbxDouble1.Get());

			// Shininess 
			fbxDouble1 = ((FbxSurfacePhong*)pSurfaceMaterial)->Shininess;
			pMaterial.Shininess = fbxDouble1.Get();

			// Reflectivity 
			fbxDouble1 = ((FbxSurfacePhong*)pSurfaceMaterial)->ReflectionFactor;
			pMaterial.ReflectionFactor = fbxDouble1.Get();
			return;
		}
		// Lambert material 
		else if (pSurfaceMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			// Ambient Color 
			fbxDouble3 = ((FbxSurfaceLambert*)pSurfaceMaterial)->Ambient;
			theColor.Set(fbxDouble3.Get()[0], fbxDouble3.Get()[1], fbxDouble3.Get()[2]);
			pMaterial.Ambient = { (float)theColor.mRed, (float)theColor.mGreen, (float)theColor.mBlue };

			// Diffuse Color 
			fbxDouble3 = ((FbxSurfaceLambert*)pSurfaceMaterial)->Diffuse;
			theColor.Set(fbxDouble3.Get()[0], fbxDouble3.Get()[1], fbxDouble3.Get()[2]);
			pMaterial.Diffuse = { (float)theColor.mRed, (float)theColor.mGreen, (float)theColor.mBlue };

			// Emissive Color 
			fbxDouble3 = ((FbxSurfaceLambert*)pSurfaceMaterial)->Emissive;
			theColor.Set(fbxDouble3.Get()[0], fbxDouble3.Get()[1], fbxDouble3.Get()[2]);
			pMaterial.Emissive = { (float)theColor.mRed, (float)theColor.mGreen, (float)theColor.mBlue };

			// Opacity 
			fbxDouble1 = ((FbxSurfaceLambert*)pSurfaceMaterial)->TransparencyFactor;
			pMaterial.TransparencyFactor = 1.0 - fbxDouble1.Get();
			return;
		}
		else
			log_error("Unknown type of Material");
	}

	void LoadMaterialTexture(FbxSurfaceMaterial* pSurfaceMaterial, import_material& pMaterial)
	{
		int textureLayerIndex;
		FbxProperty pProperty;
		int texID;
		for (textureLayerIndex = 0; textureLayerIndex < FbxLayerElement::sTypeTextureCount; textureLayerIndex++)
		{
			const char* texture_define = FbxLayerElement::sTextureChannelNames[textureLayerIndex];

			pProperty = pSurfaceMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[textureLayerIndex]);
			if (pProperty.IsValid())
			{
				int lTextureCount = pProperty.GetSrcObjectCount<FbxTexture>();

				for (int j = 0; j < lTextureCount; ++j)
				{
					FbxLayeredTexture *lLayeredTexture = pProperty.GetSrcObject<FbxLayeredTexture>(j);
					if (lLayeredTexture)
					{
						int lNbTextures = lLayeredTexture->GetSrcObjectCount<FbxTexture>();
						for (int k = 0; k < lNbTextures; ++k)
						{
							FbxTexture* lTexture = lLayeredTexture->GetSrcObject<FbxTexture>(k);
							if (lTexture)
							{
								FbxFileTexture *lFileTexture = FbxCast<FbxFileTexture>(lTexture);
								const char* texturefilename = lFileTexture->GetRelativeFileName();

								import_texture im_texture;
								StringUnitl::getFilename(im_texture.path, 1024, texturefilename);

								if (StringUnitl::equalStrings(texture_define, "DiffuseColor"))
									im_texture.texture_type = import_texture::diffuse_texture;

								pMaterial.m_textures.push_back(im_texture);
							}
						}
					}
					else
					{
						//no layered texture simply get on the property
						FbxTexture* lTexture = pProperty.GetSrcObject<FbxTexture>(j);
						if (lTexture)
						{
							FbxFileTexture *lFileTexture = FbxCast<FbxFileTexture>(lTexture);
							if (lFileTexture)
							{
								const char* texturefilename = lFileTexture->GetRelativeFileName();

								import_texture im_texture;
								StringUnitl::getFilename(im_texture.path, 1024, texturefilename);

								if (StringUnitl::equalStrings(texture_define, "DiffuseColor"))
									im_texture.texture_type = import_texture::diffuse_texture;

								if (StringUnitl::equalStrings(texture_define, "Bump"))
									im_texture.texture_type = import_texture::bump_texture;
								if (StringUnitl::equalStrings(texture_define, "TransparentColor"))
									im_texture.texture_type = import_texture::diffuse_texture;
								if (StringUnitl::equalStrings(texture_define, ""))
									im_texture.texture_type = import_texture::diffuse_texture;
								if (StringUnitl::equalStrings(texture_define, ""))
									im_texture.texture_type = import_texture::diffuse_texture;

								if (StringUnitl::equalStrings(texture_define, "TransparentColor"))
									return;

								pMaterial.m_textures.push_back(im_texture);
							}
						}
					}
				}
			}
			
		}
	}

	void LoadMaterial(FbxMesh* pMesh, import_material& p_Material)
	{
		int materialCount;
		FbxNode* pNode;

		if (pMesh && pMesh->GetNode())
		{
			pNode = pMesh->GetNode();
			materialCount = pNode->GetMaterialCount();
		}

		//no material
		if (materialCount == 0)
		{
			log_info("        no material applied");
			return;
		}

		//check whether the material maps with only one mesh
		bool lIsAllSame = true;
		for (int l = 0; l < pMesh->GetElementMaterialCount(); l++)
		{
			FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
			if (lMaterialElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
			{
				lIsAllSame = false;
				break;
			}
		}

		//For eAllSame mapping type, just out the material and texture mapping info once
		if (lIsAllSame)
		{
			for (int l = 0; l < pMesh->GetElementMaterialCount(); l++)
			{
				FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
				if (lMaterialElement->GetMappingMode() == FbxGeometryElement::eAllSame)
				{
					FbxSurfaceMaterial* lMaterial = pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(0));
					if (!lMaterial)
						continue;

					LoadMaterialAttribute(lMaterial, p_Material);

					LoadMaterialTexture(lMaterial, p_Material);
				}
			}
		}
		//else	//todo
	}

	void convert2material(FbxMesh* pMesh, int triangleCount, int* pTriangleMtlIndex)
	{
		auto* material_manager = EngineRoot::getSingletonPtr()->getResourceManager().get(RESOURCE_MATERIAL_TYPE);
		Material* material = static_cast<Material*>(material_manager->create());
		

		// Get the material index list of current mesh 
		FbxLayerElementArrayTemplate<int>* pMaterialIndices;
		FbxGeometryElement::EMappingMode   materialMappingMode = FbxLayerElement::EMappingMode::eNone;

		if (pMesh->GetElementMaterial())
		{
			pMaterialIndices = &pMesh->GetElementMaterial()->GetIndexArray();
			materialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
			if (pMaterialIndices)
			{
				switch (materialMappingMode)
				{
					case FbxLayerElement::EMappingMode::eByPolygon:
					{
						if (pMaterialIndices->GetCount() == triangleCount)
						{
							for (int triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
							{
								int materialIndex = pMaterialIndices->GetAt(triangleIndex);

								pTriangleMtlIndex[triangleIndex] = materialIndex;
							}
						}
					}
					break;
					case FbxLayerElement::EMappingMode::eAllSame:
					{
						int lMaterialIndex = pMaterialIndices->GetAt(0);

						for (int triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
						{
							int materialIndex = pMaterialIndices->GetAt(triangleIndex);

							pTriangleMtlIndex[triangleIndex] = materialIndex;
						}
					}
				}
			}
		}
	}


}
