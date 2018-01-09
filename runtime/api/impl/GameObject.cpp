#include "Seed.h"
#include "SeedEnginePublic.h"

#include "EngineManager.h"
#include "DynamicModelObject.h"
#include "DownloadPackageManager.h"

#include <OgreVector3.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreLogmanager.h>
#include "OgreProfiler.h"

#include "GameObject_Actor.h"

#include "DynamicModelResourceObject.h"

#define		FOBJ_POSITION			"position"				
#define		FOBJ_ORIENTATION		"orientation"			
#define		FOBJ_ACTOR_FILE			"logic model name"	

//#define TEST
#ifdef TEST
#define TEST_LOG(str) Ogre::LogManager::getSingleton().logMessage("[Actor]:" + str);
#else
#define TEST_LOG(str)
#endif



#include "seed_memory_leak_check.h" //the last include head file,no include after this line
namespace SeedEngine
{
	GameObject_Actor::GameObject_Actor(): GameObject()
	{
		mModel = NULL;
		m_bVisible = TRUE;
		m_wxModel = NULL;
		m_wxEffect = 0;
		m_bUiVisible = FALSE;
		m_ValueStr = "1;1";
		m_bDownloaded = FALSE;
		m_pResourceObject = NULL;

		m_pResourceObject = ENGINE_NEW_T(DynamicModelResourceObject, Ogre::MEMCATEGORY_GENERAL)();
		m_pResourceObject->setData((DWORD)this);
		m_ActorProperty.m_defualtModel = false;
		m_ActorProperty.init();
	}

	GameObject_Actor::~GameObject_Actor()
	{
		//有一点绕，本身是被绑定的模型，删除父节点上关于自己的信息。
		//if (m_ActorProperty.m_game_object_actor)
		//{
		//	AttachMap::iterator it = m_ActorProperty.m_game_object_actor->m_ActorProperty.m_attachMap.find(this);
		//	if (it != m_ActorProperty.m_game_object_actor->m_ActorProperty.m_attachMap.end())
		//		m_ActorProperty.m_game_object_actor->m_ActorProperty.m_attachMap.erase(this);
		//}

		m_ActorProperty.init();
		Destroy();
	}

	VOID GameObject_Actor::Destroy(VOID)
	{
		PackageManager::getSingletonPtr()->removeObject((void*)this);
#if enable_check
		delete m_pResourceObject;
#else
		ENGINE_DELETE_T((DynamicModelResourceObject*)m_pResourceObject, DynamicModelResourceObject, Ogre::MEMCATEGORY_GENERAL);
#endif
		

		mModel = NULL;
		m_wxModel = NULL;
	}

	VOID GameObject_Actor::SetResourceFile(LPCTSTR szActorFile)
	{
		if (m_ActorProperty.m_ActorFileName == szActorFile || !m_pResourceObject)
			return;

		if(!g_EngineOption.bGlobalCreateDynObject)
			return;

		m_ActorProperty.init();

		m_ActorProperty.m_ActorFileName = szActorFile;

		assert(szActorFile && m_pResourceObject);
		bool isReady = PackageManager::getSingletonPtr()->isReady(m_ActorProperty.m_ActorFileName);
		if (isReady || !g_EngineOption.bEnableDownload)
		{
			m_pResourceObject->unloadInstance();
			m_pResourceObject->setProperty("logic model name", Ogre::Any(m_ActorProperty.m_ActorFileName));
			m_pResourceObject->loadInstance();
			mModel = ((DynamicModelResourceObject*)m_pResourceObject)->getLogicModel();
			m_bDownloaded = TRUE;
		}
		else
		{
			m_ActorProperty.m_defualtModel = true;
			m_pResourceObject->unloadInstance();
			m_pResourceObject->setProperty(FOBJ_ACTOR_FILE, Ogre::Any(String(DEFAULT_ACTOR)));
			m_pResourceObject->loadInstance();
			mModel = ((DynamicModelResourceObject*)m_pResourceObject)->getLogicModel();
			if (!mModel)
				return;

			mModel->setVisible(false);
			mModel->setDefaultModel(TRUE);
			PackageManager::getSingletonPtr()->execuDownProcess((void*)this, m_pResourceObject->getResourceType(), m_ActorProperty.m_ActorFileName);
		}
	}

