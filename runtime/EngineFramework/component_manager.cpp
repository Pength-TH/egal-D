#include "runtime/EngineFramework/component_manager.h"
#include "runtime/EngineFramework/scene_manager.h"
namespace egal
{
	const ComponentUID ComponentUID::INVALID(INVALID_GAME_OBJECT, { -1 }, 0, INVALID_COMPONENT);

	ComponentManager::ComponentManager(IAllocator& allocator)
		: m_component_added(allocator)
		, m_game_objects(allocator)
		, m_component_destroyed(allocator)
		, m_game_object_created(allocator)
		, m_game_object_destroyed(allocator)
		, m_game_object_moved(allocator)
		, m_names(allocator)
		, m_first_free_slot(-1)
		, m_scenes(allocator)
		, m_hierarchy(allocator)
		, m_allocator(allocator)
	{
		//m_component_added = TDelegateList<void(const ComponentUID&)>(allocator);
		//m_game_objects = TVector<GameObjectData>(allocator);

		//m_scenes = TVector<SceneManager*>(allocator);
		//m_hierarchy = TVector<Hierarchy>(allocator);
		//m_names = TVector<GameObjectName>(allocator);
		//m_game_object_moved = TDelegateList<void(GameObject)>(allocator);
		//m_game_object_created = TDelegateList<void(GameObject)>(allocator);
		//m_game_object_destroyed = TDelegateList<void(GameObject)>(allocator);
		//m_component_destroyed = TDelegateList<void(const ComponentUID&)>(allocator);

		m_game_objects.reserve(RESERVED_ENTITIES_COUNT);
	}

	ComponentManager::~ComponentManager()
	{

	}

	SceneManager* ComponentManager::getScene(ComponentType type) const
	{
		return m_component_type_map[type.index].scene;
	}

	SceneManager* ComponentManager::getScene(e_uint32 hash) const
	{
		for (auto* scene : m_scenes)
		{
			if (crc32(scene->getRenderer().getName()) == hash)
			{
				return scene;
			}
		}
		return nullptr;
	}

	TArrary<SceneManager*>& ComponentManager::getScenes()
	{
		return m_scenes;
	}

	void ComponentManager::addScene(SceneManager* scene)
	{
		m_scenes.push_back(scene);
	}

	const float3& ComponentManager::getPosition(GameObject game_object) const
	{
		return m_game_objects[game_object.index].position;
	}

	const Quaternion& ComponentManager::getRotation(GameObject game_object) const
	{
		return m_game_objects[game_object.index].rotation;
	}

	void ComponentManager::transformGameObject(GameObject game_object, bool update_local)
	{
		int hierarchy_idx = m_game_objects[game_object.index].hierarchy;
		GameObjectTransformed().invoke(game_object);
		if (hierarchy_idx >= 0)
		{
			Hierarchy& h = m_hierarchy[hierarchy_idx];
			Transform my_transform = getTransform(game_object);
			if (update_local && h.parent.isValid())
			{
				Transform parent_tr = getTransform(h.parent);
				h.local_transform = (parent_tr.inverted() * my_transform);
			}

			GameObject child = h.first_child;
			while (child.isValid())
			{
				Hierarchy& child_h = m_hierarchy[m_game_objects[child.index].hierarchy];
				Transform abs_tr = my_transform * child_h.local_transform;
				GameObjectData& child_data = m_game_objects[child.index];
				child_data.position = abs_tr.pos;
				child_data.rotation = abs_tr.rot;
				child_data.scale = abs_tr.scale;
				transformGameObject(child, false);

				child = child_h.next_sibling;
			}
		}
	}

	void ComponentManager::setRotation(GameObject game_object, const Quaternion& rot)
	{
		m_game_objects[game_object.index].rotation = rot;
		transformGameObject(game_object, true);
	}

	void ComponentManager::setRotation(GameObject game_object, float x, float y, float z, float w)
	{
		m_game_objects[game_object.index].rotation.set(x, y, z, w);
		transformGameObject(game_object, true);
	}

	bool ComponentManager::hasGameObject(GameObject game_object) const
	{
		return game_object.index >= 0 && game_object.index < m_game_objects.size() && m_game_objects[game_object.index].valid;
	}

	void ComponentManager::setMatrix(GameObject game_object, const Matrix& mtx)
	{
		GameObjectData& out = m_game_objects[game_object.index];
		mtx.decompose(out.position, out.rotation, out.scale);
		transformGameObject(game_object, true);
	}

