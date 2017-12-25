#ifndef _universe_h_
#define _universe_h_

#include "common/egal-d.h"
#include "common/framework/plugin_manager.h"
#include "common/serializer/serializer.h"

namespace egal
{
	class Universe
	{
	public:
		typedef void (IScene::*Serialize)(ISerializer&, ComponentHandle);
		typedef void (IScene::*Deserialize)(IDeserializer&, GameObject, int);
		struct ComponentTypeEntry
		{
			IScene* scene = nullptr;
			void (IScene::*serialize)(ISerializer&, ComponentHandle);
			void (IScene::*deserialize)(IDeserializer&, GameObject, int);
		};

		enum { ENTITY_NAME_MAX_LENGTH = 32 };

	public:
		explicit Universe(IAllocator& allocator);
		~Universe();

		IAllocator& getAllocator() { return m_allocator; }
		void emplaceEntity(GameObject entity);
		GameObject createEntity(const float3& position, const Quaternion& rotation);
		void destroyEntity(GameObject entity);
		void addComponent(GameObject entity, ComponentType component_type, IScene* scene, ComponentHandle index);
		void destroyComponent(GameObject entity, ComponentType component_type, IScene* scene, ComponentHandle index);
		bool hasComponent(GameObject entity, ComponentType component_type) const;
		ComponentUID getComponent(GameObject entity, ComponentType type) const;
		ComponentUID getFirstComponent(GameObject entity) const;
		ComponentUID getNextComponent(const ComponentUID& cmp) const;
		ComponentTypeEntry& registerComponentType(ComponentType type) { return m_component_type_map[type.index]; }
		template <typename T1, typename T2>
		void registerComponentType(ComponentType type, IScene* scene, T1 serialize, T2 deserialize)
		{
			m_component_type_map[type.index].scene = scene;
			m_component_type_map[type.index].serialize = static_cast<Serialize>(serialize);
			m_component_type_map[type.index].deserialize = static_cast<Deserialize>(deserialize);
		}

		GameObject getFirstEntity() const;
		GameObject getNextEntity(GameObject entity) const;
		const char* getEntityName(GameObject entity) const;
		GameObject getEntityByName(const char* name);
		void setEntityName(GameObject entity, const char* name);
		bool hasEntity(GameObject entity) const;

		bool isDescendant(GameObject ancestor, GameObject descendant) const;
		GameObject getParent(GameObject entity) const;
		GameObject getFirstChild(GameObject entity) const;
		GameObject getNextSibling(GameObject entity) const;
		Transform getLocalTransform(GameObject entity) const;
		float getLocalScale(GameObject entity) const;
		void setParent(GameObject parent, GameObject child);
		void setLocalPosition(GameObject entity, const float3& pos);
		void setLocalRotation(GameObject entity, const Quaternion& rot);
		void setLocalTransform(GameObject entity, const Transform& transform);
		Transform computeLocalTransform(GameObject parent, const Transform& global_transform) const;

		void setMatrix(GameObject entity, const Matrix& mtx);
		Matrix getPositionAndRotation(GameObject entity) const;
		Matrix getMatrix(GameObject entity) const;
		void setTransform(GameObject entity, const Transform& transform);
		void setTransformKeepChildren(GameObject entity, const Transform& transform);
		void setTransform(GameObject entity, const float3& pos, const Quaternion& rot, float scale);
		Transform getTransform(GameObject entity) const;
		void setRotation(GameObject entity, float x, float y, float z, float w);
		void setRotation(GameObject entity, const Quaternion& rot);
		void setPosition(GameObject entity, float x, float y, float z);
		void setPosition(GameObject entity, const float3& pos);
		void setScale(GameObject entity, float scale);
		//GameObject instantiatePrefab(const PrefabResource& prefab,
		//	const float3& pos,
		//	const Quaternion& rot,
		//	float scale);
		float getScale(GameObject entity) const;
		const float3& getPosition(GameObject entity) const;
		const Quaternion& getRotation(GameObject entity) const;
		const char* getName() const { return m_name; }
		void setName(const char* name)
		{
			m_name = name;
		}

		TDelegateList<void(GameObject)>& entityTransformed() { return m_entity_moved; }
		TDelegateList<void(GameObject)>& entityCreated() { return m_entity_created; }
		TDelegateList<void(GameObject)>& entityDestroyed() { return m_entity_destroyed; }
		TDelegateList<void(const ComponentUID&)>& componentDestroyed() { return m_component_destroyed; }
		TDelegateList<void(const ComponentUID&)>& componentAdded() { return m_component_added; }

		void serializeComponent(ISerializer& serializer, ComponentType type, ComponentHandle cmp);
		void deserializeComponent(IDeserializer& serializer, Entity entity, ComponentType type, int scene_version);
		void serialize(WriteBinary& serializer);
		void deserialize(ReadBinary& serializer);

		IScene* getScene(ComponentType type) const;
		IScene* getScene(e_uint32 hash) const;
		TVector<IScene*>& getScenes();
		void addScene(IScene* scene);

	private:
		void transformEntity(GameObject entity, bool update_local);
		void updateGlobalTransform(GameObject entity);

		struct Hierarchy
		{
			GameObject entity;
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
			GameObject entity;
			char name[ENTITY_NAME_MAX_LENGTH];
		};

	private:
		IAllocator& m_allocator;
		ComponentTypeEntry m_component_type_map[ComponentType::MAX_TYPES_COUNT];
		TVector<IScene*> m_scenes;
		TVector<GameObjectData> m_entities;
		TVector<Hierarchy> m_hierarchy;
		TVector<GameObjectName> m_names;
		TDelegateList<void(GameObject)> m_entity_moved;
		TDelegateList<void(GameObject)> m_entity_created;
		TDelegateList<void(GameObject)> m_entity_destroyed;
		TDelegateList<void(const ComponentUID&)> m_component_destroyed;
		TDelegateList<void(const ComponentUID&)> m_component_added;
		int m_first_free_slot;
		StaticString<64> m_name;
	};

}

#endif