	BOOL GameObject_Actor::HasSetResourceFile()
	{
		if (m_ActorProperty.m_ActorFileName.empty())
			return FALSE;
		return TRUE;
	}

	BOOL GameObject_Actor::GetDownload()
	{
		return m_bDownloaded;
	}

	void GameObject_Actor::Update()
	{
		//0、非默认模型
		m_ActorProperty.m_defualtModel = false;
		//1、解除已经绑定的模型,避免绑定的模型被删除
		AttachMap::iterator detachIt = m_ActorProperty.m_attachMap.begin();
		while (detachIt != m_ActorProperty.m_attachMap.end())
		{
			Detach_Object(detachIt->first);
			detachIt++;
		}

		bool mCreateRoleSpecialVisible = GetWxModel()->getCreateRoleSpecialVisible();

		DynamicModelObject* mAttachParentModel = GetWxModel()->getAttachModel();
		Ogre::String locatName =  GetWxModel()->getAttachLocatorName();

		m_pResourceObject->unloadInstance();
		m_pResourceObject->setProperty(FOBJ_ACTOR_FILE, Ogre::Any(String(m_ActorProperty.m_ActorFileName)));
		m_pResourceObject->loadInstance();
		mModel = ((DynamicModelResourceObject*)m_pResourceObject)->getLogicModel();
		TEST_LOG(String(m_ActorProperty.m_ActorFileName) + " update.");

		//6、做事件回掉
		SetAnimationEndEvent(m_ActorProperty.pAniFunc, m_ActorProperty.dwAniEndParam);
		SetAnimationShakeEvent(m_ActorProperty.pShakeFunc, m_ActorProperty.dwShakeParam);

		//1、刷新最后一个动作
		TEST_LOG(String(m_ActorProperty.m_ActorFileName) + " " + String(m_ActorProperty.m_sAniName) + " " + Ogre::StringConverter::toString(m_ActorProperty.m_nLoop) + " update.");
		PlayAnimTag(m_ActorProperty.m_sAniName.c_str(), m_ActorProperty.m_sSkillName.c_str(), m_ActorProperty.m_nLoop, 0.3);
		//2、刷新属性
		AttributeMap::iterator it = m_ActorProperty.m_attributeMap.begin();
		while (it != m_ActorProperty.m_attributeMap.end())
		{
			SetProperty(it->first.c_str(), it->second.c_str());
			it++;
		}
		WeaponAnimationMap::iterator aniIt = m_ActorProperty.m_weaponMap.begin();
		while (aniIt != m_ActorProperty.m_weaponMap.end())
		{
			WeaponAnimation weaAni = aniIt->second;
			EnterWeaponAnim(weaAni.szAniName.c_str(), weaAni.locLocation, weaAni.bLoop, weaAni.fFuseParam);
			if (weaAni.fRate != 1.0)
				ChangeActionRate(weaAni.fRate, weaAni.locLocation);
			aniIt++;
		}
		//3、添加特效
		EffectMap::iterator effIt = m_ActorProperty.m_effectMap.begin();
		while (effIt != m_ActorProperty.m_effectMap.end())
		{
			EffectInfo info = effIt->second;
			uint32_t effHandle = AddEffect(info.nameStr.c_str(), info.locatorStr.c_str(), EDL_NONE, info.bRoat);
			effIt->second.newId = effHandle;
			effIt++;
		}

		//4、添加绑定模型
		mModel->updateLocatorPos();

		//选人界面
		mModel->setCreateRoleSpecialVisible(mCreateRoleSpecialVisible);

		if (mAttachParentModel)
		{
			mAttachParentModel->attachModel(locatName, mModel);
		}
		AttachMap::iterator attachIt = m_ActorProperty.m_attachMap.begin();
		while (attachIt != m_ActorProperty.m_attachMap.end())
		{
			Attach_Object(attachIt->first, attachIt->second.c_str());
			attachIt++;
		}
		m_ActorProperty.m_game_object_actor = NULL;

		//5、设置transform
		if (!mModel->isAttached())
		{
			SetPosition(m_ActorProperty.m_Position);
			SetOrientation(m_ActorProperty.m_roate);
			ApplyShader(m_ActorProperty.m_szShaderName.c_str());
			SetVisible(m_ActorProperty.m_bVisible, m_ActorProperty.m_Localation);
			SetEffectVisible(m_ActorProperty.m_bEffectVisible, m_ActorProperty.m_Localation);
		}

		ChangeActionRate(m_ActorProperty.m_fActionRate);

		if (m_ActorProperty.m_bUIVisible)
		{
			SetUIVisibleFlag();
			SetUIObjectVisible(m_ActorProperty.m_bUIVisible);
		}
		m_bDownloaded = TRUE;
	}