	Matrix ComponentManager::getPositionAndRotation(GameObject game_object) const
	{
		auto& transform = m_game_objects[game_object.index];
		Matrix mtx = transform.rotation.toMatrix();
		mtx.setTranslation(transform.position);
		return mtx;
	}

	void ComponentManager::setTransformKeepChildren(GameObject game_object, const Transform& transform)
	{
		auto& tmp = m_game_objects[game_object.index];
		tmp.position = transform.pos;
		tmp.rotation = transform.rot;
		tmp.scale = transform.scale;

		int hierarchy_idx = m_game_objects[game_object.index].hierarchy;
		GameObjectTransformed().invoke(game_object);
		if (hierarchy_idx >= 0)
		{
			Hierarchy& h = m_hierarchy[hierarchy_idx];
			Transform my_transform = getTransform(game_object);
			if (h.parent.isValid())
			{
				Transform parent_tr = getTransform(h.parent);
				h.local_transform = parent_tr.inverted() * my_transform;
			}

			GameObject child = h.first_child;
			while (child.isValid())
			{
				Hierarchy& child_h = m_hierarchy[m_game_objects[child.index].hierarchy];

				child_h.local_transform = my_transform.inverted() * getTransform(child);
				child = child_h.next_sibling;
			}
		}

	}

	void ComponentManager::setTransform(GameObject game_object, const Transform& transform)
	{
		auto& tmp = m_game_objects[game_object.index];
		tmp.position = transform.pos;
		tmp.rotation = transform.rot;
		tmp.scale = transform.scale;
		transformGameObject(game_object, true);
	}

	void ComponentManager::setTransform(GameObject game_object, const float3& pos, const Quaternion& rot, float scale)
	{
		auto& tmp = m_game_objects[game_object.index];
		tmp.position = pos;
		tmp.rotation = rot;
		tmp.scale = scale;
		transformGameObject(game_object, true);
	}

	Transform ComponentManager::getTransform(GameObject game_object) const
	{
		auto& transform = m_game_objects[game_object.index];
		return { transform.position, transform.rotation, transform.scale };
	}

	Matrix ComponentManager::getMatrix(GameObject game_object) const
	{
		auto& transform = m_game_objects[game_object.index];
		Matrix mtx = transform.rotation.toMatrix();
		mtx.setTranslation(transform.position);
		mtx.multiply3x3(transform.scale);
		return mtx;
	}

	void ComponentManager::setPosition(GameObject game_object, float x, float y, float z)
	{
		auto& transform = m_game_objects[game_object.index];
		transform.position.set(x, y, z);
		transformGameObject(game_object, true);
	}

	void ComponentManager::setPosition(GameObject game_object, const float3& pos)
	{
		auto& transform = m_game_objects[game_object.index];
		transform.position = pos;
		transformGameObject(game_object, true);
	}

	void ComponentManager::setGameObjectName(GameObject game_object, const char* name)
	{
		int name_idx = m_game_objects[game_object.index].name;
		if (name_idx < 0)
		{
			if (name[0] == '\0') 
				return;
			m_game_objects[game_object.index].name = m_names.size();
			GameObjectName& name_data = m_names.emplace();
			name_data.game_object = game_object;
			StringUnitl::copyString(name_data.name, name);
		}
		else
		{
			StringUnitl::copyString(m_names[name_idx].name, name);
		}
	}

	const char* ComponentManager::getGameObjectName(GameObject game_object) const
	{
		int name_idx = m_game_objects[game_object.index].name;
		if (name_idx < 0) 
			return "";
		return m_names[name_idx].name;
	}

	GameObject ComponentManager::getGameObjectByName(const char* name)
	{
		for (const GameObjectName& name_data : m_names)
		{
			if (StringUnitl::equalStrings(name_data.name, name)) 
				return name_data.game_object;
		}
		return INVALID_GAME_OBJECT;
	}

