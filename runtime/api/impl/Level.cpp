#include "Level.h"
#include "common/egal-d.h"

namespace SeedEngine
{
	Level::Level()
	{
		mObjectList.clear();

		//mPageManager = ENGINE_NEW PagingManager();
		//mTerrainGroup = ENGINE_NEW TerrainGroup();
		//mWalkPlaneMgr = ENGINE_NEW CollisionManager();
		//mEnvMgr = ENGINE_NEW AreaFogManager();

		mSkeletonEntityManager = egal::_new(SkeletonEntityManager)();

		mSceneFileName = "";
		mSceneName = "";
		mbPageObjectsLoadEnd = false;
		mLoadScene = true;
		mTotalNum = 0;
		mCurrentNum = 0;
		mUseLoadFrame = true;
		mLoadLightMap = false;
		fReloadSceneTime = -1;
		m_loaded = false;
	}
	Level::~Level()
	{
		if (g_EngineOption.bEnableDownload)
			PackageManager::getSingletonPtr()->removeSceneGroupPak();

		unload();

		mPageManager->destroyRenderInstance();
		ENGINE_DELETE(mPageManager);
		ENGINE_DELETE(mTerrainGroup);
		ENGINE_DELETE(mWalkPlaneMgr);
		ENGINE_DELETE(mEnvMgr);
		ENGINE_DELETE(mSkeletonEntityManager);
		mObjectList.clear();
		mLoadLightMap = false;
	}

	BOOL Level::play_scene_anim(LPCTSTR szAniName, BOOL bLoop, Real fTime)
	{
		return TRUE;
	}

	BOOL Level::play_screen_effect(LPCTSTR effectName)
	{
		return TRUE;
	}

	BOOL Level::play_3dsound(LPCTSTR effectName, BOOL bLoop /*= FALSE*/)
	{
		return TRUE;
	}

	BOOL Level::change_bgm(LPCTSTR effectName, BOOL bLoop /*= FALSE*/)
	{
		return TRUE;
	}

	void Level::RefreshPageObject()
	{
		return;
	}

	void Level::CreateAllNoPageObject()
	{
		ObjectList::iterator itObject = mObjectList.begin();
		while (itObject != mObjectList.end())
		{
			IResourceObject* objPtr = *itObject;
			objPtr->loadInstance();
			itObject++;
		}

		return;
	}
	void Level::DestoryAllNoPageobject()
	{
		ObjectList::iterator itObject = mObjectList.begin();
		while (itObject != mObjectList.end())
		{
			IResourceObject* objPtr = *itObject;
			g_EngineManager->getResourceManager()->destory_resource_object(objPtr);

			itObject++;
		}
		mObjectList.clear();

		itObject = mALLObjectList.begin();
		while (itObject != mALLObjectList.end())
		{
			IResourceObject* objPtr = *itObject;
			PackageManager::getSingletonPtr()->removeObject(objPtr);
			g_EngineManager->getResourceManager()->destory_resource_object(objPtr);
			itObject++;
		}
		mALLObjectList.clear();
		return;
	}

	IResourceObject* Level::ParseObjectNode(rapidxml::xml_node<>* pObjectElement)
	{
		rapidxml::xml_attribute<> *pTempAttribute = NULL;
		IResourceObject* objectPtr = NULL;
		pTempAttribute = pObjectElement->first_attribute("type");
		const String& type = pTempAttribute->value();

		objectPtr = g_EngineManager->getResourceManager()->create_resource_object(type);
		if (!objectPtr)
		{
			assert(!objectPtr);
			return NULL;
		}

		pTempAttribute = pObjectElement->first_attribute("name");
		if (pTempAttribute)
		{
			const String& name = pTempAttribute->value();
			objectPtr->setPropertyAsString("name", name);
		}

		rapidxml::xml_node<>* pPropertyElement = pObjectElement->first_node("Property");
		while (pPropertyElement)
		{
			pTempAttribute = pPropertyElement->first_attribute("name");
			if (pTempAttribute)
			{
				const String& name = pTempAttribute->value();

				pTempAttribute = pPropertyElement->first_attribute("value");
				if (pTempAttribute)
				{
					const String& value = pTempAttribute->value();
					objectPtr->setPropertyAsString(name, value);
				}
			}

			pPropertyElement = pPropertyElement->next_sibling("Property");
		}

		if (g_EngineOption.bEnableDownload)
		{
			PackageManager::getSingletonPtr()->addSceneGroupPak(objectPtr->getName(), "", objectPtr->getResourceType());

			rapidxml::xml_node<>* pResourceInfoElement = pObjectElement->first_node("Resource");
			while (pResourceInfoElement)
			{
				pTempAttribute = pResourceInfoElement->first_attribute("type");
				if (pTempAttribute)
				{
					const String& type = pTempAttribute->value();
					pTempAttribute = pResourceInfoElement->first_attribute("name");
					if (pTempAttribute)
					{
						const String& value = pTempAttribute->value();
						if (!value.empty())
						{
							ResourceType eType = (ResourceType)(Ogre::StringConverter::parseUnsignedInt(type));

							PackageManager::getSingletonPtr()->addSceneGroupPak(objectPtr->getName(), value, eType);
						}
					}
				}

				pResourceInfoElement = pResourceInfoElement->next_sibling("Resource");
			}
		}

		return objectPtr;
	}