	void GameObject_Actor::UpdateProperty(Ogre::String name)
	{
		if (name.find(".material") != -1)
			name = name.substr(0, name.length() - 9);

		AttributeMap::iterator it = m_ActorProperty.m_attributeNeedCheckMap.begin();
		while (it != m_ActorProperty.m_attributeNeedCheckMap.end())
		{
			if (m_ActorProperty.m_defualtModel && name == it->second)
			{
				AttributeMap::iterator aait = m_ActorProperty.m_attributeMap.find(it->first);
				if (aait != m_ActorProperty.m_attributeMap.end())
					aait->second = it->second;// name.c_str();
				else
					m_ActorProperty.m_attributeMap.insert(AttributeMap::value_type(it->first, it->second));
			}
			else
			{
				if (name == String(it->second))
					SetProperty(it->first.c_str(), it->second.c_str());
			}

			it++;
		}
		WeaponAnimationMap::iterator aniIt = m_ActorProperty.m_weaponMap.begin();
		while (aniIt != m_ActorProperty.m_weaponMap.end())
		{
			WeaponAnimation weaAni = aniIt->second;
			EnterWeaponAnim(weaAni.szAniName.c_str(), weaAni.locLocation, weaAni.bLoop, weaAni.fFuseParam);
			if (weaAni.fRate != 1.0)
				ChangeActionRate(weaAni.fRate, weaAni.locLocation);
			aniIt++;
		}
	}

	void GameObject_Actor::SetUIObjectVisible(BOOL bFlag)
	{
		m_ActorProperty.m_bUIVisible = bFlag;
		//确认资源已经Load
		if (!mModel) return;

		mModel->setUIVisible(bFlag);
	}
	void GameObject_Actor::SetCreateRoleSpecialVisible(BOOL bFlag)
	{
		//确认资源已经Load
		if (!mModel) return;
		mModel->setCreateRoleSpecialVisible(bFlag);
	}

	VOID GameObject_Actor::SetVisible(BOOL bVisible, enum_dummy_location eInfoType)
	{
		//确认资源已经Load
		if (!mModel) return;

		if (m_ActorProperty.m_defualtModel)
		{
			m_ActorProperty.m_Localation = eInfoType;
			m_ActorProperty.m_bVisible = bVisible;
		}

		GetWxModel(eInfoType);
		if (!m_wxModel)
			return;

		m_wxModel->setVisible(bVisible == TRUE);

		m_bVisible = bVisible;
	}

	VOID GameObject_Actor::SetEffectVisible(BOOL bVisible, enum_dummy_location eInfoType)
	{
		if (!mModel) return;

		if (m_ActorProperty.m_defualtModel)
		{
			m_ActorProperty.m_Localation = eInfoType;
			m_ActorProperty.m_bEffectVisible = bVisible;
		}

		GetWxModel(eInfoType);
		if (!m_wxModel)
			return;

		m_wxModel->setEffectVisible(bVisible == TRUE);
	}

	VOID GameObject_Actor::SetScale(const vector3& vScale, enum_dummy_location eInfoType, float fTime)
	{
		if (!mModel) return;

		GetWxModel(eInfoType);
		if (!m_wxModel)
			return;

		m_wxModel->CanSetChildScale(eInfoType != EDL_NONE && fTime != 0);
		m_wxModel->setScale(Ogre::Vector3(vScale.x, vScale.y, vScale.z), fTime);
	}

	BOOL GameObject_Actor::GetVisible() const
	{
		return m_bVisible;
	}