	void ComponentManager::emplaceGameObject(GameObject game_object)
	{
		while (m_game_objects.size() <= game_object.index)
		{
			GameObjectData& data = m_game_objects.emplace();
			data.valid = false;
			data.prev = -1;
			data.name = -1;
			data.hierarchy = -1;
			data.next = m_first_free_slot;
			data.scale = -1;
			if (m_first_free_slot >= 0)
			{
				m_game_objects[m_first_free_slot].prev = m_game_objects.size() - 1;
			}
			m_first_free_slot = m_game_objects.size() - 1;
		}
		if (m_first_free_slot == game_object.index)
		{
			m_first_free_slot = m_game_objects[game_object.index].next;
		}
		if (m_game_objects[game_object.index].prev >= 0)
		{
			m_game_objects[m_game_objects[game_object.index].prev].next = m_game_objects[game_object.index].next;
		}
		if (m_game_objects[game_object.index].next >= 0)
		{
			m_game_objects[m_game_objects[game_object.index].next].prev = m_game_objects[game_object.index].prev;
		}
		GameObjectData& data = m_game_objects[game_object.index];
		data.position.set(0, 0, 0);
		data.rotation.set(0, 0, 0, 1);
		data.scale = 1;
		data.name = -1;
		data.hierarchy = -1;
		data.components = 0;
		data.valid = true;
		m_game_object_created.invoke(game_object);
	}

	GameObject ComponentManager::createGameObject(const float3& position, const Quaternion& rotation)
	{
		GameObjectData* data;
		GameObject game_object;
		if (m_first_free_slot >= 0)
		{
			data = &m_game_objects[m_first_free_slot];
			game_object.index = m_first_free_slot;
			if (data->next >= 0)
				m_game_objects[data->next].prev = -1;
			m_first_free_slot = data->next;
		}
		else
		{
			game_object.index = m_game_objects.size();
			data = &m_game_objects.emplace();
		}
		data->position = position;
		data->rotation = rotation;
		data->scale = 1;
		data->name = -1;
		data->hierarchy = -1;
		data->components = 0;
		data->valid = true;
		m_game_object_created.invoke(game_object);

		return game_object;
	}

	void ComponentManager::destroyGameObject(GameObject game_object)
	{
		if (!game_object.isValid()) 
			return;

		GameObjectData& game_object_data = m_game_objects[game_object.index];
		ASSERT(game_object_data.valid);
		for (GameObject first_child = getFirstChild(game_object); first_child.isValid(); first_child = getFirstChild(game_object))
		{
			setParent(INVALID_GAME_OBJECT, first_child);
		}
		setParent(INVALID_GAME_OBJECT, game_object);

		e_uint64 mask = game_object_data.components;
		for (int i = 0; i < ComponentType::MAX_TYPES_COUNT; ++i)
		{
			if ((mask & ((e_uint64)1 << i)) != 0)
			{
				ComponentType type = { i };
				auto original_mask = mask;
				SceneManager* scene = m_component_type_map[i].scene;
				scene->destroyComponent(scene->getComponent(game_object, type), type);
				mask = game_object_data.components;
				ASSERT(original_mask != mask);
			}
		}

		game_object_data.next = m_first_free_slot;
		game_object_data.prev = -1;
		game_object_data.hierarchy = -1;

		game_object_data.valid = false;
		if (m_first_free_slot >= 0)
		{
			m_game_objects[m_first_free_slot].prev = game_object.index;
		}

		if (game_object_data.name >= 0)
		{
			m_game_objects[m_names.back().game_object.index].name = game_object_data.name;
			m_names.eraseFast(game_object_data.name);
			game_object_data.name = -1;
		}

		m_first_free_slot = game_object.index;
		m_game_object_destroyed.invoke(game_object);
	}


	GameObject ComponentManager::getFirstGameObject() const
	{
		for (int i = 0; i < m_game_objects.size(); ++i)
		{
			if (m_game_objects[i].valid)
				return { i };
		}
		return INVALID_GAME_OBJECT;
	}


	GameObject ComponentManager::getNextGameObject(GameObject game_object) const
	{
		for (int i = game_object.index + 1; i < m_game_objects.size(); ++i)
		{
			if (m_game_objects[i].valid) 
				return { i };
		}
		return INVALID_GAME_OBJECT;
	}


	GameObject ComponentManager::getParent(GameObject game_object) const
	{
		int idx = m_game_objects[game_object.index].hierarchy;
		if (idx < 0) 
			return INVALID_GAME_OBJECT;
		return m_hierarchy[idx].parent;
	}


	GameObject ComponentManager::getFirstChild(GameObject game_object) const
	{
		int idx = m_game_objects[game_object.index].hierarchy;
		if (idx < 0) 
			return INVALID_GAME_OBJECT;
		return m_hierarchy[idx].first_child;
	}


	GameObject ComponentManager::getNextSibling(GameObject game_object) const
	{
		int idx = m_game_objects[game_object.index].hierarchy;
		if (idx < 0) 
			return INVALID_GAME_OBJECT;
		return m_hierarchy[idx].next_sibling;
	}


