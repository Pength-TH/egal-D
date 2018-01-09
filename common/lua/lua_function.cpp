#include "common/lua/lua_function.h"
#include "common/egal_string.h"
#include "common/lua/lua_manager.h"
#include "common/framework/renderer.h"
#include "common/resource/shader_manager.h"

#include <lua.hpp>
#include <lauxlib.h>

/**engine*/
namespace egal
{
	struct SetPropertyVisitor : public Reflection::IPropertyVisitor
	{
		void visit(const Reflection::Property<float>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_isnumber(L, -1))
			{
				float f = (float)lua_tonumber(L, -1);
				ReadBinary input_blob(&f, sizeof(f));
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::Property<int>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_isinteger(L, -1))
			{
				int i = (int)lua_tointeger(L, -1);
				ReadBinary input_blob(&i, sizeof(i));
				prop.setValue(cmp, -1, input_blob);
			}

		}

		void visit(const Reflection::Property<GameObject>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_isinteger(L, -1))
			{
				int i = (int)lua_tointeger(L, -1);
				ReadBinary input_blob(&i, sizeof(i));
				prop.setValue(cmp, -1, input_blob);
			}

		}

		void visit(const Reflection::Property<Int2>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_istable(L, -1))
			{
				auto v = lua_manager::toType<Int2>(L, -1);
				ReadBinary input_blob(&v, sizeof(v));
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::Property<float2>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_istable(L, -1))
			{
				auto v = lua_manager::toType<float2>(L, -1);
				ReadBinary input_blob(&v, sizeof(v));
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::Property<float3>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_istable(L, -1))
			{
				auto v = lua_manager::toType<float3>(L, -1);
				ReadBinary input_blob(&v, sizeof(v));
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::Property<float4>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_istable(L, -1))
			{
				auto v = lua_manager::toType<float4>(L, -1);
				ReadBinary input_blob(&v, sizeof(v));
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::Property<ArchivePath>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_isstring(L, -1))
			{
				const char* str = lua_tostring(L, -1);
				ReadBinary input_blob(str, StringUnitl::stringLength(str) + 1);
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::Property<bool>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_isboolean(L, -1))
			{
				bool b = lua_toboolean(L, -1) != 0;
				ReadBinary input_blob(&b, sizeof(b));
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::Property<const char*>& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			if (lua_isstring(L, -1))
			{
				const char* str = lua_tostring(L, -1);
				ReadBinary input_blob(str, StringUnitl::stringLength(str) + 1);
				prop.setValue(cmp, -1, input_blob);
			}
		}

		void visit(const Reflection::IArrayProperty& prop) override
		{
			if (!StringUnitl::equalStrings(property_name, prop.name)) return;
			log_error("Lua Script Property %s has unsupported type.", prop.name);
		}

		void visit(const Reflection::IEnumProperty& prop) override { notSupported(prop); }
		void visit(const Reflection::IBlobProperty& prop) override { notSupported(prop); }
		void visit(const Reflection::ISampledFuncProperty& prop) override { notSupported(prop); }

		void notSupported(const Reflection::PropertyBase& prop)
		{
			if (!StringUnitl::equalStrings(property_name, prop.name))
				return;

			log_error("Lua Script Property %s has unsupported type.", prop.name);
		}

		lua_State* L;
		ComponentUID cmp;
		const e_char* property_name;
	};

	namespace lua
	{
		static bool LUA_hasFilesystemWork(EngineRoot* engine)
		{
			return g_file_system->hasWork();
		}

		static void LUA_processFilesystemWork(EngineRoot* engine)
		{
			g_file_system->updateAsyncTransactions();
		}

		static void LUA_startGame(EngineRoot* engine, ComponentManager* com_man)
		{
			if (engine && com_man) 
				engine->startGame(*com_man);
		}

		static ComponentHandle LUA_createComponent(ComponentManager* com_man, GameObject game_object, const char* type)
		{
			if (!com_man)
				return INVALID_COMPONENT;

			ComponentType cmp_type = Reflection::getComponentType(type);
			IScene* scene = com_man->getScene(cmp_type);

			if (!scene) return INVALID_COMPONENT;
			if (scene->getComponent(game_object, cmp_type) != INVALID_COMPONENT)
			{
				log_error("Lua Script Component %s already exists in entity %s.", type, game_object.index);
				return INVALID_COMPONENT;
			}

			return scene->createComponent(cmp_type, game_object);
		}

		static GameObject LUA_createGameObject(ComponentManager* univ)
		{
			return univ->createGameObject(float3(0, 0, 0), Quaternion(0, 0, 0, 1));
		}

		static int LUA_packageLoader(lua_State* L)
		{
			const char* module = lua_manager::toType<const char*>(L, 1);
			StaticString<MAX_PATH_LENGTH> tmp(module);
			tmp << ".lua";
			lua_getglobal(L, "g_engine");
			auto* engine = (EngineRoot*)lua_touserdata(L, -1);
			lua_pop(L, 1);
			auto* file = g_file_system->open(g_file_system->getDefaultDevice(), tmp, FS::Mode::OPEN_AND_READ);
			if (!file)
			{
				log_error("Engine Failed to open file %s", tmp);
				StaticString<MAX_PATH_LENGTH + 40> msg("Failed to open file ");
				msg << tmp;
				lua_pushstring(L, msg);
			}
			else if (luaL_loadbuffer(L, (const char*)file->getBuffer(), file->size(), tmp) != LUA_OK)
			{
				log_error("Engine Failed to load package  %s:", tmp, lua_tostring(L, -1));
			}

			if (file)
				g_file_system->close(*file);

			return 1;
		}

		static void* LUA_Allocator(void* ud, void* ptr, size_t osize, size_t nsize)
		{
			auto& allocator = *static_cast<IAllocator*>(ud);
			if (nsize == 0)
			{
				allocator.deallocate(ptr);
				return nullptr;
			}
			if (nsize > 0 && ptr == nullptr) return allocator.allocate(nsize);

			void* new_mem = allocator.allocate(nsize);
			StringUnitl::copyMemory(new_mem, ptr, Math::minimum(osize, nsize));
			allocator.deallocate(ptr);
			return new_mem;
		}

		static int LUA_getComponentType(const char* component_type)
		{
			return Reflection::getComponentType(component_type).index;
		}

		static ComponentHandle LUA_getComponent(ComponentManager* com_man, GameObject game_object, int component_type)
		{
			if (!com_man->hasComponent(game_object, { component_type })) return INVALID_COMPONENT;
			ComponentType type = { component_type };
			IScene* scene = com_man->getScene(type);
			if (scene) return scene->getComponent(game_object, type);

			ASSERT(false);
			return INVALID_COMPONENT;
		}

		static int LUA_createGameObjectEx(lua_State* L)
		{
			auto* ctx = lua_manager::checkArg<ComponentManager*>(L, 2);
			lua_manager::checkTableArg(L, 3);

			GameObject e = ctx->createGameObject(float3(0, 0, 0), Quaternion(0, 0, 0, 1));

			lua_pushvalue(L, 3);
			lua_pushnil(L);
			while (lua_next(L, -2) != 0)
			{
				const char* parameter_name = luaL_checkstring(L, -2);
				if (StringUnitl::equalStrings(parameter_name, "position"))
				{
					auto pos = lua_manager::toType<float3>(L, -1);
					ctx->setPosition(e, pos);
				}
				else if (StringUnitl::equalStrings(parameter_name, "rotation"))
				{
					auto rot = lua_manager::toType<Quaternion>(L, -1);
					ctx->setRotation(e, rot);
				}
				else
				{
					ComponentType cmp_type = Reflection::getComponentType(parameter_name);
					IScene* scene = ctx->getScene(cmp_type);
					if (scene)
					{
						ComponentUID cmp(e, cmp_type, scene, scene->createComponent(cmp_type, e));
						const Reflection::ComponentBase* cmp_des = Reflection::getComponent(cmp_type);
						if (cmp.isValid())
						{
							lua_pushvalue(L, -1);
							lua_pushnil(L);
							while (lua_next(L, -2) != 0)
							{
								const char* property_name = luaL_checkstring(L, -2);
								SetPropertyVisitor v;
								v.property_name = property_name;
								v.cmp = cmp;
								v.L = L;
								cmp_des->visit(v);

								lua_pop(L, 1);
							}
							lua_pop(L, 1);
						}
					}
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			lua_manager::push(L, e);
			return 1;
		}

		static int LUA_setGameObjectRotation(lua_State* L)
		{
			ComponentManager* univ = lua_manager::checkArg<ComponentManager*>(L, 1);
			int entity_index = lua_manager::checkArg<int>(L, 2);
			if (entity_index < 0) return 0;

			if (lua_gettop(L) > 3)
			{
				float3 axis = lua_manager::checkArg<float3>(L, 3);
				float angle = lua_manager::checkArg<float>(L, 4);
				univ->setRotation({ entity_index }, Quaternion(axis, angle));
			}
			else
			{
				Quaternion rot = lua_manager::checkArg<Quaternion>(L, 3);
				univ->setRotation({ entity_index }, rot);
			}
			return 0;
		}

		static IScene* LUA_getScene(ComponentManager* com_man, const char* name)
		{
			e_uint32 hash = crc32(name);
			return com_man->getScene(hash);
		}

		static int LUA_loadResource(EngineImpl* engine, const char* path, const char* type)
		{
			ResourceManagerBase* res_manager = engine->getResourceManager().get(ResourceType(type));
			if (!res_manager) return -1;
			Resource* res = res_manager->load(ArchivePath(path));
			++engine->m_last_lua_resource_idx;
			engine->m_lua_resources.insert(engine->m_last_lua_resource_idx, res);
			return engine->m_last_lua_resource_idx;
		}

		static void LUA_setGameObjectLocalRotation(ComponentManager* com_man, GameObject entity, const Quaternion& rotation)
		{
			if (!entity.isValid()) return;
			if (!com_man->getParent(entity).isValid()) return;

			com_man->setLocalRotation(entity, rotation);
		}

		static void LUA_setGameObjectLocalPosition(ComponentManager* com_man, GameObject entity, const float3& position)
		{
			if (!entity.isValid()) return;
			if (!com_man->getParent(entity).isValid()) return;

			com_man->setLocalPosition(entity, position);
		}

		static ComponentManager* LUA_createComponentManager(EngineImpl* engine) 
		{ 
			ComponentManager* com_man = &engine->createComponentManager(false);
		
			if (set_lua_globals)
			{
				for (auto* scene : com_man->getScenes())
				{
					const char* name = scene->getPlugin().getName();
					char tmp[128];

					StringUnitl::copyString(tmp, "g_scene_");
					StringUnitl::catString(tmp, name);
					lua_pushlightuserdata(m_state, scene);
					lua_setglobal(m_state, tmp);
				}
				lua_pushlightuserdata(m_state, com_man);
				lua_setglobal(m_state, "g_com_man");
			}
			return com_man;
		}

		static void LUA_setGameObjectPosition(ComponentManager* univ, GameObject entity, float3 pos) { univ->setPosition(entity, pos); }
		static void LUA_unloadResource(EngineImpl* engine, int resource_idx) { engine->unloadLuaResource(resource_idx); }
		
		static void LUA_destroyComponentManager(EngineImpl* engine, ComponentManager* com_man) { engine->destroyComponentManager(*com_man); }
		static ComponentManager* LUA_getSceneComponentManager(IScene* scene) { return &scene->getComponentManager(); }
		static void LUA_logError(const char* text) { log_error("Lua Script %s", text); }
		static void LUA_logInfo(const char* text) { log_info("Lua Script %s.", text); }
		static void LUA_pause(EngineRoot* engine, bool pause) { engine->pause(pause); }
		static void LUA_nextFrame(EngineRoot* engine) { engine->nextFrame(); }
		static void LUA_setTimeMultiplier(EngineRoot* engine, float multiplier) { engine->setTimeMultiplier(multiplier); }
		static GameObject LUA_getFirstGameObject(ComponentManager* com_man) { return com_man->getFirstGameObject(); }
		static GameObject LUA_getNextGameObject(ComponentManager* com_man, GameObject entity) { return com_man->getNextGameObject(entity); }
		static float4 LUA_multMatrixVec(const Matrix& m, const float4& v) { return m * v; }
		static Quaternion LUA_multQuaternion(const Quaternion& a, const Quaternion& b) { return a * b; }

		static int LUA_loadComponentManager(lua_State* L)
		{
			auto* engine = lua_manager::checkArg<EngineRoot*>(L, 1);
			auto* com_man = lua_manager::checkArg<ComponentManager*>(L, 2);
			auto* path = lua_manager::checkArg<const char*>(L, 3);
			if (!lua_isfunction(L, 4)) lua_manager::argError(L, 4, "function");
			FS::FileSystem& fs = engine->getFileSystem();
			FS::ReadCallback cb;
			struct Callback
			{
				~Callback()
				{
					luaL_unref(L, LUA_REGISTRYINDEX, lua_func);
				}

				void invoke(FS::IFile& file, bool success)
				{
					if (!success)
					{
						log_error("Engine Failed to open com_man %s.", path);
					}
					else
					{
						ReadBinary blob(file.getBuffer(), (int)file.size());
#pragma pack(1)
						struct Header
						{
							e_uint32 magic;
							int version;
							e_uint32 hash;
							e_uint32 engine_hash;
						};
#pragma pack()
						Header header;
						blob.read(&header, sizeof(header));

						if (!engine->deserialize(*com_man, blob))
						{
							log_error("Engine Failed to deserialize com_man %s.", path);
						}
						else
						{
							if (lua_rawgeti(L, LUA_REGISTRYINDEX, lua_func) != LUA_TFUNCTION)
							{
								ASSERT(false);
							}

							if (lua_pcall(L, 0, 0, 0) != LUA_OK)
							{
								log_error("Engine %s", lua_tostring(L, -1));
								lua_pop(L, 1);
							}
						}
					}
					_delete(engine->getAllocator(), this);
				}

				EngineRoot* engine;
				ComponentManager* com_man;
				ArchivePath path;
				lua_State* L;
				int lua_func;
			};
			Callback* inst = _aligned_new(engine->getAllocator(), Callback);
			inst->engine = engine;
			inst->com_man = com_man;
			inst->path = path;
			inst->L = L;
			inst->lua_func = luaL_ref(L, LUA_REGISTRYINDEX);
			cb.bind<Callback, &Callback::invoke>(inst);
			fs.openAsync(fs.getDefaultDevice(), inst->path.c_str(), FS::Mode::OPEN_AND_READ, cb);
			return 0;
		}

		static int LUA_multVecQuaternion(lua_State* L)
		{
			float3 v = lua_manager::checkArg<float3>(L, 1);
			Quaternion q;
			if (lua_manager::isType<Quaternion>(L, 2))
			{
				q = lua_manager::checkArg<Quaternion>(L, 2);
			}
			else
			{
				float3 axis = lua_manager::checkArg<float3>(L, 2);
				float angle = lua_manager::checkArg<float>(L, 3);

				q = Quaternion(axis, angle);
			}

			float3 res = q.rotate(v);

			lua_manager::push(L, res);
			return 1;
		}

		static float3 LUA_getGameObjectPosition(ComponentManager* com_man, GameObject entity)
		{
			if (!entity.isValid())
			{
				log_waring("Engine Requesting position on invalid entity");
				return float3(0, 0, 0);
			}
			return com_man->getPosition(entity);
		}

		static GameObject LUA_getGameObjectByName(ComponentManager* com_man, const char* name)
		{
			return com_man->getGameObjectByName(name);
		}

		static Quaternion LUA_getGameObjectRotation(ComponentManager* com_man, GameObject entity)
		{
			if (!entity.isValid())
			{
				log_waring("Engine Requesting rotation on invalid entity");
				return Quaternion(0, 0, 0, 1);
			}
			return com_man->getRotation(entity);
		}

		static void LUA_destroyGameObject(ComponentManager* com_man, GameObject entity)
		{
			com_man->destroyGameObject(entity);
		}

		static float3 LUA_getGameObjectDirection(ComponentManager* com_man, GameObject entity)
		{
			Quaternion rot = com_man->getRotation(entity);
			return rot.rotate(float3(0, 0, 1));
		}

		static int LUA_instantiatePrefab(lua_State* L)
		{
			auto* engine = lua_manager::checkArg<EngineImpl*>(L, 1);
			auto* com_man = lua_manager::checkArg<ComponentManager*>(L, 2);
			float3 position = lua_manager::checkArg<float3>(L, 3);
			int prefab_id = lua_manager::checkArg<int>(L, 4);
			//PrefabResource* prefab = static_cast<PrefabResource*>(engine->getLuaResource(prefab_id));
			/*if (!prefab)
			{
			log_error("Editor Cannot instantiate null prefab.");
			return 0;
			}
			if (!prefab->isReady())
			{
			log_error("Editor Prefab %s is not ready, preload it.", prefab->getArchivePath().c_str());
			return 0;
			}
			GameObject entity = com_man->instantiatePrefab(*prefab, position, {0, 0, 0, 1}, 1);
			*/
			//lua_manager::push(L, entity);
			return 1;
		}

/********************************************************************************************************************/
/**register lua**/
/********************************************************************************************************************/

		void registerLuaAPI()
		{
			lua_pushlightuserdata(m_state, this);
			lua_setglobal(m_state, "g_engine");

#define REGISTER_FUNCTION(name) \
				lua_manager::createSystemFunction(m_state, "Engine", #name, \
					&lua_manager::wrap<decltype(&LUA_##name), LUA_##name>); \

			REGISTER_FUNCTION(createComponent);
			REGISTER_FUNCTION(createGameObject);
			REGISTER_FUNCTION(createComponentManager);
			REGISTER_FUNCTION(destroyGameObject);
			REGISTER_FUNCTION(destroyComponentManager);
			REGISTER_FUNCTION(getComponent);
			REGISTER_FUNCTION(getComponentType);
			REGISTER_FUNCTION(getGameObjectDirection);
			REGISTER_FUNCTION(getGameObjectPosition);
			REGISTER_FUNCTION(getGameObjectRotation);
			REGISTER_FUNCTION(getGameObjectByName);
			REGISTER_FUNCTION(getFirstGameObject);
			REGISTER_FUNCTION(getNextGameObject);
			REGISTER_FUNCTION(getScene);
			REGISTER_FUNCTION(getSceneComponentManager);
			REGISTER_FUNCTION(hasFilesystemWork);
			REGISTER_FUNCTION(loadResource);
			REGISTER_FUNCTION(logError);
			REGISTER_FUNCTION(logInfo);
			REGISTER_FUNCTION(multMatrixVec);
			REGISTER_FUNCTION(multQuaternion);
			REGISTER_FUNCTION(nextFrame);
			REGISTER_FUNCTION(pause);
			REGISTER_FUNCTION(processFilesystemWork);
			REGISTER_FUNCTION(setGameObjectLocalPosition);
			REGISTER_FUNCTION(setGameObjectLocalRotation);
			REGISTER_FUNCTION(setGameObjectPosition);
			REGISTER_FUNCTION(setGameObjectRotation);
			REGISTER_FUNCTION(setTimeMultiplier);
			REGISTER_FUNCTION(startGame);
			REGISTER_FUNCTION(unloadResource);

			lua_manager::createSystemFunction(m_state, "Engine", "loadComponentManager", LUA_loadComponentManager);

#undef REGISTER_FUNCTION

			lua_manager::createSystemFunction(m_state, "Engine", "instantiatePrefab", &LUA_instantiatePrefab);
			lua_manager::createSystemFunction(m_state, "Engine", "createGameObjectEx", &LUA_createGameObjectEx);
			lua_manager::createSystemFunction(m_state, "Engine", "multVecQuaternion", &LUA_multVecQuaternion);

			lua_manager::createSystemVariable(m_state, "Engine", "INPUT_DEVICE_KEYBOARD", InputSystem::Device::KEYBOARD);
			lua_manager::createSystemVariable(m_state, "Engine", "INPUT_DEVICE_MOUSE", InputSystem::Device::MOUSE);
			lua_manager::createSystemVariable(m_state, "Engine", "INPUT_DEVICE_CONTROLLER", InputSystem::Device::CONTROLLER);

			lua_manager::createSystemVariable(m_state, "Engine", "INPUT_EVENT_BUTTON", InputSystem::Event::BUTTON);
			lua_manager::createSystemVariable(m_state, "Engine", "INPUT_EVENT_AXIS", InputSystem::Event::AXIS);
			lua_manager::createSystemVariable(m_state, "Engine", "INPUT_EVENT_DEVICE_ADDED", InputSystem::Event::DEVICE_ADDED);
			lua_manager::createSystemVariable(m_state, "Engine", "INPUT_EVENT_DEVICE_REMOVED", InputSystem::Event::DEVICE_REMOVED);

			lua_pop(m_state, 1);

			installLuaPackageLoader();
		}

		void installLuaPackageLoader() const
		{
			if (lua_getglobal(m_state, "package") != LUA_TTABLE)
			{
				log_error("Engine Lua \"package\" is not a table");
				return;
			}
			if (lua_getfield(m_state, -1, "searchers") != LUA_TTABLE)
			{
				log_error("Engine Lua \"package.searchers\" is not a table");
				return;
			}
			int numLoaders = 0;
			lua_pushnil(m_state);
			while (lua_next(m_state, -2) != 0)
			{
				lua_pop(m_state, 1);
				numLoaders++;
			}

			lua_pushinteger(m_state, numLoaders + 1);
			lua_pushcfunction(m_state, LUA_packageLoader);
			lua_rawset(m_state, -3);
			lua_pop(m_state, 2);
		}




	}
}

/***/
namespace egal
{
	namespace lua
	{

		static e_uint32 LUA_getTexturePixel(Texture* texture, e_int32 x, e_int32 y)
		{
			if (!texture) return 0;
			if (!texture->isReady()) return 0;
			if (texture->data.empty()) return 0;
			if (texture->bytes_per_pixel != 4) return 0;

			x = Math::clamp(x, 0, texture->width - 1);
			y = Math::clamp(y, 0, texture->height - 1);

			return ((e_uint32*)&texture->data[0])[x + y * texture->width];
		}


		static Pipeline* LUA_createPipeline(EngineRoot* engine, const e_char* path)
		{
			//Renderer& renderer = *static_cast<Renderer*>(engine->getPluginManager().getPlugin("renderer"));
			//Pipeline* pipeline = Pipeline::create(renderer, ArchivePath(path), "", renderer.getEngine().getAllocator());
			//pipeline->load();
			//return pipeline;
			return nullptr;
		}


		static e_void LUA_destroyPipeline(Pipeline* pipeline)
		{
			Pipeline::destroy(pipeline);
		}


		static e_void LUA_setPipelineScene(Pipeline* pipeline, RenderScene* scene)
		{
			pipeline->setScene(scene);
		}


		static RenderScene* LUA_getPipelineScene(Pipeline* pipeline)
		{
			return pipeline->getScene();
		}


		static e_void LUA_pipelineRender(Pipeline* pipeline, e_int32 w, e_int32 h)
		{
			pipeline->resize(w, h);
			pipeline->render();
		}


		static bgfx::TextureHandle* LUA_getRenderBuffer(Pipeline* pipeline,
			const e_char* framebuffer_name,
			e_int32 renderbuffer_idx)
		{
			bgfx::TextureHandle& handle = pipeline->getRenderbuffer(framebuffer_name, renderbuffer_idx);
			return &handle;
		}


		static Texture* LUA_getMaterialTexture(Material* material, e_int32 texture_index)
		{
			if (!material) return nullptr;
			return material->getTexture(texture_index);
		}


		static e_void LUA_setEntityInstancePath(IScene* scene, e_int32 component, const e_char* path)
		{
			RenderScene* render_scene = (RenderScene*)scene;
			render_scene->setEntityInstancePath({ component }, ArchivePath(path));
		}


		static e_int32 LUA_getEntityBoneIndex(Entity* entity, const e_char* bone)
		{
			if (!entity) return 0;
			return entity->getBoneIndex(crc32(bone)).value();
		}


		static e_uint32 LUA_compareTGA(RenderSceneImpl* scene, const e_char* path, const e_char* path_preimage, e_int32 min_diff)
		{
			auto& fs = *g_file_system;
			auto file1 = fs.open(fs.getDefaultDevice(), path, FS::Mode::OPEN_AND_READ);
			auto file2 = fs.open(fs.getDefaultDevice(), path_preimage, FS::Mode::OPEN_AND_READ);
			if (!file1)
			{
				if (file2) fs.close(*file2);
				log_error("render_test Failed to open %s.", path);
				return 0xffffFFFF;
			}
			else if (!file2)
			{
				fs.close(*file1);
				log_error("render_test Failed to open %s.", path_preimage);
				return 0xffffFFFF;
			}
			e_uint32 result = Texture::compareTGA(file1, file2, min_diff, scene->m_allocator);
			fs.close(*file1);
			fs.close(*file2);
			return result;
		}


		static e_void LUA_makeScreenshot(RenderSceneImpl* scene, const e_char* path)
		{
			scene->m_renderer.makeScreenshot(ArchivePath(path));
		}


		static e_void LUA_setEntityInstanceMaterial(RenderScene* scene,
			ComponentHandle cmp,
			e_int32 index,
			const e_char* path)
		{
			scene->setEntityInstanceMaterial(cmp, index, ArchivePath(path));
		}


		namespace LuaAPI
		{
			e_int32 setViewMode(lua_State* L)
			{
				PipelineImpl* that = lua_manager::checkArg<PipelineImpl*>(L, 1);
				auto mode = (bgfx::ViewMode::Enum)lua_manager::checkArg<e_int32>(L, 2);
				if (!that->m_current_view) return 0;
				bgfx::setViewMode(that->m_current_view->bgfx_id, mode);
				return 0;
			}


			e_int32 bindFramebufferTexture(lua_State* L)
			{
				PipelineImpl* that = lua_manager::checkArg<PipelineImpl*>(L, 1);
				if (!that->m_current_view) return 0;
				const e_char* framebuffer_name = lua_manager::checkArg<const e_char*>(L, 2);
				e_int32 renderbuffer_idx = lua_manager::checkArg<e_int32>(L, 3);
				e_int32 uniform_idx = lua_manager::checkArg<e_int32>(L, 4);
				e_uint32 flags = lua_gettop(L) > 4 ? lua_manager::checkArg<e_uint32>(L, 5) : 0xffffFFFF;

				FrameBuffer* fb = that->getFramebuffer(framebuffer_name);
				if (!fb) return 0;

				float4 size;
				size.x = (e_float)fb->getWidth();
				size.y = (e_float)fb->getHeight();
				size.z = 1.0f / (e_float)fb->getWidth();
				size.w = 1.0f / (e_float)fb->getHeight();
				that->m_current_view->command_buffer.beginAppend();
				if (that->m_global_textures_count == 0)
				{
					that->m_current_view->command_buffer.setUniform(that->m_texture_size_uniform, size);
				}
				that->m_current_view->command_buffer.setTexture(15 - that->m_global_textures_count,
					that->m_uniforms[uniform_idx],
					fb->getRenderbufferHandle(renderbuffer_idx),
					flags);
				++that->m_global_textures_count;
				that->m_current_view->command_buffer.end();
				return 0;
			}



			e_int32 newView(lua_State* L)
			{
				auto* pipeline = lua_manager::checkArg<PipelineImpl*>(L, 1);
				const e_char* debug_name = lua_manager::checkArg<const e_char*>(L, 2);
				const e_char* framebuffer_name = lua_manager::checkArg<const e_char*>(L, 3);
				e_uint64 layer_mask = 0;
				if (lua_gettop(L) > 3) layer_mask = lua_manager::checkArg<e_uint64>(L, 4);

				pipeline->m_layer_mask |= layer_mask;

				lua_manager::push(L, pipeline->newView(debug_name, layer_mask));
				pipeline->setFramebuffer(framebuffer_name);
				return 1;
			}


			e_int32 addFramebuffer(lua_State* L)
			{
				auto* pipeline = lua_manager::checkArg<PipelineImpl*>(L, 1);
				const e_char* name = lua_manager::checkArg<const e_char*>(L, 2);
				FrameBuffer* framebuffer = pipeline->getFramebuffer(name);
				if (framebuffer)
				{
					log_error("Renderer Trying to create already existing framebuffer %S.", name);
					return 0;
				}

				lua_manager::checkTableArg(L, 3);
				FrameBuffer::Declaration decl;
				StringUnitl::copyString(decl.m_name, name);

				lua_manager::getOptionalField(L, 3, "width", &decl.m_width);
				decl.m_size_ratio = float2(-1, -1);
				if (lua_getfield(L, 3, "size_ratio") == LUA_TTABLE)
				{
					decl.m_size_ratio = lua_manager::toType<float2>(L, -1);
				}
				lua_pop(L, 1);
				if (lua_getfield(L, 3, "screen_size") == LUA_TBOOLEAN)
				{
					e_bool is_screen_size = lua_toboolean(L, -1) != 0;
					decl.m_size_ratio = is_screen_size ? float2(1, 1) : float2(-1, -1);
				}
				lua_pop(L, 1);
				lua_manager::getOptionalField(L, 3, "height", &decl.m_height);

				if (lua_getfield(L, 3, "renderbuffers") == LUA_TTABLE)
				{
					PipelineImpl::parseRenderbuffers(L, decl, pipeline);
				}
				lua_pop(L, 1);
				if ((decl.m_size_ratio.x > 0 || decl.m_size_ratio.y > 0) && pipeline->m_height > 0)
				{
					decl.m_width = e_int32(pipeline->m_width * decl.m_size_ratio.x);
					decl.m_height = e_int32(pipeline->m_height * decl.m_size_ratio.y);
				}
				auto* fb = _aligned_new(pipeline->m_allocator, FrameBuffer)(decl);
				pipeline->m_framebuffers.push(fb);
				if (StringUnitl::equalStrings(decl.m_name, "default")) pipeline->m_default_framebuffer = fb;

				return 0;
			}


			e_int32 renderModels(lua_State* L)
			{
				auto* pipeline = lua_manager::checkArg<PipelineImpl*>(L, 1);

				ComponentHandle cam = pipeline->m_applied_camera;

				pipeline->renderAll(pipeline->m_camera_frustum, true, cam, pipeline->m_layer_mask);
				pipeline->m_layer_mask = 0;
				return 0;
			}


			e_void logError(const e_char* message)
			{
				log_error("Renderer %s.", message);
			}


			e_int32 setUniform(lua_State* L)
			{
				auto* pipeline = lua_manager::checkArg<PipelineImpl*>(L, 1);
				if (!pipeline->m_current_view) return 0;

				e_int32 uniform_idx = lua_manager::checkArg<e_int32>(L, 2);
				lua_manager::checkTableArg(L, 3);

				float4 tmp[64];
				e_int32 len = Math::minimum((e_int32)lua_rawlen(L, 3), TlengthOf(tmp));
				for (e_int32 i = 0; i < len; ++i)
				{
					if (lua_rawgeti(L, 3, 1 + i) == LUA_TTABLE)
					{
						if (lua_rawgeti(L, -1, 1) == LUA_TNUMBER) tmp[i].x = (e_float)lua_tonumber(L, -1);
						if (lua_rawgeti(L, -2, 2) == LUA_TNUMBER) tmp[i].y = (e_float)lua_tonumber(L, -1);
						if (lua_rawgeti(L, -3, 3) == LUA_TNUMBER) tmp[i].z = (e_float)lua_tonumber(L, -1);
						if (lua_rawgeti(L, -4, 4) == LUA_TNUMBER) tmp[i].w = (e_float)lua_tonumber(L, -1);
						lua_pop(L, 4);
					}
					lua_pop(L, 1);
				}

				if (uniform_idx >= pipeline->m_uniforms.size()) luaL_argerror(L, 2, "unknown uniform");

				pipeline->m_current_view->command_buffer.beginAppend();
				pipeline->m_current_view->command_buffer.setUniform(pipeline->m_uniforms[uniform_idx], tmp, len);
				pipeline->m_current_view->command_buffer.end();
				return 0;
			}


			e_int32 getRenderbuffer(lua_State* L)
			{
				auto* pipeline = lua_manager::checkArg<PipelineImpl*>(L, 1);
				const e_char* fb_name = lua_manager::checkArg<const e_char*>(L, 2);
				e_int32 rb_idx = lua_manager::checkArg<e_int32>(L, 3);

				e_void* rb = &pipeline->getRenderbuffer(fb_name, rb_idx);
				lua_manager::push(L, rb);
				return 1;
			}


			e_int32 renderLocalLightsShadowmaps(lua_State* L)
			{
				auto* pipeline = lua_manager::checkArg<PipelineImpl*>(L, 1);
				const e_char* camera_slot = lua_manager::checkArg<const e_char*>(L, 2);

				FrameBuffer* fbs[16];
				e_int32 len = Math::minimum((e_int32)lua_rawlen(L, 3), TlengthOf(fbs));
				for (e_int32 i = 0; i < len; ++i)
				{
					if (lua_rawgeti(L, 3, 1 + i) == LUA_TSTRING)
					{
						const e_char* fb_name = lua_tostring(L, -1);
						fbs[i] = pipeline->getFramebuffer(fb_name);
					}
					lua_pop(L, 1);
				}

				SceneManager* scene = pipeline->m_scene;
				ComponentHandle camera = scene->getCameraInSlot(camera_slot);
				pipeline->renderLocalLightShadowmaps(camera, fbs, len);

				return 0;
			}


			e_void print(e_int32 x, e_int32 y, const e_char* text)
			{
				bgfx::dbgTextPrintf(x, y, 0x4f, text);
			}
		}

		static e_void parseRenderbuffers(lua_State* L, FrameBuffer::Declaration& decl, PipelineImpl* pipeline)
		{
			decl.m_renderbuffers_count = 0;
			e_int32 len = (e_int32)lua_rawlen(L, -1);
			for (e_int32 i = 0; i < len; ++i)
			{
				if (lua_rawgeti(L, -1, 1 + i) == LUA_TTABLE)
				{
					FrameBuffer::RenderBuffer& buf = decl.m_renderbuffers[decl.m_renderbuffers_count];
					e_bool is_shared = lua_getfield(L, -1, "shared") == LUA_TTABLE;
					if (is_shared)
					{
						StaticString<64> fb_name;
						e_int32 rb_idx;
						if (lua_getfield(L, -1, "fb") == LUA_TSTRING)
						{
							fb_name = lua_tostring(L, -1);
						}
						lua_pop(L, 1);
						if (lua_getfield(L, -1, "rb") == LUA_TNUMBER)
						{
							rb_idx = (e_int32)lua_tonumber(L, -1);
						}
						lua_pop(L, 1);
						FrameBuffer* shared_fb = pipeline->getFramebuffer(fb_name);
						if (!shared_fb || rb_idx >= shared_fb->getRenderbuffersCounts())
						{
							buf.m_format = bgfx::TextureFormat::RGBA8;
							log_error("Renderer Can not share render buffer from %s , it does not exist.", fb_name);
						}
						else
						{
							buf.m_shared = &shared_fb->getRenderbuffer(rb_idx);
						}
					}
					lua_pop(L, 1);
					if (!is_shared) buf.parse(L);
					++decl.m_renderbuffers_count;
				}
				lua_pop(L, 1);
			}
		}
		e_void setDefine()
		{
			if (m_define.length() == 0) return;
			StaticString<256> tmp(m_define.c_str(), " = true");

			e_bool errors = luaL_loadbuffer(m_lua_state, tmp, StringUnitl::stringLength(tmp.data), m_path.c_str()) != LUA_OK;
			if (errors)
			{
				log_error("Renderer %s : %s", m_path.c_str(), lua_tostring(m_lua_state, -1));
				lua_pop(m_lua_state, 1);
				return;
			}

			lua_rawgeti(m_lua_state, LUA_REGISTRYINDEX, m_lua_env);
			lua_setupvalue(m_lua_state, -2, 1);
			errors = lua_pcall(m_lua_state, 0, 0, 0) != LUA_OK;
			if (errors)
			{
				log_error("Renderer %s:%s", m_path.c_str(), lua_tostring(m_lua_state, -1));
				lua_pop(m_lua_state, 1);
			}
		}

		e_void callLuaFunction(const e_char* function) override
		{
			if (!m_lua_state) return;

			lua_rawgeti(m_lua_state, LUA_REGISTRYINDEX, m_lua_env);
			if (lua_getfield(m_lua_state, -1, function) != LUA_TFUNCTION)
			{
				lua_pop(m_lua_state, 2);
				return;
			}

			if (lua_pcall(m_lua_state, 0, 0, 0) != LUA_OK)
			{
				log_error("Renderer %s.", lua_tostring(m_lua_state, -1));
				lua_pop(m_lua_state, 1);
			}
			lua_pop(m_lua_state, 1);
		}
		Renderer()
		{
			lua_rawgeti(m_lua_state, LUA_REGISTRYINDEX, m_lua_env);
			e_bool success = true;
			if (lua_getfield(m_lua_state, -1, "render") == LUA_TFUNCTION)
			{
				lua_pushlightuserdata(m_lua_state, this);
				if (lua_pcall(m_lua_state, 1, 0, 0) != LUA_OK)
				{
					success = false;
					log_error("Renderer %s.", lua_tostring(m_lua_state, -1));
					lua_pop(m_lua_state, 1);
				}
			}
			else
			{
				lua_pop(m_lua_state, 1);
			}
		
		}

		e_void exposeCustomCommandToLua(const CustomCommandHandler& handler)
		{
			if (!m_lua_state) return;

			e_char tmp[1024];
			StringUnitl::copyString(tmp, "function ");
			StringUnitl::catString(tmp, handler.name);
			StringUnitl::catString(tmp, "(pipeline) executeCustomCommand(pipeline, \"");
			StringUnitl::catString(tmp, handler.name);
			StringUnitl::catString(tmp, "\") end");

			e_bool errors = luaL_loadbuffer(m_lua_state, tmp, StringUnitl::stringLength(tmp), nullptr) != LUA_OK;
			errors = errors || lua_pcall(m_lua_state, 0, 0, 0) != LUA_OK;

			if (errors)
			{
				log_error("Renderer %s.", lua_tostring(m_lua_state, -1));
				lua_pop(m_lua_state, 1);
			}
		}

		e_void callInitScene()
		{
			lua_rawgeti(m_lua_state, LUA_REGISTRYINDEX, m_lua_env);
			if (lua_getfield(m_lua_state, -1, "initScene") == LUA_TFUNCTION)
			{
				lua_pushlightuserdata(m_lua_state, this);
				if (lua_pcall(m_lua_state, 1, 0, 0) != LUA_OK)
				{
					log_error("lua %s.", lua_tostring(m_lua_state, -1));
					lua_pop(m_lua_state, 1);
				}
			}
			else
			{
				lua_pop(m_lua_state, 1);
			}
		}

		e_void cleanup()
		{
			if (m_lua_state)
			{
				luaL_unref(m_renderer.getEngine().getState(), LUA_REGISTRYINDEX, m_lua_thread_ref);
				luaL_unref(m_lua_state, LUA_REGISTRYINDEX, m_lua_env);
				m_lua_state = nullptr;
			}
		}
	}


	static e_int32 LUA_castCameraRay(lua_State* L)
	{
		auto* scene = lua_manager::checkArg<RenderSceneImpl*>(L, 1);
		const e_char* slot = lua_manager::checkArg<const e_char*>(L, 2);
		e_float x, y;
		ComponentHandle camera_cmp = scene->getCameraInSlot(slot);
		if (!camera_cmp.isValid())
			return 0;
		if (lua_gettop(L) > 3)
		{
			x = lua_manager::checkArg<e_float>(L, 3);
			y = lua_manager::checkArg<e_float>(L, 4);
		}
		else
		{
			x = scene->getCameraScreenWidth(camera_cmp) * 0.5f;
			y = scene->getCameraScreenHeight(camera_cmp) * 0.5f;
		}

		float3 origin, dir;
		scene->getRay(camera_cmp, { x, y }, origin, dir);

		RayCastEntityHit hit = scene->castRay(origin, dir, INVALID_COMPONENT);
		lua_manager::push(L, hit.m_is_hit);
		lua_manager::push(L, hit.m_is_hit ? hit.m_origin + hit.m_dir * hit.m_t : float3(0, 0, 0));

		return 2;
	}


	static bgfx::TextureHandle* LUA_getTextureHandle(SceneManager* scene, e_int32 resource_idx)
	{
		Resource* res = scene->getEngine().getLuaResource(resource_idx);
		if (!res) return nullptr;
		return &static_cast<Texture*>(res)->handle;
	}


	static e_void LUA_setTexturePixel(Texture* texture, e_int32 x, e_int32 y, e_uint32 value)
	{
		if (!texture) return;
		if (!texture->isReady()) return;
		if (texture->data.empty()) return;
		if (texture->bytes_per_pixel != 4) return;

		x = Math::clamp(x, 0, texture->width - 1);
		y = Math::clamp(y, 0, texture->height - 1);

		((e_uint32*)&texture->data[0])[x + y * texture->width] = value;
	}


	static e_void LUA_updateTextureData(Texture* texture, e_int32 x, e_int32 y, e_int32 w, e_int32 h)
	{
		if (!texture) return;
		if (!texture->isReady()) return;
		if (texture->data.empty()) return;

		texture->onDataUpdated(x, y, w, h);
	}


	static e_int32 LUA_getTextureWidth(Texture* texture)
	{
		if (!texture) return 0;
		if (!texture->isReady()) return 0;

		return texture->width;
	}


	static e_int32 LUA_getTextureHeight(Texture* texture)
	{
		if (!texture) return 0;
		if (!texture->isReady()) return 0;

		return texture->height;
	}


	static e_float LUA_getTerrainHeightAt(RenderSceneImpl* render_scene, ComponentHandle cmp, e_int32 x, e_int32 z)
	{
		return 0;
		//return render_scene->m_terrains[{cmp.index}]->getHeight(x, z);
	}


	//static e_void LUA_emitParticle(RenderSceneImpl* render_scene, ComponentHandle emitter)
	//{
	//	e_int32 idx = render_scene->m_particle_emitters.find({ emitter.index });
	//	if (idx < 0) return;
	//	return render_scene->m_particle_emitters.at(idx)->emit();
	//}
}


namespace egal
{
	namespace lua
	{
		void LuaManager::init()
		{
			m_state = lua_newstate(LUA_Allocator, &m_allocator);
			luaL_openlibs(m_state);
			registerLuaAPI();
		}

		void LuaManager::uninit()
		{
			lua_close(m_state);
		}

		void LuaManager::runScript(const char* src, int src_length, const char* path)
		{
			if (luaL_loadbuffer(m_state, src, src_length, path) != LUA_OK)
			{
				log_error("Engine %s:%s.", path, lua_tostring(m_state, -1));
				lua_pop(m_state, 1);
			}

			if (lua_pcall(m_state, 0, 0, 0) != LUA_OK)
			{
				log_error("Engine %s:%s.", path, lua_tostring(m_state, -1));
				lua_pop(m_state, 1);
			}
		}

	}
}



namespace egal
{
	static Shader* getShader(lua_State* L)
	{
		Shader* ret = nullptr;
		if (lua_getglobal(L, "shader") == LUA_TLIGHTUSERDATA)
		{
			ret = lua_manager::toType<Shader*>(L, -1);
		}
		lua_pop(L, 1);
		return ret;
	}

	static e_void texture_slot(lua_State* state, const e_char* name, const e_char* uniform)
	{
		auto* shader = getShader(state);
		if (!shader) return;
		auto& slot = shader->m_texture_slots[shader->m_texture_slot_count];
		StringUnitl::copyString(slot.name, name);
		slot.uniform_handle = bgfx::createUniform(uniform, bgfx::UniformType::Int1);
		StringUnitl::copyString(slot.uniform, uniform);
		++shader->m_texture_slot_count;
	}

	static e_int32 indexOf(ShaderCombinations& combination, e_uint8 define_idx)
	{
		for (e_int32 i = 0; i < combination.define_count; ++i)
		{
			if (combination.defines[i] == define_idx)
			{
				return i;
			}
		}

		combination.defines[combination.define_count] = define_idx;
		++combination.define_count;
		return combination.define_count - 1;
	}

	static e_void alpha_blending(lua_State* L, const e_char* mode)
	{
		Shader* shader = getShader(L);
		if (!shader) return;
		if (StringUnitl::equalStrings(mode, "add"))
		{
			shader->m_render_states |= BGFX_STATE_BLEND_ADD;
		}
		else if (StringUnitl::equalStrings(mode, "alpha"))
		{
			shader->m_render_states |= BGFX_STATE_BLEND_ALPHA;
		}
		else
		{
			log_error("Renderer Uknown blend mode %s in %s.", mode, shader->getPath().c_str());
		}
	}

	static e_void registerCFunction(lua_State* L, const e_char* name, lua_CFunction function)
	{
		lua_pushcfunction(L, function);
		lua_setglobal(L, name);
	}

	static ShaderCombinations* getCombinations(lua_State* L)
	{
		ShaderCombinations* ret = nullptr;
		if (lua_getglobal(L, "this") == LUA_TLIGHTUSERDATA)
		{
			ret = lua_manager::toType<ShaderCombinations*>(L, -1);
		}
		lua_pop(L, 1);
		return ret;
	}

	static Renderer* getRendererGlobal(lua_State* L)
	{
		Renderer* renderer = nullptr;
		if (lua_getglobal(L, "renderer") == LUA_TLIGHTUSERDATA)
		{
			renderer = lua_manager::toType<Renderer*>(L, -1);
		}
		lua_pop(L, 1);

		if (!renderer)
		{
			log_error("Renderer Error executing function texture_define, missing renderer global variable");
		}
		return renderer;
	}

	static e_void texture_define(lua_State* L, const e_char* define)
	{
		auto* shader = getShader(L);
		if (!shader) return;
		Renderer* renderer = getRendererGlobal(L);
		if (!renderer) return;

		auto& slot = shader->m_texture_slots[shader->m_texture_slot_count - 1];
		slot.define_idx = renderer->getShaderDefineIdx(lua_tostring(L, -1));
	}

	static e_void uniform(lua_State* L, const e_char* name, const e_char* type)
	{
		auto* shader = getShader(L);
		if (!shader) return;
		auto& u = shader->m_uniforms.emplace();
		StringUnitl::copyString(u.name, name);
		u.name_hash = crc32(name);
		if (StringUnitl::equalStrings(type, "float"))
		{
			u.type = Shader::Uniform::e_float;
			u.handle = bgfx::createUniform(name, bgfx::UniformType::Vec4);
		}
		else if (StringUnitl::equalStrings(type, "color"))
		{
			u.type = Shader::Uniform::COLOR;
			u.handle = bgfx::createUniform(name, bgfx::UniformType::Vec4);
		}
		else if (StringUnitl::equalStrings(type, "int32"))
		{
			u.type = Shader::Uniform::INT32;
			u.handle = bgfx::createUniform(name, bgfx::UniformType::Int1);
		}
		else if (StringUnitl::equalStrings(type, "float4x4"))
		{
			u.type = Shader::Uniform::MATRIX4;
			u.handle = bgfx::createUniform(name, bgfx::UniformType::Mat4);
		}
		else if (StringUnitl::equalStrings(type, "time"))
		{
			u.type = Shader::Uniform::TIME;
			u.handle = bgfx::createUniform(name, bgfx::UniformType::Vec4);
		}
		else if (StringUnitl::equalStrings(type, "float3"))
		{
			u.type = Shader::Uniform::FLOAT3;
			u.handle = bgfx::createUniform(name, bgfx::UniformType::Vec4);
		}
		else if (StringUnitl::equalStrings(type, "float2"))
		{
			u.type = Shader::Uniform::FLOAT2;
			u.handle = bgfx::createUniform(name, bgfx::UniformType::Vec4);
		}
		else
		{
			log_error("Renderer  Unknown uniform type %s in %s", type, shader->getPath().c_str());
		}
	}

	static e_void pass(lua_State* state, const e_char* name)
	{
		auto* cmb = getCombinations(state);
		StringUnitl::copyString(cmb->passes[cmb->pass_count].data, name);
		cmb->vs_local_mask[cmb->pass_count] = 0;
		cmb->fs_local_mask[cmb->pass_count] = 0;
		++cmb->pass_count;
	}

	static e_void depth_test(lua_State* L, e_bool enabled)
	{
		Shader* shader = nullptr;
		if (lua_getglobal(L, "shader") == LUA_TLIGHTUSERDATA)
		{
			shader = lua_manager::toType<Shader*>(L, -1);
		}
		lua_pop(L, 1);
		if (!shader) return;
		if (enabled)
		{
			shader->m_render_states |= BGFX_STATE_DEPTH_TEST_LEQUAL;
		}
		else
		{
			shader->m_render_states &= ~BGFX_STATE_DEPTH_TEST_MASK;
		}
	}

	static e_void fs(lua_State* L)
	{
		auto* cmb = getCombinations(L);
		Renderer* renderer = getRendererGlobal(L);
		if (!renderer) return;

		lua_manager::checkTableArg(L, 1);
		e_int32 len = (e_int32)lua_rawlen(L, 1);
		for (e_int32 i = 0; i < len; ++i)
		{
			if (lua_rawgeti(L, 1, i + 1) == LUA_TSTRING)
			{
				const e_char* tmp = lua_tostring(L, -1);
				e_int32 define_idx = renderer->getShaderDefineIdx(tmp);
				cmb->all_defines_mask |= 1 << define_idx;
				cmb->fs_local_mask[cmb->pass_count - 1] |= 1 << indexOf(*cmb, define_idx);
			}
			lua_pop(L, 1);
		}
	}

	static e_void vs(lua_State* L)
	{
		auto* cmb = getCombinations(L);
		Renderer* renderer = getRendererGlobal(L);
		if (!renderer) return;

		lua_manager::checkTableArg(L, 1);
		e_int32 len = (e_int32)lua_rawlen(L, 1);
		for (e_int32 i = 0; i < len; ++i)
		{
			if (lua_rawgeti(L, 1, i + 1) == LUA_TSTRING)
			{
				const e_char* tmp = lua_tostring(L, -1);
				e_int32 define_idx = renderer->getShaderDefineIdx(tmp);
				cmb->all_defines_mask |= 1 << define_idx;
				cmb->vs_local_mask[cmb->pass_count - 1] |= 1 << indexOf(*cmb, define_idx);
			}
			lua_pop(L, 1);
		}
	}

	//**********************************************************************************************************
	//**********************************************************************************************************
	//**********************************************************************************************************
	static e_void registerFunctions
	(
		Shader* shader,
		ShaderCombinations* combinations,
		Renderer* renderer,
		lua_State* L
	)
	{
		lua_pushlightuserdata(L, combinations);
		lua_setglobal(L, "this");
		lua_pushlightuserdata(L, renderer);
		lua_setglobal(L, "renderer");
		lua_pushlightuserdata(L, shader);
		lua_setglobal(L, "shader");
		registerCFunction(L, "pass", &lua_manager::wrap<decltype(&pass), pass>);
		registerCFunction(L, "fs", &lua_manager::wrap<decltype(&fs), fs>);
		registerCFunction(L, "vs", &lua_manager::wrap<decltype(&vs), vs>);
		registerCFunction(L, "depth_test", &lua_manager::wrap<decltype(&depth_test), depth_test>);
		registerCFunction(L, "alpha_blending", &lua_manager::wrap<decltype(&alpha_blending), alpha_blending>);
		registerCFunction(L, "texture_slot", &lua_manager::wrap<decltype(&texture_slot), texture_slot>);
		registerCFunction(L, "texture_define", &lua_manager::wrap<decltype(&texture_define), texture_define>);
		registerCFunction(L, "uniform", &lua_manager::wrap<decltype(&uniform), uniform>);
	}
}