	VOID GameObject_Actor::SetProperty(LPCTSTR szName, LPCTSTR szValue, enum_dummy_location eInfoType)
	{
		assert(m_pResourceObject);

		//确认资源已经Load
		if (!mModel) return;

		GetWxModel(eInfoType);
		if (!m_wxModel)
			return;

		if (szValue != "")
		{
			ResourceType rt;
			bool  needDownm = mModel->hasAttributeNeedDownload(szName, rt);
			if (needDownm)
			{
				bool isReady = PackageManager::getSingletonPtr()->isReady(szValue, rt);
				if (!isReady)
				{
					Ogre::String valueStr = szValue;
					if (rt == RT_MATERIAL && valueStr.find(".material") == -1)   //作为区分
						valueStr += ".material";

					AttributeMap::iterator it = m_ActorProperty.m_attributeNeedCheckMap.find(szName);
					if (it != m_ActorProperty.m_attributeNeedCheckMap.end())
						it->second = szValue;
					else
						m_ActorProperty.m_attributeNeedCheckMap.insert(AttributeMap::value_type(szName, szValue));

					PackageManager::getSingletonPtr()->execuDownProcess((void*)this, RT_ACTOR_PROPERTY, valueStr);
					return;
				}
			}
		}

		if (m_ActorProperty.m_defualtModel)
		{
			//更新设置过的属性，有些属性需要下载资源的
			AttributeMap::iterator it = m_ActorProperty.m_attributeMap.find(szName);
			if (it != m_ActorProperty.m_attributeMap.end())
				it->second = szValue;
			else
				m_ActorProperty.m_attributeMap.insert(AttributeMap::value_type(szName, szValue));
		}

		try
		{
			mModel->changeAttribute(szName, szValue);
		}
		catch (...)
		{
			int a = 0;
		}
	}

	VOID GameObject_Actor::GetProperty(LPCTSTR szName, char*  szValue, size_t valueLength, enum_dummy_location eInfoType)
	{
		assert(m_pResourceObject);
		m_ValueStr = "";

		size_t  uLen = 0;
		uLen = strlen(m_ValueStr.c_str());
		uLen = uLen < valueLength ? uLen : valueLength;

		//确认资源已经Load
		if (!mModel)
		{
			memcpy(szValue, m_ValueStr.c_str(), uLen + 1);
			return;
		}

		GetWxModel(eInfoType);
		if (m_wxModel)
		{
			m_wxModel->getAttribute(szName, m_ValueStr);
		}

		uLen = strlen(m_ValueStr.c_str());
		uLen = uLen < valueLength ? uLen : valueLength;
		memcpy(szValue, m_ValueStr.c_str(), uLen + 1);
	}

	VOID GameObject_Actor::GetLocator(LPCTSTR szName, vector3& fvPosition)
	{
		assert(m_pResourceObject);
		if (!mModel) return;

		Ogre::Matrix4 mx;
		bool bResult = mModel->getLocatorWorldTransform(szName, mx);
		if (bResult)
		{
			vector3 fvGfx(mx[0][3], mx[1][3], mx[2][3]);

			g_SeedApplication->axis_trans(EXT_ENGINE, fvGfx, EXT_GAME, fvPosition);
		}
	}

	inline float SMOOTH(float f1, float f2, float fper, float max_f)
	{
		if (abs(f1 - f2) > max_f)
		{
			return (int)(f2 + 0.5f);
		}

		return (int)((float)f1 + (f2 - f1) * (float)fper + 0.5f);
	}