	bool ComponentManager::isDescendant(GameObject ancestor, GameObject descendant) const
	{
		if (!ancestor.isValid()) 
			return false;
		for (GameObject e = getFirstChild(ancestor); e.isValid(); e = getNextSibling(e))
		{
			if (e == descendant) 
				return true;
			if (isDescendant(e, descendant)) 
				return true;
		}

		return false;
	}


	void ComponentManager::setParent(GameObject new_parent, GameObject child)
	{
		bool would_create_cycle = isDescendant(child, new_parent);
		if (would_create_cycle)
		{
			log_error("Engine Hierarchy can not contains a cycle.");
			return;
		}

		auto collectGarbage = [this](GameObject game_object) 
		{
			Hierarchy& h = m_hierarchy[m_game_objects[game_object.index].hierarchy];
			if (h.parent.isValid())
				return;
			if (h.first_child.isValid()) 
				return;

			const Hierarchy& last = m_hierarchy.back();
			m_game_objects[last.game_object.index].hierarchy = m_game_objects[game_object.index].hierarchy;
			m_game_objects[game_object.index].hierarchy = -1;
			h = last;
			m_hierarchy.pop_back();
		};

		int child_idx = m_game_objects[child.index].hierarchy;

		if (child_idx >= 0)
		{
			GameObject old_parent = m_hierarchy[child_idx].parent;

			if (old_parent.isValid())
			{
				Hierarchy& old_parent_h = m_hierarchy[m_game_objects[old_parent.index].hierarchy];
				GameObject* x = &old_parent_h.first_child;
				while (x->isValid())
				{
					if (*x == child)
					{
						*x = getNextSibling(child);
						break;
					}
					x = &m_hierarchy[m_game_objects[x->index].hierarchy].next_sibling;
				}
				m_hierarchy[child_idx].parent = INVALID_GAME_OBJECT;
				m_hierarchy[child_idx].next_sibling = INVALID_GAME_OBJECT;
				collectGarbage(old_parent);
				child_idx = m_game_objects[child.index].hierarchy;
			}
		}
		else if (new_parent.isValid())
		{
			child_idx = m_hierarchy.size();
			m_game_objects[child.index].hierarchy = child_idx;
			Hierarchy& h = m_hierarchy.emplace();
			h.game_object = child;
			h.parent = INVALID_GAME_OBJECT;
			h.first_child = INVALID_GAME_OBJECT;
			h.next_sibling = INVALID_GAME_OBJECT;
		}

		if (new_parent.isValid())
		{
			int new_parent_idx = m_game_objects[new_parent.index].hierarchy;
			if (new_parent_idx < 0)
			{
				new_parent_idx = m_hierarchy.size();
				m_game_objects[new_parent.index].hierarchy = new_parent_idx;
				Hierarchy& h = m_hierarchy.emplace();
				h.game_object = new_parent;
				h.parent = INVALID_GAME_OBJECT;
				h.first_child = INVALID_GAME_OBJECT;
				h.next_sibling = INVALID_GAME_OBJECT;
			}

			m_hierarchy[child_idx].parent = new_parent;
			Transform parent_tr = getTransform(new_parent);
			Transform child_tr = getTransform(child);
			m_hierarchy[child_idx].local_transform = parent_tr.inverted() * child_tr;
			m_hierarchy[child_idx].next_sibling = m_hierarchy[new_parent_idx].first_child;
			m_hierarchy[new_parent_idx].first_child = child;
		}
		else
		{
			if (child_idx >= 0) collectGarbage(child);
		}
	}


	void ComponentManager::updateGlobalTransform(GameObject game_object)
	{
		const Hierarchy& h = m_hierarchy[m_game_objects[game_object.index].hierarchy];
		Transform parent_tr = getTransform(h.parent);

		Transform new_tr = parent_tr * h.local_transform;
		setTransform(game_object, new_tr);
	}


	void ComponentManager::setLocalPosition(GameObject game_object, const float3& pos)
	{
		int hierarchy_idx = m_game_objects[game_object.index].hierarchy;
		if (hierarchy_idx < 0)
		{
			setPosition(game_object, pos);
			return;
		}

		m_hierarchy[hierarchy_idx].local_transform.pos = pos;
		updateGlobalTransform(game_object);
	}


	void ComponentManager::setLocalRotation(GameObject game_object, const Quaternion& rot)
	{
		int hierarchy_idx = m_game_objects[game_object.index].hierarchy;
		if (hierarchy_idx < 0)
		{
			setRotation(game_object, rot);
			return;
		}
		m_hierarchy[hierarchy_idx].local_transform.rot = rot;
		updateGlobalTransform(game_object);
	}


