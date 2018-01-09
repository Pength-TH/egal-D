// * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// * illegal driver
//7日
// * File: GameObject_Actor.h
// *
// * Author SeedEnginer
// * 
// * Desc:
// * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

#ifndef __GAME_OBJECT_ACTOR_INCLUDE__
#define __GAME_OBJECT_ACTOR_INCLUDE__

#pragma once

#include "IGameObject.h"
#include "Ogre.h"
#include "GameObject.h"
#include "OgrePlatform.h"
#include "IResourceObject.h"
#include "DynamicModelObject.h"
namespace SeedEngine
{

	struct EffectInfo
	{
	public:
		Ogre::String nameStr;
		Ogre::String locatorStr;
		bool bRoat;
		uint32_t  newId; //原来是空摸，后来换成实体模型之后添加的特效id
	};
	struct WeaponAnimation
	{
		WeaponAnimation()
		{
			szAniName = "";
			bLoop = false;
			fFuseParam = 0.3;
			fRate = 1.0;
		}
		Ogre::String szAniName;
		enum_dummy_location locLocation;
		BOOL bLoop;
		FLOAT fFuseParam;
		FLOAT fRate;
	};
	/************************************************************************************************************/
	typedef Ogre::map<Ogre::String, Ogre::String>::type				AttributeMap;
	typedef Ogre::map<enum_dummy_location, WeaponAnimation>::type	WeaponAnimationMap;
	typedef Ogre::map<int, EffectInfo>::type						EffectMap;
	typedef Ogre::map<IGameObject*, Ogre::String>::type				AttachMap;
	/************************************************************************************************************/

	class GameObject_Actor;
	struct ActorProperty
	{
	public:
		void init()
		{
			m_ActorFileName = "";
			m_sAniName = "";
			m_sSkillName = "";
			m_nLoop = false;
			m_attributeMap.clear();
			m_effectMap.clear();
			m_attachMap.clear();
			m_weaponMap.clear();
			m_attributeNeedCheckMap.clear();
			m_defualtModel = false;
			m_fActionRate = 1.0;
			m_szShaderName = "";
			m_bVisible = true;
			m_bEffectVisible = TRUE;
			m_bUIVisible = false;
			pAniFunc = NULL;
			pShakeFunc = NULL;
			dwAniEndParam = 0;
			dwShakeParam = 0;
			m_game_object_actor = NULL;
		}
		Ogre::String         m_ActorFileName;
		Ogre::String	     m_sAniName; //最后一次播放的动作名
		Ogre::String		 m_sSkillName;
		bool            m_nLoop;
		AttributeMap    m_attributeMap;
		AttributeMap    m_attributeNeedCheckMap;
		EffectMap		m_effectMap;
		AttachMap       m_attachMap;
		WeaponAnimationMap m_weaponMap;
		bool			m_defualtModel;

		float			m_fActionRate;
		vector3        m_Position;
		vector3        m_roate;
		uint32_t			m_lightMask;

		Ogre::String			m_szShaderName;

		bool            m_bVisible;
		BOOL			m_bEffectVisible;
		bool            m_bUIVisible;
		enum_dummy_location     m_Localation;
	
		//处理回掉
		CallBack_On_Animation_End pAniFunc;
		DWORD dwAniEndParam;
		CallBack_On_Animation_Shake pShakeFunc;
		DWORD dwShakeParam;

		//处理attach
		GameObject_Actor* m_game_object_actor;
	};
	/************************************************************************************************************/
	/************************************************************************************************************/
	/************************************************************************************************************/
	class GameObject_Actor : public GameObject
	{
	public:
		virtual BOOL GetDownload();

		/** death effect */
		virtual VOID DeathEnterAnim(LPCTSTR szAniName, LPCTSTR szTagName, BOOL bLoop, FLOAT fFuseParam = 0.3);

		/** base */
		virtual enum_game_object_type GetType(VOID) const
		{
			return EGOT_CHARACTOR;
		};
		virtual VOID Destroy(VOID);
		virtual void SetUIObjectVisible(BOOL bFlag);
		virtual void SetCreateRoleSpecialVisible(BOOL bFlag);
		virtual VOID SetVisible(BOOL bVisible, enum_dummy_location eInfoType = EDL_NONE);
		virtual VOID SetEffectVisible(BOOL bVisible, enum_dummy_location eInfoType = EDL_NONE);
		virtual BOOL GetVisible() const;

		/** transform */
		virtual VOID SetPosition(const vector3& vPos);
		virtual VOID SetOrientation(const vector3& vRotate);
		virtual VOID SetEulerOrientation(const vector3& vRotate);
		virtual VOID SetScale(const vector3& vScale, enum_dummy_location eInfoType = EDL_NONE, float fTime = 0.0);