	BOOL GameObject_Actor::GetInfoBoardPos(vector3& fvPos, const vector3 *pvObjPos, const float fObligeHeight)
	{
		if (!mModel)
			return FALSE;

		Ogre::Vector3 vPos;
		if (pvObjPos != NULL)
		{
			//坐标转化
			vector3 fvGfxPos;
			g_SeedApplication->axis_trans(EXT_GAME, *pvObjPos, EXT_ENGINE, fvGfxPos);

			vPos.x = fvGfxPos.x;
			vPos.y = fvGfxPos.y;
			vPos.z = fvGfxPos.z;
		}
		else
		{
			vPos = mModel->getSceneNode()->_getDerivedPosition();
		}

		Real s_fAddOffset = 0.1;
		Real s_fAddHeight = mModel->getBoundBoxHeight();

		Ogre::Vector3 vScale;
		vScale = mModel->getScaleValue();
		if (fObligeHeight > 0)
			vPos.y += fObligeHeight;
		else
			vPos.y += mModel->getBoundBoxHeight() * vScale.y;

		//增加0.1m
		vPos.y += 0.1;

		vector3 fvGfx(vPos.x, vPos.y, vPos.z);
		vector3 fvScreen;
		BOOL bVisible = g_SeedApplication->axis_trans(EXT_ENGINE, fvGfx, EXT_SCREEN, fvScreen);

		if (!bVisible)
			return FALSE;
		static vector3 fLastValue;

		fvPos.x = SMOOTH(fLastValue.x, fvScreen.x, 0.8f, 3.f);
		fvPos.y = SMOOTH(fLastValue.y, fvScreen.y, 0.8f, 3.f);
		fvPos.z = fvScreen.z;
		fLastValue.x = fvPos.x;

		fLastValue.y = fvPos.y;

		return TRUE;
	}
	
	VOID GameObject_Actor::PlayAnim(LPCTSTR szAniName, BOOL bLoop, FLOAT fFuseParam, FLOAT fRadian)
	{
		if (!mModel || !szAniName) return;
		if (m_ActorProperty.m_defualtModel)
		{
			m_ActorProperty.m_sAniName = szAniName;
			m_ActorProperty.m_nLoop = bLoop;
		}

		mModel->changeAnimation(szAniName, fFuseParam);
		mModel->setAnimationLoop(bLoop == TRUE);
	}

	VOID GameObject_Actor::PlayAnimDoubleTag(LPCTSTR szAniName, LPCTSTR szTagName_1, LPCTSTR szTagName_2, BOOL bLoop, FLOAT fFuseParam, FLOAT fRadian)
	{
		if (!mModel) return;

		if (m_ActorProperty.m_defualtModel)
		{
			m_ActorProperty.m_sAniName = szAniName;
			m_ActorProperty.m_nLoop = bLoop;
		}

		{
			if (szTagName_1 != NULL && szTagName_1[0] != '\0' && szTagName_2 != NULL && szTagName_2[0] != '\0')
			{
				mModel->delDoubleCurrentTag();
				mModel->createDoubleTag(szTagName_1, szTagName_2, bLoop == TRUE, TRUE, fRadian);
			}
			else
			{
				mModel->delDoubleCurrentTag(true);
			}
		}

		mModel->changeAnimation(szAniName, fFuseParam);
		mModel->setAnimationLoop(bLoop == TRUE);
	}

	VOID  GameObject_Actor::PlayTag(LPCTSTR szfTagName, BOOL bLoop, BOOL bTimeControl)
	{
		if (!mModel || !szfTagName) return;
		mModel->createTag(szfTagName, bLoop == TRUE, bTimeControl == TRUE);
	}

	VOID  GameObject_Actor::PlayAnimTag(LPCTSTR szAniName, LPCTSTR szTagName, BOOL bLoop, FLOAT fFuseParam, FLOAT fRadian)
	{
		if (m_ActorProperty.m_defualtModel)
		{
			TEST_LOG(String(m_ActorProperty.m_ActorFileName) + String(szAniName) + Ogre::StringConverter::toString(bLoop) + " Anim.");
			m_ActorProperty.m_sAniName = szAniName;
			m_ActorProperty.m_sSkillName = szTagName;
			m_ActorProperty.m_nLoop = bLoop;
		}

		if (!mModel) return;

		if (szTagName != NULL && szTagName[0] != '\0')
		{
			if (szTagName != NULL && szTagName[0] != '\0')
			{
				mModel->delDoubleCurrentTag();
				mModel->createTag(szTagName, bLoop == TRUE, TRUE, fRadian);
			}
			else
			{
				mModel->delDoubleCurrentTag();
			}
		}
		else
		{
			mModel->delCurrentTag(true);
		}
		mModel->changeAnimation(szAniName, fFuseParam);
		mModel->setAnimationLoop(bLoop == TRUE);
	};