	Transform ComponentManager::computeLocalTransform(GameObject parent, const Transform& global_transform) const
	{
		Transform parent_tr = getTransform(parent);
		return parent_tr.inverted() * global_transform;
	}


	void ComponentManager::setLocalTransform(GameObject game_object, const Transform& transform)
	{
		int hierarchy_idx = m_game_objects[game_object.index].hierarchy;
		if (hierarchy_idx < 0)
		{
			setTransform(game_object, transform);
			return;
		}

		Hierarchy& h = m_hierarchy[hierarchy_idx];
		h.local_transform = transform;
		updateGlobalTransform(game_object);
	}


	Transform ComponentManager::getLocalTransform(GameObject game_object) const
	{
		int hierarchy_idx = m_game_objects[game_object.index].hierarchy;
		if (hierarchy_idx < 0)
		{
			return getTransform(game_object);
		}

		return m_hierarchy[hierarchy_idx].local_transform;
	}


	float ComponentManager::getLocalScale(GameObject game_object) const
	{
		int hierarchy_idx = m_game_objects[game_object.index].hierarchy;
		if (hierarchy_idx < 0)
		{
			return getScale(game_object);
		}

		return m_hierarchy[hierarchy_idx].local_transform.scale;
	}



	void ComponentManager::serializeComponent(ISerializer& serializer, ComponentType type, ComponentHandle cmp)
	{
		auto* scene = m_component_type_map[type.index].scene;
		auto& method = m_component_type_map[type.index].serialize;
		(scene->*method)(serializer, cmp);
	}


	void ComponentManager::deserializeComponent(IDeserializer& serializer, GameObject game_object, ComponentType type, int scene_version)
	{
		auto* scene = m_component_type_map[type.index].scene;
		auto& method = m_component_type_map[type.index].deserialize;
		(scene->*method)(serializer, game_object, scene_version);
	}


	void ComponentManager::serialize(WriteBinary& serializer)
	{
		serializer.write((e_int32)m_game_objects.size());
		serializer.write(&m_game_objects[0], sizeof(m_game_objects[0]) * m_game_objects.size());
		serializer.write((e_int32)m_names.size());
		for (const GameObjectName& name : m_names)
		{
			serializer.write(name.game_object);
			serializer.writeString(name.name);
		}
		serializer.write(m_first_free_slot);
		serializer.write(m_hierarchy.size());

		if (!m_hierarchy.empty()) 
			serializer.write(&m_hierarchy[0], sizeof(m_hierarchy[0]) * m_hierarchy.size());
	}


	void ComponentManager::deserialize(ReadBinary& serializer)
	{
		e_int32 count;
		serializer.read(count);
		m_game_objects.resize(count);

		if (count > 0) 
			serializer.read(&m_game_objects[0], sizeof(m_game_objects[0]) * m_game_objects.size());

		serializer.read(count);
		for (int i = 0; i < count; ++i)
		{
			GameObjectName& name = m_names.emplace();
			serializer.read(name.game_object);
			serializer.readString(name.name, TlengthOf(name.name));
			m_game_objects[name.game_object.index].name = m_names.size() - 1;
		}

		serializer.read(m_first_free_slot);

		serializer.read(count);
		m_hierarchy.resize(count);

		if (count > 0) 
			serializer.read(&m_hierarchy[0], sizeof(m_hierarchy[0]) * m_hierarchy.size());
	}


	struct PrefabGameObjectGUIDMap : public ILoadGameObjectGUIDMap
	{
		explicit PrefabGameObjectGUIDMap(const TArrary<GameObject>& _entities)
			: entities(_entities)
		{
		}


		GameObject get(GameObjectGUID guid) override
		{
			if (guid.value >= entities.size()) 
				return INVALID_GAME_OBJECT;
			return entities[(int)guid.value];
		}


		const TArrary<GameObject>& entities;
	};


