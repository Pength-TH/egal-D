#include "common/input/reflection.h"
#include "common/filesystem/binary.h"
#include "common/resource/resource_public.h"
#include "common/egal-d.h"
namespace egal
{
	namespace Reflection
	{
		namespace detail
		{
			template <> ArchivePath readFromStream<ArchivePath>(ReadBinary& stream)
			{
				const e_char* c_str = (const e_char*)stream.getData() + stream.getPosition();
				ArchivePath path(c_str);
				stream.skip(StringUnitl::stringLength(c_str) + 1);
				return path;
			}

			template <> e_void writeToStream<const ArchivePath&>(WriteBinary& stream, const ArchivePath& path)
			{
				const e_char* str = path.c_str();
				stream.write(str, StringUnitl::stringLength(str) + 1);
			}

			template <> e_void writeToStream<ArchivePath>(WriteBinary& stream, ArchivePath path)
			{
				const e_char* str = path.c_str();
				stream.write(str, StringUnitl::stringLength(str) + 1);
			}


			template <> const e_char* readFromStream<const e_char*>(ReadBinary& stream)
			{
				const e_char* c_str = (const e_char*)stream.getData() + stream.getPosition();
				stream.skip(StringUnitl::stringLength(c_str) + 1);
				return c_str;
			}


			template <> e_void writeToStream<const e_char*>(WriteBinary& stream, const e_char* value)
			{
				stream.write(value, StringUnitl::stringLength(value) + 1);
			}


		}

		struct ComponentTypeData
		{
			e_char id[50];
			e_uint32 id_hash;
		};


		static IAllocator* g_allocator = nullptr;
		static const SceneBase* g_scenes[64];
		static e_int32 g_scenes_count = 0;


		struct ComponentLink
		{
			const ComponentBase* desc;
			ComponentLink* next;
		};


		static ComponentLink* g_first_component = nullptr;


		const IAttribute* getAttribute(const PropertyBase& prop, IAttribute::Type type)
		{
			struct AttrVisitor : IAttributeVisitor
			{
				e_void visit(const IAttribute& attr) override
				{
					if (attr.getType() == type) result = &attr;
				}
				const IAttribute* result = nullptr;
				IAttribute::Type type;
			} attr_visitor;
			attr_visitor.type = type;
			prop.visit(attr_visitor);
			return attr_visitor.result;
		}


		const ComponentBase* getComponent(ComponentType cmp_type)
		{
			ComponentLink* link = g_first_component;
			while (link)
			{
				if (link->desc->component_type == cmp_type) return link->desc;
				link = link->next;
			}

			return nullptr;
		}


		const PropertyBase* getProperty(ComponentType cmp_type, e_uint32 property_name_hash)
		{
			auto* cmp = getComponent(cmp_type);
			if (!cmp) return nullptr;
			struct Visitor : ISimpleComponentVisitor
			{
				e_void visitProperty(const PropertyBase& prop) override {
					if (crc32(prop.name) == property_name_hash) result = &prop;
				}
				e_void visit(const IArrayProperty& prop) override {
					if (result) return;
					visitProperty(prop);
					prop.visit(*this);
				}

				e_uint32 property_name_hash;
				const PropertyBase* result = nullptr;
			} visitor;
			visitor.property_name_hash = property_name_hash;
			cmp->visit(visitor);
			return visitor.result;
		}



		const PropertyBase* getProperty(ComponentType cmp_type, const e_char* property, const e_char* subproperty)
		{
			auto* cmp = getComponent(cmp_type);
			if (!cmp) return nullptr;
			struct Visitor : ISimpleComponentVisitor
			{
				e_void visitProperty(const PropertyBase& prop) override {}

				e_void visit(const IArrayProperty& prop) override {
					if (StringUnitl::equalStrings(prop.name, property))
					{
						struct Subvisitor : ISimpleComponentVisitor
						{
							e_void visitProperty(const PropertyBase& prop) override {
								if (StringUnitl::equalStrings(prop.name, property)) result = &prop;
							}
							const e_char* property;
							const PropertyBase* result = nullptr;
						} subvisitor;
						subvisitor.property = subproperty;
						prop.visit(subvisitor);
						result = subvisitor.result;
					}
				}

				const e_char* subproperty;
				const e_char* property;
				const PropertyBase* result = nullptr;
			} visitor;
			visitor.subproperty = subproperty;
			visitor.property = property;
			cmp->visit(visitor);
			return visitor.result;
		}