	VOID GameObject_Actor::EnterWeaponAnim(LPCTSTR szAniName, enum_dummy_location locLocation, BOOL bLoop, FLOAT fFuseParam)
	{
		{
			WeaponAnimationMap::iterator it = m_ActorProperty.m_weaponMap.find(locLocation);
			if (it != m_ActorProperty.m_weaponMap.end())
			{
				it->second.szAniName = szAniName;
				it->second.locLocation = locLocation;
				it->second.bLoop = bLoop;
				it->second.fFuseParam = fFuseParam;
			}
			else
			{
				WeaponAnimation weaponAnima;
				weaponAnima.szAniName = szAniName;
				weaponAnima.locLocation = locLocation;
				weaponAnima.bLoop = bLoop;
				weaponAnima.fFuseParam = fFuseParam;
				m_ActorProperty.m_weaponMap.insert(WeaponAnimationMap::value_type(locLocation, weaponAnima));
			}
		}

		GetWxModel(locLocation);
		if (m_wxModel)
		{
			m_wxModel->changeAnimation(szAniName, fFuseParam);
			m_wxModel->setAnimationLoop(bLoop == TRUE);
		}
	}

	VOID GameObject_Actor::ChangeActionRate(FLOAT fRate, enum_dummy_location locLocation)
	{
		if (m_ActorProperty.m_defualtModel)
		{
			WeaponAnimationMap::iterator it = m_ActorProperty.m_weaponMap.find(locLocation);
			if (it != m_ActorProperty.m_weaponMap.end())
			{
				it->second.fRate = fRate;
			}
		}

		if (!mModel) return;

		GetWxModel(locLocation);
		if (m_wxModel)
			m_wxModel->setGlobalAnimationRate(fRate);
	}

	VOID GameObject_Actor::SetDefaultAnim(LPCTSTR szAnimName)
	{
		if (!mModel) return;

		mModel->setDefaultAnimationName(szAnimName);
	}

	VOID GameObject_Actor::SetLightMask(uint32_t flag)
	{
		if (!mModel) return;
		if (m_ActorProperty.m_defualtModel)
			m_ActorProperty.m_lightMask = flag;
		mModel->setLightMask(flag);
	}

	VOID GameObject_Actor::SetTransparent(FLOAT fTransparency, FLOAT fTime)
	{
		if (!mModel) return;

		mModel->setTransparent(fTime, fTransparency);
	}

	VOID GameObject_Actor::SetEffectExtraTransformInfos(uint32_t uEffect, vector3& fvPosition)
	{
		if (!mModel) return;

		EffectTarget *pEffect = mModel->getEffect(uEffect);
		if (pEffect != NULL)
		{
			vector3 fvGfxPos;
			g_SeedApplication->axis_trans(EXT_GAME, fvPosition, EXT_ENGINE, fvGfxPos);
			TransformInfo tempTransformInfo(Ogre::Vector3(fvGfxPos.x, fvGfxPos.y, fvGfxPos.z));
			pEffect->setTransformInfo(tempTransformInfo);
		}
	}

	void GameObject_Actor::SetAnimationEndEvent(CallBack_On_Animation_End pFunc, DWORD dwParam)
	{
		if (!mModel) return;

		if (m_ActorProperty.m_defualtModel)
		{
			m_ActorProperty.pAniFunc = pFunc;
			m_ActorProperty.dwShakeParam = dwParam;
		}
		mModel->SetAnimationFinishListener(pFunc, dwParam);
	}

	void GameObject_Actor::SetAnimationShakeEvent(CallBack_On_Animation_Shake pFunc, DWORD dwParam)
	{
		if (!mModel) return;
		if (m_ActorProperty.m_defualtModel)
		{
			m_ActorProperty.pShakeFunc = pFunc;
			m_ActorProperty.dwShakeParam = dwParam;
		}

		mModel->SetSkillShakeTimeListener(pFunc, dwParam);
	}