	void Level::load_level(LPCTSTR filename)
	{
		if (m_loaded)
		{
			unload();
		}

		mLoadTime = 0.0;

		mSceneFileName = filename;
		g_EngineOption.bLightTextureDownloaded = FALSE;

		AddSceneResourceLocation();
		
		parseLevel();

		Ogre::String strWalkPlaneMgr = mSceneName.substr(0, mSceneName.length() - 5) + "collision";
		mWalkPlaneMgr->load_collision_file(strWalkPlaneMgr.c_str());

		if (g_EngineOption.bStramEnable)
			PackageManager::getSingletonPtr()->setFilePath(filename);	  //切换场景的时候清除所有下载内容
		else
		{
			Ogre::String preName = mSceneName.substr(0, mSceneName.length() - 6) + ".xml";
			PackageManager::getSingletonPtr()->parsePreloadFile(preName);
		}

		//camera view distacne
		//int dis = EngineManager::getSingleton().getSceneManager()->getFogEnd() + EngineManager::getSingleton().getSceneManager()->getFogStart();

		m_loaded = true;
	}

	void Level::ApplyMainPos(Ogre::Vector3& vecPos)
	{
		mEnvMgr->ApplyMainPos(vecPos);
	}

	void Level::parseLevel()
	{
		mUseLoadFrame = true;
		mPagesObjectList.clear();
		mALLObjectList.clear();
		mbPageObjectsLoadEnd = false;
		mTotalNum = 0;
		mCurrentNum = 0;

		rapidxml::xml_document<> xmldoc;
		rapidxml::xml_node<> *pRootElement = NULL;
		Ogre::DataStreamPtr stream;
		stream.reset();
		rapidxml::xml_attribute<> *pTempAttribute = NULL;
		{
			tTime tt;
			getLocalTime(&tt);
			Ogre::uint16 hour = tt.wHour;
			Ogre::StringVector nameAndExt = Ogre::StringUtil::split(mSceneName, ".");
			Ogre::String name = nameAndExt.at(0);
			for (Ogre::uint16 i = hour - 1; i <= hour + 1; ++i)
			{
				Ogre::String targetSceneName = name + "#" + Ogre::StringConverter::toString(i) + ".level";

				if (Ogre::ResourceGroupManager::getSingleton().resourceExists("General", targetSceneName))
				{
					stream = Ogre::ResourceGroupManager::getSingleton().openResource(targetSceneName, "General");
					break;
				}
			}
		}
		if (!stream)
			stream = Ogre::ResourceGroupManager::getSingleton().openResource(mSceneName, "General");

		char* xml_copy = OGRE_ALLOC_T(char, stream->size() + 1, Ogre::MEMCATEGORY_GENERAL);
		stream->seek(0);
		stream->read(xml_copy, stream->size());
		xml_copy[stream->size()] = '\0';

		xmldoc.parse<0>(&xml_copy[0]);
		int nLightNum = 0;
		pRootElement = xmldoc.first_node("scene");
		if (pRootElement)
		{
			mPageManager->createRenderInstance();
			rapidxml::xml_node<>* pPageMgrElement = pRootElement->first_node("pagemgr");
			if (pPageMgrElement)
			{
				mPageManager->parsePageMgrConfig(pPageMgrElement);
			}

			//loadLightmap
			rapidxml::xml_node<>* pEnvironmentElement = pRootElement->first_node("environment");
			if (pEnvironmentElement)
			{
				rapidxml::xml_node<>* pLightmapElement = pEnvironmentElement->first_node("LightmapResourceInfo");
				if (pLightmapElement)
				{
					rapidxml::xml_attribute<> *pTempAttribute = NULL;

					//添加光照图到资源列表
					//1、创建资源组
					PackageManager::getSingletonPtr()->addSceneGroupPak(mSceneName + "_LightMap", "", RT_SCENE_LIGHTMAP);

					rapidxml::xml_node<>* pResourceElement = pLightmapElement->first_node("Resource");
					while (pResourceElement)
					{
						pTempAttribute = pResourceElement->first_attribute("name");
						if (pTempAttribute)
						{
							const String& strValue = pTempAttribute->value();
							PackageManager::getSingletonPtr()->addSceneGroupPak(mSceneName + "_LightMap", strValue, RT_TEXTURE);
						}
						pResourceElement = pResourceElement->next_sibling("Resource");
					}
				}
			}

			//创建角色阴影  和烘焙图的主光源一直
			rapidxml::xml_node<>* pShadowLightElement = pRootElement->first_node("shadowLight");
			if (pShadowLightElement)
			{
				pTempAttribute = pShadowLightElement->first_attribute("direction");
				if (pTempAttribute)
				{
					const String& strDirValue = pTempAttribute->value();

					IResourceObject* objectPtr = g_EngineManager->getResourceManager()->create_resource_object("Light");

					assert(objectPtr);

					if (objectPtr)
					{
						objectPtr->setPropertyAsString("name", "shadowLight");
						objectPtr->setPropertyAsString("type", "1");
						objectPtr->setPropertyAsString("cast shadows", "true");
						objectPtr->setPropertyAsString("direction", strDirValue);
						objectPtr->setPropertyAsString("power", "0");
						objectPtr->setPropertyAsString("diffuse", "0 0 0");

						mObjectList.push_back(objectPtr);
					}
				}
			}

			rapidxml::xml_node<>* pTerrainGroupElement = pRootElement->first_node("terrain");
			if (pTerrainGroupElement)
			{
				mTerrainGroup->setTerrainLightNum(0);
				mTerrainGroup->parseTerrain(pTerrainGroupElement, mPageManager);
			}

			rapidxml::xml_node<>* pNodeElementPart = pRootElement->first_node("nodes");
			if (pNodeElementPart)
			{
				rapidxml::xml_node<>* pObjectElement = pNodeElementPart->first_node("node");
				while (pObjectElement)
				{
					IResourceObject* objectPtr = ParseObjectNode(pObjectElement);

////******************************Debug
//#if 1
//					if (!mTestScenenode)
//					{
//						mTestScenenode = g_EngineManager->getDynamicSceneNode()->createChildSceneNode();
//					}
//					
//					Ogre::String meshName = objectPtr->getPropertyAsString("mesh name");
//					if (!meshName.empty())
//					{
//						Ogre::Entity* mEntity = g_EngineManager->getSceneManager()->createEntity(meshName);
//						mTestScenenode->attachObject(mEntity);
//					}
//					pObjectElement = pObjectElement->next_sibling("node");
//					continue;
//#endif
////******************************************

					if (!objectPtr)
					{
						assert(objectPtr);
						pObjectElement = pObjectElement->next_sibling("node");
						continue;
					}

					if (objectPtr->getType() == "Enviroment")
					{
						mEnvMgr->AddEnvObj(objectPtr);
						mObjectList.push_back(objectPtr);
					}
					else
					{
						//这里是特殊处理，这是必须首先加载出来的物件
						if (objectPtr->getLoadOrder() == 999 || objectPtr->getResourceType() == RT_SCENE_PLANE_WATER)
						{
							mPageManager->addObject(objectPtr);
							mALLObjectList.push_back(objectPtr);
						}
						else if (objectPtr->determinCreate())
						{
							mPagesObjectList.push_back(objectPtr);
							mALLObjectList.push_back(objectPtr);
						}
						else
						{
							g_EngineManager->getResourceManager()->destory_resource_object(objectPtr);
							objectPtr = NULL;
						}
					}
					pObjectElement = pObjectElement->next_sibling("node");
				}
			}
		}

		if (mUseLoadFrame)
		{
			mTotalNum = mPagesObjectList.size();
		}
		CreateAllNoPageObject();

		stream->close();
		stream.reset();

		OGRE_FREE(xml_copy, Ogre::MEMCATEGORY_GENERAL);
		xml_copy = 0;

		//3、添加到下载列表里面去
		PackageManager::getSingletonPtr()->execuDownProcess((void*)this, RT_SCENE_LIGHTMAP, mSceneName + "_LightMap");
		//4. end

		fReloadSceneTime = 0;
		
	}

