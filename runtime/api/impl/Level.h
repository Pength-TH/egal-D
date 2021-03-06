// * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// * illegal driver
//2日
// * File: Scene.h
// *
// * Author SeedEnginer
// * 
// * Desc:
// * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#ifndef __SCENE_H__
#define __SCENE_H__

#include "runtime/api/impl/CollisionManager.h"
#include "common/egal-d.h"
#include "runtime/api/rapidxml-1.13/rapidxml.hpp"

class SkeletonEntityManager;
namespace SeedEngine
{
	class Level : public ILevel
	{
	public:

		Level();
		virtual ~Level();

		virtual BOOL play_scene_anim(LPCTSTR szAniName, BOOL bLoop, Real fTime);
		virtual BOOL play_screen_effect(LPCTSTR effectName);

		virtual BOOL play_3dsound(LPCTSTR effectName, BOOL bLoop = FALSE);
		virtual BOOL change_bgm(LPCTSTR effectName, BOOL bLoop = FALSE);

		virtual VOID load_level(LPCTSTR level_file);
		virtual VOID unload();

		virtual VOID refreshPageObject();

		virtual Real getHeight(float x, float z);
		bool getIntersects(Real winx, Real winy, vector3& position, vector3* normal = 0, bool allowOutside = false) const;

		//TerrainGroup* GetTerrainGroupHandle()
		//{
		//	return mTerrainGroup;
		//}
		CollisionManager* GetWalkPlaneMgr()
		{
			return mWalkPlaneMgr;
		}
		void RefreshPageObject();

		void ApplyMainPos(vector3& vecPos);

		const std::string& getSceneFileName()
		{
			return mSceneFileName;
		}

		void LoadLightMapOnDownloaded(bool flag);

		void setLoadLightMap(bool flag)
		{
			mLoadLightMap = flag;
		}

		bool update(Real fElapsedTime);

		bool getPercentage(float& percentage);

		float mLoadTime;


		bool is_loaded(){ return m_loaded; }
	protected:
		bool m_loaded;

		void parseLevel();

		void AddSceneResourceLocation();
		void RemoveSceneResourceLocation();

		egal::GameObject* ParseObjectNode(rapidxml::xml_node<>* pNode);

		CollisionManager*  mWalkPlaneMgr;

		typedef std::list<egal::GameObject*> ObjectList;
		ObjectList mObjectList;
		ObjectList mALLObjectList;

		void CreateAllNoPageObject();
		void DestoryAllNoPageobject();

		std::string mSceneFileName;
		std::string mSceneName;

		/** 加载场景的时候分帧加载 */
		ObjectList mPagesObjectList;
		bool mbPageObjectsLoadEnd;

		bool mLoadScene;

		bool mUseLoadFrame;		//是否启用分帧加载
		int mCurrentNum;		//当前加载的个数
		int mTotalNum;			//总共需要加载的个数

		bool mLoadLightMap;

		Real fReloadSceneTime;

		//SkeletonEntityManager* mSkeletonEntityManager;
	};
}
#endif 