		/** set other obj property */
		virtual VOID SetProperty(LPCTSTR szPropertyName, LPCTSTR szPropertyValue, enum_dummy_location eInfoType = EDL_NONE);
		virtual VOID GetProperty(LPCTSTR szName, char*  szValue, size_t valueLength, enum_dummy_location eInfoType = EDL_NONE);

		//附加一个特效
		virtual uint32_t AddEffect(LPCTSTR effectName, LPCTSTR szBindName, enum_dummy_location eInfoType = EDL_NONE, BOOL bRot = FALSE);
		virtual VOID DelEffect(uint32_t effect, enum_remove_type removeEffectType = ERT_NOW, enum_dummy_location eInfoType = EDL_NONE);
		virtual VOID DelAllEffect(enum_dummy_location eInfoType = EDL_NONE);

		virtual void Attach_Object(IGameObject *pObject, LPCTSTR szAttachLocator);
		virtual void Detach_Object(IGameObject *pObject);

		/** 设置模型名字 */
		virtual VOID SetResourceFile(LPCTSTR szActorFile);
		virtual BOOL HasSetResourceFile();

		/** 播放动画以及标签 */
		virtual VOID EnterWeaponAnim(LPCTSTR szAniName, enum_dummy_location locLocation, BOOL bLoop, FLOAT fFuseParam = 0.3);
		/** 播放标签，只播放标签，不播放动作 */
		virtual VOID PlayTag(LPCTSTR szfTagName, BOOL bLoop, BOOL bTimeControl);
		/** 播放动作，只播放动作  */
		virtual VOID PlayAnim(LPCTSTR szAniName, BOOL bLoop, FLOAT fFuseParam = 0.3, FLOAT fRadian = 0.0);
		/** 播放动作标签 */
		virtual VOID PlayAnimTag(LPCTSTR szAniName, LPCTSTR szTagName, BOOL bLoop, FLOAT fFuseParam = 0.3, FLOAT fRadian = 0.0);
		/** 播放动作双标签 */
		virtual VOID PlayAnimDoubleTag(LPCTSTR szAniName, LPCTSTR szfTagName, LPCTSTR szsTagName, BOOL bLoop, FLOAT fFuseParam = 0.3, FLOAT fRadian = 0.0);

		/** 取得任务模型上的某关键点位置 */
		virtual VOID GetLocator(LPCTSTR szName, vector3& fvPosition);
		/** 取得"头顶状态点"在屏幕上的位置,如果返回FALSE，表示在屏幕之外,或者没有该点 */
		virtual BOOL GetInfoBoardPos(vector3& ivPos, const vector3 *pvObjPos = NULL, const float fObligeHeight = -1);

		/** 切换动画的播放速度 fRate - 缩放比率 */
		virtual VOID ChangeActionRate(FLOAT fRate, enum_dummy_location locLocation = EDL_NONE);

		/** 设置缺省动作 */
		virtual VOID SetDefaultAnim(LPCTSTR szAnimName);

		/** 设置UI VisibleFlag */
		virtual VOID SetUIVisibleFlag(VOID);

		virtual VOID SetTransparent(FLOAT fTransparency, FLOAT fTime);

		/** 设置连线特效的目标点 */
		virtual VOID SetEffectExtraTransformInfos(uint32_t uEffect, vector3& fvPosition);

		/** 设置是否出于鼠标Hover状态 */
		virtual VOID SetMouseHover(BOOL bHover, uint32_t color = 0xFF0000FF, float crisperdingWidth = 0.04f);
		virtual VOID SetHitColour(const vector3& vColour, float fTime);

		/** set shader */
		virtual VOID ApplyShader(LPCTSTR szShaderName);
		virtual VOID NormalShader();

		/**  Event */
		/** 设置角色动画结束通知*/
		virtual void SetAnimationEndEvent(CallBack_On_Animation_End pFunc, DWORD dwParam);
		/** 设置角色动画打击点通知*/
		virtual void SetAnimationShakeEvent(CallBack_On_Animation_Shake pFunc, DWORD dwParam);

		VOID SetLightMask(uint32_t flag);

		virtual void Update();
		virtual void UpdateProperty(Ogre::String name);
		virtual VOID SetHideAllEffect(BOOL bFlag);

		DynamicModelObject* GetWxModel(enum_dummy_location locLocation = EDL_NONE);

		void set_effect_obj();
		void active_all_effect();
	protected:
		DynamicModelObject*					mModel;
		DynamicModelObject*					m_wxModel;
		uint32_t							m_wxEffect;

		IResourceObject*						m_pResourceObject;

		BOOL							m_bVisible;
		BOOL                            m_bUiVisible;
		ActorProperty					m_ActorProperty;

		Ogre::String                    m_ValueStr;

		bool                            m_bDownloaded;
	public:
		GameObject_Actor();
		virtual ~GameObject_Actor();
	};
}
#endif