	uint32_t GameObject_Actor::AddEffect(LPCTSTR effectName, LPCTSTR locatorName, enum_dummy_location eInfoType, BOOL bRot)
	{
		if (!mModel)
			return NULL;

		GetTransformInfoType transformtype = GTIT_POSITION;
		if (bRot)
		{
			transformtype = GTIT_ALL;
		}
		GetWxModel(eInfoType);
		uint32_t i = m_wxModel->addEffect(Ogre::String(effectName), Ogre::String(locatorName), transformtype);
		if (m_ActorProperty.m_defualtModel)
		{
			EffectInfo effectInfo;
			effectInfo.nameStr = effectName;
			effectInfo.bRoat = transformtype;
			effectInfo.locatorStr = locatorName;
			m_ActorProperty.m_effectMap.insert(EffectMap::value_type(i, effectInfo));
		}
		return i;
	}

	VOID GameObject_Actor::DelEffect(uint32_t effect, enum_remove_type removeEffectType, enum_dummy_location eInfoType)
	{
		if (!mModel)
			return;

		GetWxModel(eInfoType);
		if (!m_wxModel)
			return;
		EffectMap::iterator it = m_ActorProperty.m_effectMap.find(effect);
		if (it != m_ActorProperty.m_effectMap.end())
		{
			EffectInfo info = it->second;

			m_wxModel->delEffect(info.newId);
			m_ActorProperty.m_effectMap.erase(it);
		}
		m_wxModel->delEffect(effect);
	}

	VOID GameObject_Actor::DelAllEffect(enum_dummy_location eInfoType)
	{
		if (!mModel)
			return;

		if (m_ActorProperty.m_defualtModel)
			m_ActorProperty.m_effectMap.clear();

		GetWxModel(eInfoType);

		if (m_wxModel)
			m_wxModel->delAllEffect();
	}

	void GameObject_Actor::Attach_Object(IGameObject *pObject, LPCTSTR szAttachLocator)
	{
		if (!pObject)
			return;

		if (mModel != NULL && pObject != NULL && pObject->GetType() == EGOT_CHARACTOR && szAttachLocator != NULL)
		{
			GameObject_Actor *pRFObject = (GameObject_Actor*)pObject;
			DynamicModelObject* pActorImpl = pRFObject->GetWxModel();
			if (pActorImpl != NULL && mModel != pActorImpl)
			{
				bool bResult = mModel->attachModel(szAttachLocator, pActorImpl);
				if (bResult)
				{
					if (m_ActorProperty.m_defualtModel)
					{
						//pRFObject->m_ActorProperty.m_game_object_actor = this;
						m_ActorProperty.m_attachMap.insert(AttachMap::value_type(pObject, szAttachLocator));
					}
				}
			}
		}
	}

	void GameObject_Actor::Detach_Object(IGameObject *pObject)
	{
		if (m_ActorProperty.m_defualtModel)
		{
			AttachMap::iterator it = m_ActorProperty.m_attachMap.find(pObject);
			if (it != m_ActorProperty.m_attachMap.end())
				m_ActorProperty.m_attachMap.erase(it);
		}

		if (mModel != NULL && pObject != NULL && pObject->GetType() == EGOT_CHARACTOR)
		{
			GameObject_Actor *pRFObject = (GameObject_Actor*)pObject;
			DynamicModelObject* pActorImpl = pRFObject->GetWxModel();
			if (pActorImpl != NULL)
			{
				mModel->detachModel(pActorImpl);
			}
		}
	}

	VOID GameObject_Actor::SetUIVisibleFlag(VOID)
	{
		if (!mModel) return;

		m_bUiVisible = TRUE;

		if (m_ActorProperty.m_defualtModel)
			m_ActorProperty.m_bUIVisible = true;

		mModel->setVisibleFlag(VF_GUI_ELEMENTS);
	}

	VOID GameObject_Actor::SetPosition(const vector3& vPos)
	{
		assert(m_pResourceObject);
		if (!mModel) return;

		vector3 fvGfxPos;
		g_SeedApplication->axis_trans(EXT_GAME, vPos, EXT_ENGINE, fvGfxPos);

		if (m_ActorProperty.m_defualtModel)
			m_ActorProperty.m_Position = vPos;

		mModel->setPosition(Ogre::Vector3(fvGfxPos.x, fvGfxPos.y, fvGfxPos.z));
	}