		const PropertyBase* getProperty(ComponentType cmp_type, const e_char* property)
		{
			auto* cmp = getComponent(cmp_type);
			if (!cmp) return nullptr;
			struct Visitor : ISimpleComponentVisitor
			{
				e_void visitProperty(const PropertyBase& prop) override {
					if (StringUnitl::equalStrings(prop.name, property)) result = &prop;
				}

				const e_char* property;
				const PropertyBase* result = nullptr;
			} visitor;
			visitor.property = property;
			cmp->visit(visitor);
			return visitor.result;
		}


		e_void registerScene(const SceneBase& scene)
		{
			struct : IComponentVisitor
			{
				e_void visit(const ComponentBase& cmp) override
				{
					registerComponent(cmp);
				}
			} visitor;
			scene.visit(visitor);

			ASSERT(g_scenes_count < TlengthOf(g_scenes));
			g_scenes[g_scenes_count] = &scene;
			++g_scenes_count;
		}


		e_void registerComponent(const ComponentBase& desc)
		{
			ComponentLink* link = _aligned_new(*g_allocator, ComponentLink);
			link->next = g_first_component;
			link->desc = &desc;
			g_first_component = link;
		}


		static TVector<ComponentTypeData>& getComponentTypes()
		{
			static DefaultAllocator allocator;
			static TVector<ComponentTypeData> types(allocator);
			return types;
		}


		e_void init(IAllocator& allocator)
		{
			g_allocator = &allocator;
		}


		static e_void destroy(ComponentLink* link)
		{
			if (!link) return;
			destroy(link->next);
			_delete(*g_allocator, link);
		}


		e_void shutdown()
		{
			destroy(g_first_component);
			g_allocator = nullptr;
		}


		ComponentType getComponentTypeFromHash(e_uint32 hash)
		{
			auto& types = getComponentTypes();
			for (e_int32 i = 0, c = types.size(); i < c; ++i)
			{
				if (types[i].id_hash == hash)
				{
					return { i };
				}
			}
			ASSERT(false);
			return { -1 };
		}


		e_uint32 getComponentTypeHash(ComponentType type)
		{
			return getComponentTypes()[type.index].id_hash;
		}


		ComponentType getComponentType(const e_char* id)
		{
			e_uint32 id_hash = crc32(id);
			auto& types = getComponentTypes();
			for (e_int32 i = 0, c = types.size(); i < c; ++i)
			{
				if (types[i].id_hash == id_hash)
				{
					return { i };
				}
			}

			auto& cmp_types = getComponentTypes();
			if (types.size() == ComponentType::MAX_TYPES_COUNT)
			{
				log_error("Engine Too many component types");
				return INVALID_COMPONENT_TYPE;
			}

			ComponentTypeData& type = cmp_types.emplace();
			StringUnitl::copyString(type.id, id);
			type.id_hash = id_hash;
			return { getComponentTypes().size() - 1 };
		}

		e_int32 getComponentTypesCount()
		{
			return getComponentTypes().size();
		}

		const e_char* getComponentTypeID(e_int32 index)
		{
			return getComponentTypes()[index].id;
		}

		e_int32 getScenesCount()
		{
			return g_scenes_count;
		}

		const SceneBase& getScene(e_int32 index)
		{
			ASSERT(index < TlengthOf(g_scenes) && g_scenes[index]);
			return *g_scenes[index];
		}
	}
}