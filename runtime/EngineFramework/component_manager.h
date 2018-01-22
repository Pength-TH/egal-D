#ifndef _component_manager_h_
#define _component_manager_h_

#include "common/egal-d.h"
#include "runtime/EngineFramework/plugin_manager.h"
#include "common/serializer/serializer.h"

namespace egal
{
	class ComponentManager
	{
	public:
		typedef void (SceneManager::*Serialize)(ISerializer&, ComponentHandle);
		typedef void (SceneManager::*Deserialize)(IDeserializer&, GameObject, int);

		struct ComponentTypeEntry
		{
			SceneManager* scene = nullptr;
			void (SceneManager::*serialize)(ISerializer&, ComponentHandle);
			void (SceneManager::*deserialize)(IDeserializer&, GameObject, int);
		};
		enum { ENTITY_NAME_MAX_LENGTH = 32 };

	public:
		explicit ComponentManager(IAllocator& allocator);
		~ComponentManager();

		IAllocator& getAllocator() { return m_allocator; }

		void emplaceGameObject(GameObject game_object);

		GameObject createGameObject(const float3& position, const Quaternion& rotation);
		void destroyGameObject(GameObject game_object);
		GameObject getFirstGameObject() const;
		GameObject getNextGameObject(GameObject game_object) const;

		const char* getGameObjectName(GameObject game_object) const;
		GameObject getGameObjectByName(const char* name);
		void setGameObjectName(GameObject game_object, const char* name);
		bool hasGameObject(GameObject game_object) const;


		void addComponent(GameObject game_object, ComponentType component_type, SceneManager* scene, ComponentHandle index);
		void destroyComponent(GameObject game_object, ComponentType component_type, SceneManager* scene, ComponentHandle index);
		bool hasComponent(GameObject game_object, ComponentType component_type) const;
		ComponentUID getComponent(GameObject game_object, ComponentType type) const;
		ComponentUID getFirstComponent(GameObject game_object) const;
		ComponentUID getNextComponent(const ComponentUID& cmp) const;

		ComponentTypeEntry& registerComponentType(ComponentType type) { return m_component_type_map[type.index]; }
		template <typename T1, typename T2>
		void registerComponentType(ComponentType type, SceneManager* scene, T1 serialize, T2 deserialize)
		{
			m_component_type_map[type.index].scene = scene;
			m_component_type_map[type.index].serialize = static_cast<Serialize>(serialize);
			m_component_type_map[type.index].deserialize = static_cast<Deserialize>(deserialize);
		}

		bool isDescendant(GameObject ancestor, GameObject descendant) const;
		GameObject getParent(GameObject game_object) const;
		GameObject getFirstChild(GameObject game_object) const;
		GameObject getNextSibling(GameObject game_object) const;

		Transform getLocalTransform(GameObject game_object) const;
		float getLocalScale(GameObject game_object) const;
		void setParent(GameObject parent, GameObject child);
		void setLocalPosition(GameObject game_object, const float3& pos);
		void setLocalRotation(GameObject game_object, const Quaternion& rot);
		void setLocalTransform(GameObject game_object, const Transform& transform);
		Transform computeLocalTransform(GameObject parent, const Transform& global_transform) const;
		void setMatrix(GameObject game_object, const Matrix& mtx);
		Matrix getPositionAndRotation(GameObject game_object) const;
		Matrix getMatrix(GameObject game_object) const;
		void setTransform(GameObject game_object, const Transform& transform);
		void setTransformKeepChildren(GameObject game_object, const Transform& transform);
		void setTransform(GameObject game_object, const float3& pos, const Quaternion& rot, float scale);
		Transform getTransform(GameObject game_object) const;
		void setRotation(GameObject game_object, float x, float y, float z, float w);
		void setRotation(GameObject game_object, const Quaternion& rot);
		void setPosition(GameObject game_object, float x, float y, float z);
		void setPosition(GameObject game_object, const float3& pos);
		void setScale(GameObject game_object, float scale);
		float getScale(GameObject game_object) const;
		const float3& getPosition(GameObject game_object) const;
		const Quaternion& getRotation(GameObject game_object) const;
		
		const char* getName() const { return m_name; }
		void setName(const char* name)
		{
			m_name = name;
		}

		TDelegateList<void(GameObject)>& GameObjectTransformed() 
		{
			return m_game_object_moved; 
		}

		TDelegateList<void(GameObject)>& GameObjectCreated() 
		{ 
			return m_game_object_created; 
		}

		TDelegateList<void(GameObject)>& GameObjectDestroyed() 
		{ 
			return m_game_object_destroyed; 
		}

		TDelegateList<void(const ComponentUID&)>& componentDestroyed() 
		{ 
			return m_component_destroyed;
		}

		TDelegateList<void(const ComponentUID&)>& componentAdded() 
		{ 
			return m_component_added;
		}

		void serializeComponent(ISerializer& serializer, ComponentType type, ComponentHandle cmp);
		void deserializeComponent(IDeserializer& serializer, GameObject game_object, ComponentType type, int scene_version);
		void serialize(WriteBinary& serializer);
		void deserialize(ReadBinary& serializer);

		SceneManager* getScene(ComponentType type) const;
		SceneManager* getScene(e_uint32 hash) const;
		TArrary<SceneManager*>& getScenes();
		void addScene(SceneManager* scene);
	private:
		void transformGameObject(GameObject game_object, bool update_local);
		void updateGlobalTransform(GameObject game_object);

		struct Hierarchy
		{
			GameObject game_object;
			GameObject parent;
			GameObject first_child;
			GameObject next_sibling;

			Transform local_transform;
		};

		struct GameObjectData
		{
			GameObjectData() {}

			float3 position;
			Quaternion rotation;

			int hierarchy;
			int name;

			union
			{
				struct
				{
					float scale;
					e_uint64 components;
				};
				struct
				{
					int prev;
					int next;
				};
			};
			bool valid;
		};

		struct GameObjectName
		{
			GameObject game_object;
			char name[ENTITY_NAME_MAX_LENGTH];
		};

	public:
		IAllocator&									m_allocator;
		ComponentTypeEntry							m_component_type_map[ComponentType::MAX_TYPES_COUNT];
		
		TArrary<SceneManager*>						m_scenes;
		TArrary<GameObjectData>						m_game_objects;
		TArrary<Hierarchy>							m_hierarchy;
		TArrary<GameObjectName>						m_names;
		TDelegateList<void(GameObject)>				m_game_object_moved;
		TDelegateList<void(GameObject)>				m_game_object_created;
		TDelegateList<void(GameObject)>				m_game_object_destroyed;
		TDelegateList<void(const ComponentUID&)>	m_component_destroyed;
		TDelegateList<void(const ComponentUID&)>	m_component_added;

		int											m_first_free_slot;
		StaticString<64>							m_name;
	};

}

#endif