	VOID GameObject_Actor::SetOrientation(const vector3& vRotate)
	{
		assert(m_pResourceObject);
		if (!mModel) return;

		if (m_ActorProperty.m_defualtModel)
			m_ActorProperty.m_roate = vRotate;

		Ogre::Quaternion qu(Ogre::Radian(vRotate.y), Ogre::Vector3::UNIT_Y);
		mModel->setOrientation(qu);
	}
	VOID GameObject_Actor::SetEulerOrientation(const vector3& vRotate)
	{
		assert(m_pResourceObject);

		if (!mModel) return;

		Ogre::Matrix3 orientMatrix;

		orientMatrix.FromEulerAnglesYXZ(Ogre::Radian(vRotate.x), Ogre::Radian(vRotate.y), Ogre::Radian(vRotate.z));
		Ogre::Quaternion orientation(orientMatrix);
		mModel->setOrientation(orientation);
	}
	VOID GameObject_Actor::SetMouseHover(BOOL bHover, uint32_t color, float crisperdingWidth)
	{
		assert(m_pResourceObject);

		if (!mModel) return;

		Ogre::ColourValue colour;

		colour.a = ((FLOAT)((color & 0xFF))) / 255.0f;
		colour.r = (((FLOAT)(color >> 24)) / 255.0f) * colour.a;
		colour.g = (((FLOAT)((color >> 16) & 0xFF)) / 255.0f) * colour.a;
		colour.b = (((FLOAT)((color >> 8) & 0xFF)) / 255.0f) * colour.a;
		mModel->setSelected(bHover == TRUE, colour, crisperdingWidth);
	}

	VOID GameObject_Actor::SetHitColour(const vector3& vColour, float fTime)
	{
		assert(m_pResourceObject);

		if (!mModel) return;

		mModel->addModelColour(Ogre::ColourValue(vColour.x, vColour.y, vColour.z), fTime);
	}

	void  GameObject_Actor::DeathEnterAnim(LPCTSTR szAniName, LPCTSTR szTagName, BOOL bLoop, FLOAT fFuseParam)
	{
		if (!mModel)
			return;
		mModel->removeShader();
		mModel->setEffectVisible(true);
		PlayAnimTag(szAniName, szTagName, bLoop, fFuseParam);
	}

	VOID GameObject_Actor::ApplyShader(LPCTSTR szShaderName)
	{
		if (!mModel)
			return;

		if (m_ActorProperty.m_defualtModel)
			m_ActorProperty.m_szShaderName = szShaderName;

		mModel->applyShader(szShaderName);
	}

	VOID GameObject_Actor::NormalShader()
	{
		if (!mModel)
			return;
		if (m_ActorProperty.m_defualtModel)
		{
			m_ActorProperty.m_szShaderName = "";
		}
		mModel->removeShader();
	}

	VOID GameObject_Actor::SetHideAllEffect(BOOL bFlag)
	{
		if (!mModel)
			return;
	}

	DynamicModelObject* GameObject_Actor::GetWxModel(enum_dummy_location locLocation)
	{
		if (!mModel) return NULL;

		m_wxModel = NULL;
		Ogre::String attributeName = "";

		switch (locLocation)
		{
		case EDL_NONE:
			m_wxModel = mModel;
			break;
		case EDL_LEFT:
			attributeName = "LeftWeaponObj";
			break;
		case EDL_RIGHT:
			attributeName = "RightWeaponObj";
			break;
		case EDL_BACK:
			attributeName = "WingObj";
			break;
		case EDL_TOP:
			attributeName = "TalismanObj";
			break;
		default:
			break;
		}

		if (locLocation == EDL_NONE)
			return m_wxModel;

		if (mModel && mModel->hasAttribute(attributeName))
			m_wxModel = Ogre::any_cast<DynamicModelObject*>(mModel->getAttribute(attributeName));

		return m_wxModel;
	}

	void GameObject_Actor::set_effect_obj()
	{
		if (!mModel)
			return;

		mModel->set_effect_obj();
	}

	void GameObject_Actor::active_all_effect()
	{
		if (!mModel)
			return;

		mModel->active_all_effect();
	}
}