	void Level::unload()
	{
//#if 1
//		if (mTestScenenode)
//		{
//			Ogre::SceneNode::ObjectIterator iter = mTestScenenode->getAttachedObjectIterator();
//			while(iter.hasMoreElements())
//			{
//				 Ogre::MovableObject *object = iter.getNext();
//				object->_getCreator()->destroyInstance(object);
//			}
//
//			mTestScenenode->removeAndDestroyAllChildren();
//			mTestScenenode = NULL;
//		}
//#endif
		if (!m_loaded)
			return;
	
		m_loaded = false;
		
		mSkeletonEntityManager->clear();
		mEnvMgr->Reset();

		if (g_SeedApplication)
			static_cast<SeedEngine::Application*>(g_SeedApplication)->clear_hide_object();
		
		PackageManager::getSingletonPtr()->removeObject(NULL);
		PackageManager::getSingletonPtr()->removeObject((void*)this);
		PackageManager::getSingletonPtr()->removeSceneGroupPak();

		TargetManager::getSingletonPtr()->removeEffectOnGotoScene();
		TargetManager::getSingletonPtr()->clearAllFreeEffects();

		DynamicObjectManager::getSingletonPtr()->_cleanupSkeletalEntityCache();
		mPageManager->destroyRenderInstance();

		mTerrainGroup->unLoad();
		DestoryAllNoPageobject();
		mWalkPlaneMgr->clear_data();

		RemoveSceneResourceLocation();

		mPagesObjectList.clear();

		SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);