	/*GameObject ComponentManager::instantiatePrefab(const PrefabResource& prefab,
		const float3& pos,
		const Quaternion& rot,
		float scale)
	{
		ReadBinary blob(prefab.blob.getData(), prefab.blob.getPos());
		TVector<GameObject> entities(m_allocator);
		PrefabGameObjectGUIDMap game_object_map(entities);
		TextDeserializer deserializer(blob, game_object_map);
		e_uint32 version;
		deserializer.read(&version);
		if (version > (int)PrefabVersion::LAST)
		{
			log_error("Engine Prefab %s has unsupported version.", prefab.getPath());
			return INVALID_GAME_OBJECT;
		}
		int count;
		deserializer.read(&count);
		int game_object_idx = 0;
		entities.reserve(count);
		for (int i = 0; i < count; ++i)
		{
			entities.push_back(createGameObject({ 0, 0, 0 }, { 0, 0, 0, 1 }));
		}
		while (blob.getPosition() < blob.getSize() && game_object_idx < count)
		{
			e_uint64 prefab;
			deserializer.read(&prefab);
			GameObject game_object = entities[game_object_idx];
			setTransform(game_object, { pos, rot, scale });
			if (version > (int)PrefabVersion::WITH_HIERARCHY)
			{
				GameObject parent;

				deserializer.read(&parent);
				if (parent.isValid())
				{
					RigidTransform local_tr;
					deserializer.read(&local_tr);
					float scale;
					deserializer.read(&scale);
					setParent(parent, game_object);
					setLocalTransform(game_object, { local_tr.pos, local_tr.rot, scale });
				}
			}
			e_uint32 cmp_type_hash;
			deserializer.read(&cmp_type_hash);
			while (cmp_type_hash != 0)
			{
				ComponentType cmp_type = Reflection::getComponentTypeFromHash(cmp_type_hash);
				int scene_version;
				deserializer.read(&scene_version);
				deserializeComponent(deserializer, game_object, cmp_type, scene_version);
				deserializer.read(&cmp_type_hash);
			}
			++game_object_idx;
		}
		return entities[0];
	}*/

	void ComponentManager::setScale(GameObject game_object, float scale)
	{
		auto& transform = m_game_objects[game_object.index];
		transform.scale = scale;
		transformGameObject(game_object, true);
	}


	float ComponentManager::getScale(GameObject game_object) const
	{
		auto& transform = m_game_objects[game_object.index];
		return transform.scale;
	}


	ComponentUID ComponentManager::getFirstComponent(GameObject game_object) const
	{
		e_uint64 mask = m_game_objects[game_object.index].components;
		for (int i = 0; i < ComponentType::MAX_TYPES_COUNT; ++i)
		{
			if ((mask & (e_uint64(1) << i)) != 0)
			{
				SceneManager* scene = m_component_type_map[i].scene;
				return ComponentUID(game_object, { i }, scene, scene->getComponent(game_object, { i }));
			}
		}
		return ComponentUID::INVALID;
	}


	ComponentUID ComponentManager::getNextComponent(const ComponentUID& cmp) const
	{
		e_uint64 mask = m_game_objects[cmp.entity.index].components;
		for (int i = cmp.type.index + 1; i < ComponentType::MAX_TYPES_COUNT; ++i)
		{
			if ((mask & (e_uint64(1) << i)) != 0)
			{
				SceneManager* scene = m_component_type_map[i].scene;
				return ComponentUID(cmp.entity, { i }, scene, scene->getComponent(cmp.entity, { i }));
			}
		}
		return ComponentUID::INVALID;
	}


	ComponentUID ComponentManager::getComponent(GameObject game_object, ComponentType component_type) const
	{
		e_uint64 mask = m_game_objects[game_object.index].components;
		if ((mask & (e_uint64(1) << component_type.index)) == 0) 
			return ComponentUID::INVALID;
		SceneManager* scene = m_component_type_map[component_type.index].scene;
		return ComponentUID(game_object, component_type, scene, scene->getComponent(game_object, component_type));
	}


	bool ComponentManager::hasComponent(GameObject game_object, ComponentType component_type) const
	{
		e_uint64 mask = m_game_objects[game_object.index].components;
		return (mask & (e_uint64(1) << component_type.index)) != 0;
	}


	void ComponentManager::destroyComponent(GameObject game_object, ComponentType component_type, SceneManager* scene, ComponentHandle index)
	{
		auto mask = m_game_objects[game_object.index].components;
		auto old_mask = mask;
		mask &= ~((e_uint64)1 << component_type.index);
		ASSERT(old_mask != mask);
		m_game_objects[game_object.index].components = mask;
		m_component_destroyed.invoke(ComponentUID(game_object, component_type, scene, index));
	}


	void ComponentManager::addComponent(GameObject game_object, ComponentType component_type, SceneManager* scene, ComponentHandle index)
	{
		ComponentUID cmp(game_object, component_type, scene, index);
		m_game_objects[game_object.index].components |= (e_uint64)1 << component_type.index;
		m_component_added.invoke(cmp);
	}
}