		g_EngineManager->clearFreeResource();
		mTotalNum = 0;
		mCurrentNum = 0;
		mSceneFileName = "";

		g_EngineManager->getResourceManager()->check_map();
	}

	VOID Level::refreshPageObject()
	{

	}

	Real Level::getHeight(Ogre::Real x, Ogre::Real z)
	{
		float fHeight, fWalkHeight;
		fHeight = mTerrainGroup->getHeightAt(x, z);
		if (mWalkPlaneMgr->get_hight_at_pos(x, z, fWalkHeight))
		{
			if (fHeight < fWalkHeight)
				fHeight = fWalkHeight;
		}
		return fHeight;
	}

	void Level::AddSceneResourceLocation()
	{
		String strPath, strBaseName, strExtName, strResourceName, strResourcePath;
		Ogre::StringUtil::splitFullFilename(mSceneFileName, strBaseName, strExtName, strPath);
		mSceneName = strBaseName + "." + strExtName;
		size_t i = strBaseName.find_first_of('_');
		String strResGroupPath;
		if (i == String::npos)
		{
			strResGroupPath = strBaseName;
		}
		else
		{
			strResGroupPath = strBaseName.substr(0, i);
		}

		Ogre::ResourceGroupManager::getSingletonPtr()->addResourceLocation(strPath, g_EngineOption.sFileSystemName, "TerrainResources");
		Ogre::ResourceGroupManager::getSingletonPtr()->addResourceLocation(strPath + strBaseName + "/", g_EngineOption.sFileSystemName, "TerrainResources");
		Ogre::ResourceGroupManager::getSingletonPtr()->addResourceLocation(strPath + "terrain/", g_EngineOption.sFileSystemName, "TerrainResources");
	}
	void Level::RemoveSceneResourceLocation()
	{
		String strPath, strBaseName, strExtName;
		Ogre::StringUtil::splitFullFilename(mSceneFileName, strBaseName, strExtName, strPath);

		if (strPath == "")
			return;

		if (Ogre::ResourceGroupManager::getSingletonPtr()->resourceGroupExists("TerrainResources"))
		{
			Ogre::ResourceGroupManager::getSingletonPtr()->removeResourceLocation(strPath, "TerrainResources");
			Ogre::ResourceGroupManager::getSingletonPtr()->removeResourceLocation(strPath + strBaseName + "/", "TerrainResources");
			Ogre::ResourceGroupManager::getSingletonPtr()->removeResourceLocation(strPath + "terrain", "TerrainResources");
			Ogre::ResourceGroupManager::getSingletonPtr()->destroyResourceGroup("TerrainResources");
		}
		Ogre::ArchiveManager::getSingletonPtr()->unload(strPath);
		Ogre::ArchiveManager::getSingletonPtr()->unload(strPath + strBaseName + "/");
		Ogre::ArchiveManager::getSingletonPtr()->unload(strPath + "terrain");
	}

	void Level::LoadLightMapOnDownloaded(bool flag)
	{
		g_EngineOption.bLightTextureDownloaded = flag;
		if (mTerrainGroup && mTerrainGroup->getUserLightMap())
		{
			mTerrainGroup->setUserLightMap(flag && !g_EngineOption.bNoLightMap);
			if (mTerrainGroup->getGrassLoaderHandle())
			{
				std::list<Forests::GrassLayer*> layerList = mTerrainGroup->getGrassLoaderHandle()->getLayerList();
				std::list<Forests::GrassLayer*>::iterator it = layerList.begin();
				while (it != layerList.end())
				{
					Forests::GrassLayer* grassLayer = *it;
					grassLayer->setLightMapEnable(flag && !g_EngineOption.bNoLightMap);
					it++;
				}
			}
		}

		mLoadLightMap = true;
	}

	bool Level::getIntersects(Real winx, Real winy, Ogre::Vector3& position, Ogre::Vector3* normal, bool allowOutside) const
	{
		const float c_Maxdistance = 100.0f;
		bool bIntersects = false;
		Ogre::Vector3 vecCollisionPos;

		Ogre::Ray ray = g_EngineManager->getCamera()->GetOgreCamera()->getCameraToViewportRay(
			(winx - g_EngineManager->getViewport()->getActualLeft()) / g_EngineManager->getViewport()->getActualWidth(),
			(winy - g_EngineManager->getViewport()->getActualTop()) / g_EngineManager->getViewport()->getActualHeight());

		bIntersects = mTerrainGroup->getIntersects(ray, position, normal, allowOutside);
		if (mWalkPlaneMgr)
		{
			if (bIntersects)
			{
				bool bCollsion = false;
				bCollsion = mWalkPlaneMgr->build_collision(ray.getOrigin().x, ray.getOrigin().y, ray.getOrigin().z, ray.getDirection().x, ray.getDirection().y, ray.getDirection().z,
					position.x, position.y, position.z, vecCollisionPos.x, vecCollisionPos.y, vecCollisionPos.z);
				if (bCollsion && position.y < vecCollisionPos.y)
					position = vecCollisionPos;
			}
			else
			{
				Ogre::Vector3 vecMaxDisPoint = ray.getOrigin() + c_Maxdistance * ray.getDirection();
				bool bCollsion = false;
				bCollsion = mWalkPlaneMgr->build_collision(ray.getOrigin().x, ray.getOrigin().y, ray.getOrigin().z, ray.getDirection().x, ray.getDirection().y, ray.getDirection().z,
					vecMaxDisPoint.x, vecMaxDisPoint.y, vecMaxDisPoint.z, vecCollisionPos.x, vecCollisionPos.y, vecCollisionPos.z);
				if (bCollsion)
				{
					position = vecCollisionPos;
					bIntersects = true;
				}
			}
		}
		return bIntersects;
	}

	bool Level::getPercentage(float& percentage)
	{
		if (!mUseLoadFrame)
		{
			percentage = 1;
			return true;
		}

		if (mTotalNum == 0)
		{
			mTotalNum = 1;
		}
		percentage = mCurrentNum * 1.0 / mTotalNum;

		return mbPageObjectsLoadEnd;
	}

	bool Level::update(Real  fElapsedTime)
	{
		OgreProfile("level_update", Ogre::OGREPROF_GENERAL);
		if (mEnvMgr)
			mEnvMgr->ApplyMainPos(g_EngineManager->getCharactorPosition());

		if (mLoadLightMap)
		{
			if (mPageManager)
			{
				bool loadEnd = mPageManager->LoadAllLightMap(fElapsedTime);
				if (loadEnd == true)
				{
					mLoadLightMap = false; 
				}
			}
		}

		//防止地形没有被加载出来，由于具体原因没有找到，故先加个保险丝
		if (fReloadSceneTime >= 0)
		{
			fReloadSceneTime += fElapsedTime;
			if (fReloadSceneTime > 20)  //地形如果20秒还没有被加载出来，就重新加载一遍
			{
				//if (tTerrainGroup && !tTerrainGroup->isAllTerrainLoaded())
				//{
				//	tTerrainGroup->unLoad();
				//	g_Seed->Load(0, mSceneFileName.c_str());
				//}

				fReloadSceneTime = -1;
			}
		}

		if (!mUseLoadFrame)
			return true;

		if (!mbPageObjectsLoadEnd)
		{
			int k = 0;
			ObjectList::iterator itObject = mPagesObjectList.begin();
			while (itObject != mPagesObjectList.end())
			{
				k++;
				IResourceObject* objPtr = *itObject;
				mPageManager->addObject(objPtr);
				mCurrentNum++;

				mPagesObjectList.erase(itObject++);
				if (k > 20)
					break;
			}

			k = 0;

			if (mPagesObjectList.size() == 0)
			{
				mbPageObjectsLoadEnd = true;
				mUseLoadFrame = false;
				mCurrentNum = 0;
				mTotalNum = 0;
				m_loaded = true;
			}
		}
		return mbPageObjectsLoadEnd;
	}
}