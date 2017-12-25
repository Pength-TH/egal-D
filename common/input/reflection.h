#ifndef _reflection_h_
#define _reflection_h_
#pragma once

#include "common/egal.h"
#include "runtime/Resource/resource_public.h"
#include "common/metaprogramming.h"

#define PROPERITY(Scene, Getter, Setter) \
	&Scene::Getter, #Scene "::" #Getter, &Scene::Setter, #Scene "::" #Setter

#define FUNCTION(Func) &Func, #Func

namespace egal
{

	template <typename T> class Array;
	class PropertyDescriptorBase;

	namespace Reflection
	{
		struct IAttribute
		{
			enum Type
			{
				MIN,
				CLAMP,
				RADIANS,
				COLOR,
				RESOURCE
			};

			virtual ~IAttribute() {}
			virtual e_int32 getType() const = 0;
		};

		struct ComponentBase;
		struct IPropertyVisitor;
		struct SceneBase;

		struct IAttributeVisitor
		{
			virtual ~IAttributeVisitor() {}
			virtual e_void visit(const IAttribute& attr) = 0;
		};

		struct PropertyBase
		{
			virtual ~PropertyBase() {}

			virtual e_void visit(IAttributeVisitor& visitor) const = 0;
			virtual e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const = 0;
			virtual e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const = 0;

			const e_char* name;
			const e_char* getter_code = "";
			const e_char* setter_code = "";
		};


		e_void init(IAllocator& allocator);
		e_void shutdown();

		e_int32 getScenesCount();
		const SceneBase& getScene(e_int32 index);

		const IAttribute* getAttribute(const PropertyBase& prop, IAttribute::Type type);
		e_void registerComponent(const ComponentBase& desc);
		e_void registerScene(const SceneBase& scene);
		const ComponentBase* getComponent(ComponentType cmp_type);
		const PropertyBase* getProperty(ComponentType cmp_type, const e_char* property);
		const PropertyBase* getProperty(ComponentType cmp_type, e_uint32 property_name_hash);
		const PropertyBase* getProperty(ComponentType cmp_type, const e_char* property, const e_char* subproperty);


		ComponentType getComponentType(const e_char* id);
		e_uint32 getComponentTypeHash(ComponentType type);
		ComponentType getComponentTypeFromHash(e_uint32 hash);
		e_int32 getComponentTypesCount();
		const e_char* getComponentTypeID(e_int32 index);


		namespace detail
		{
			template <typename T> e_void writeToStream(WriteBinary& stream, T value)
			{
				stream.write(value);
			}

			template <typename T> T readFromStream(ReadBinary& stream)
			{
				return stream.read<T>();
			}

			template <> ArchivePath readFromStream<ArchivePath>(ReadBinary& stream);
			template <> e_void writeToStream<ArchivePath>(WriteBinary& stream, ArchivePath);
			template <> e_void writeToStream<const ArchivePath&>(WriteBinary& stream, const ArchivePath& path);
			template <> const e_char* readFromStream<const e_char*>(ReadBinary& stream);
			template <> e_void writeToStream<const e_char*>(WriteBinary& stream, const e_char* path);


			template <typename Getter> struct GetterProxy;

			template <typename R, typename C>
			struct GetterProxy<R(C::*)(ComponentHandle, e_int32)>
			{
				using Getter = R(C::*)(ComponentHandle, e_int32);
				static e_void invoke(WriteBinary& stream, C* inst, Getter getter, ComponentHandle cmp, e_int32 index)
				{
					R value = (inst->*getter)(cmp, index);
					writeToStream(stream, value);
				}
			};

			template <typename R, typename C>
			struct GetterProxy<R(C::*)(ComponentHandle)>
			{
				using Getter = R(C::*)(ComponentHandle);
				static e_void invoke(WriteBinary& stream, C* inst, Getter getter, ComponentHandle cmp, e_int32 index)
				{
					R value = (inst->*getter)(cmp);
					writeToStream(stream, value);
				}
			};


			template <typename Setter> struct SetterProxy;

			template <typename C, typename A>
			struct SetterProxy<e_void (C::*)(ComponentHandle, e_int32, A)>
			{
				using Setter = e_void (C::*)(ComponentHandle, e_int32, A);
				static e_void invoke(ReadBinary& stream, C* inst, Setter setter, ComponentHandle cmp, e_int32 index)
				{
					using Value = RemoveCR<A>;
					auto value = readFromStream<Value>(stream);
					(inst->*setter)(cmp, index, value);
				}
			};

			template <typename C, typename A>
			struct SetterProxy<e_void (C::*)(ComponentHandle, A)>
			{
				using Setter = e_void (C::*)(ComponentHandle, A);
				static e_void invoke(ReadBinary& stream, C* inst, Setter setter, ComponentHandle cmp, e_int32 index)
				{
					using Value = RemoveCR<A>;
					auto value = readFromStream<Value>(stream);
					(inst->*setter)(cmp, value);
				}
			};
		}

		struct ResourceAttribute : IAttribute
		{
			ResourceAttribute(const e_char* file_type, ResourceType type) { this->file_type = file_type; this->type = type; }
			ResourceAttribute() {}

			e_int32 getType() const override { return RESOURCE; }

			const e_char* file_type;
			ResourceType type;
		};


		struct MinAttribute : IAttribute
		{
			explicit MinAttribute(e_float min) { this->min = min; }
			MinAttribute() {}

			e_int32 getType() const override { return MIN; }

			e_float min;
		};


		struct ClampAttribute : IAttribute
		{
			ClampAttribute(e_float min, e_float max) { this->min = min; this->max = max; }
			ClampAttribute() {}

			e_int32 getType() const override { return CLAMP; }

			e_float min;
			e_float max;
		};


		struct RadiansAttribute : IAttribute
		{
			e_int32 getType() const override { return RADIANS; }
		};


		struct ColorAttribute : IAttribute
		{
			e_int32 getType() const override { return COLOR; }
		};


		template <typename T> struct Property : PropertyBase {};


		struct IBlobProperty : PropertyBase {};


		struct ISampledFuncProperty : PropertyBase
		{
			virtual e_float getMaxX() const = 0;
		};


		struct IEnumProperty : public PropertyBase
		{
			e_void visit(IAttributeVisitor& visitor) const override {}
			virtual e_int32 getEnumCount(ComponentUID cmp) const = 0;
			virtual const e_char* getEnumName(ComponentUID cmp, e_int32 index) const = 0;
		};


		struct IArrayProperty : PropertyBase
		{
			virtual bool canAddRemove() const = 0;
			virtual e_void addItem(ComponentUID cmp, e_int32 index) const = 0;
			virtual e_void removeItem(ComponentUID cmp, e_int32 index) const = 0;
			virtual e_int32 getCount(ComponentUID cmp) const = 0;
			virtual e_void visit(IPropertyVisitor& visitor) const = 0;
		};


		struct IPropertyVisitor
		{
			virtual ~IPropertyVisitor() {}

			virtual e_void begin(const ComponentBase&) {}
			virtual e_void visit(const Property<e_float>& prop) = 0;
			virtual e_void visit(const Property<e_int32>& prop) = 0;
			virtual e_void visit(const Property<Entity>& prop) = 0;
			virtual e_void visit(const Property<Int2>& prop) = 0;
			virtual e_void visit(const Property<float2>& prop) = 0;
			virtual e_void visit(const Property<float3>& prop) = 0;
			virtual e_void visit(const Property<float4>& prop) = 0;
			virtual e_void visit(const Property<ArchivePath>& prop) = 0;
			virtual e_void visit(const Property<bool>& prop) = 0;
			virtual e_void visit(const Property<const e_char*>& prop) = 0;
			virtual e_void visit(const IArrayProperty& prop) = 0;
			virtual e_void visit(const IEnumProperty& prop) = 0;
			virtual e_void visit(const IBlobProperty& prop) = 0;
			virtual e_void visit(const ISampledFuncProperty& prop) = 0;
			virtual e_void end(const ComponentBase&) {}
		};


		struct ISimpleComponentVisitor : IPropertyVisitor
		{
			virtual e_void visitProperty(const PropertyBase& prop) = 0;

			e_void visit(const Property<e_float>& prop) override { visitProperty(prop); }
			e_void visit(const Property<e_int32>& prop) override { visitProperty(prop); }
			e_void visit(const Property<Entity>& prop) override { visitProperty(prop); }
			e_void visit(const Property<Int2>& prop) override { visitProperty(prop); }
			e_void visit(const Property<float2>& prop) override { visitProperty(prop); }
			e_void visit(const Property<float3>& prop) override { visitProperty(prop); }
			e_void visit(const Property<float4>& prop) override { visitProperty(prop); }
			e_void visit(const Property<ArchivePath>& prop) override { visitProperty(prop); }
			e_void visit(const Property<bool>& prop) override { visitProperty(prop); }
			e_void visit(const Property<const e_char*>& prop) override { visitProperty(prop); }
			e_void visit(const IArrayProperty& prop) override { visitProperty(prop); }
			e_void visit(const IEnumProperty& prop) override { visitProperty(prop); }
			e_void visit(const IBlobProperty& prop) override { visitProperty(prop); }
			e_void visit(const ISampledFuncProperty& prop) override { visitProperty(prop); }
		};


		struct IFunctionVisitor
		{
			virtual ~IFunctionVisitor() {}
			virtual e_void visit(const struct FunctionBase& func) = 0;
		};


		struct ComponentBase
		{
			virtual ~ComponentBase() {}

			virtual e_int32 getPropertyCount() const = 0;
			virtual e_int32 getFunctionCount() const = 0;
			virtual e_void visit(IPropertyVisitor&) const = 0;
			virtual e_void visit(IFunctionVisitor&) const = 0;

			const e_char* name;
			ComponentType component_type;
		};


		template <typename Getter, typename Setter, typename Namer>
		struct EnumProperty : IEnumProperty
		{
			e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				static_assert(4 == sizeof(typename ResultOf<Getter>::Type), "enum must have 4 bytes");
				detail::GetterProxy<Getter>::invoke(stream, inst, getter, cmp.handle, index);
			}

			e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);

				static_assert(4 == sizeof(typename ResultOf<Getter>::Type), "enum must have 4 bytes");
				detail::SetterProxy<Setter>::invoke(stream, inst, setter, cmp.handle, index);
			}


			e_int32 getEnumCount(ComponentUID cmp) const override
			{
				return count;
			}


			const e_char* getEnumName(ComponentUID cmp, e_int32 index) const override
			{
				return namer(index);
			}

			Getter getter;
			Setter setter;
			e_int32 count;
			Namer namer;
		};


		template <typename Getter, typename Setter, typename Counter, typename Namer>
		struct DynEnumProperty : IEnumProperty
		{
			e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				static_assert(4 == sizeof(typename ResultOf<Getter>::Type), "enum must have 4 bytes");
				detail::GetterProxy<Getter>::invoke(stream, inst, getter, cmp.handle, index);
			}

			e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);

				static_assert(4 == sizeof(typename ResultOf<Getter>::Type), "enum must have 4 bytes");
				detail::SetterProxy<Setter>::invoke(stream, inst, setter, cmp.handle, index);
			}


			e_int32 getEnumCount(ComponentUID cmp) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				return (inst->*counter)();
			}


			const e_char* getEnumName(ComponentUID cmp, e_int32 index) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				return (inst->*namer)(index);
			}

			Getter getter;
			Setter setter;
			Counter counter;
			Namer namer;
		};


		template <typename Getter, typename Setter, typename Counter>
		struct SampledFuncProperty : ISampledFuncProperty
		{
			e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const override
			{
				ASSERT(index == -1);
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				e_int32 count = (inst->*counter)(cmp.handle);
				stream.write(count);
				const float2* values = (inst->*getter)(cmp.handle);
				stream.write(values, sizeof(e_float) * 2 * count);
			}

			e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const override
			{
				ASSERT(index == -1);
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				e_int32 count;
				stream.read(count);
				auto* buf = (const float2*)stream.skip(sizeof(e_float) * 2 * count);
				(inst->*setter)(cmp.handle, buf, count);
			}

			e_float getMaxX() const override { return max_x; }

			e_void visit(IAttributeVisitor& visitor) const override {}

			Getter getter;
			Setter setter;
			Counter counter;
			e_float max_x;
		};


		template <typename Getter, typename Setter, typename... Attributes>
		struct BlobProperty : IBlobProperty
		{
			e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				(inst->*getter)(cmp.handle, stream);
			}

			e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				(inst->*setter)(cmp.handle, stream);
			}

			e_void visit(IAttributeVisitor& visitor) const override {
				apply([&](auto& x) { visitor.visit(x); }, attributes);
			}

			Tuple<Attributes...> attributes;
			Getter getter;
			Setter setter;
		};


		template <typename T, typename Getter, typename Setter, typename... Attributes>
		struct CommonProperty : Property<T>
		{
			e_void visit(IAttributeVisitor& visitor) const override
			{
				apply([&](auto& x) { visitor.visit(x); }, attributes);
			}

			e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				detail::GetterProxy<Getter>::invoke(stream, inst, getter, cmp.handle, index);
			}

			e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const override
			{
				using C = typename ClassOf<Getter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				detail::SetterProxy<Setter>::invoke(stream, inst, setter, cmp.handle, index);
			}


			Tuple<Attributes...> attributes;
			Getter getter;
			Setter setter;
		};


		template <typename Counter, typename Adder, typename Remover, typename... Props>
		struct ArrayProperty : IArrayProperty
		{
			ArrayProperty() {}


			bool canAddRemove() const override { return true; }


			e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const override
			{
				ASSERT(index == -1);

				e_int32 count;
				stream.read(count);
				while (getCount(cmp) < count)
				{
					addItem(cmp, -1);
				}
				while (getCount(cmp) > count)
				{
					removeItem(cmp, getCount(cmp) - 1);
				}
				for (e_int32 i = 0; i < count; ++i)
				{
					struct : ISimpleComponentVisitor
					{
						e_void visitProperty(const PropertyBase& prop) override
						{
							prop.setValue(cmp, index, *stream);
						}

						ReadBinary* stream;
						e_int32 index;
						ComponentUID cmp;

					} v;
					v.stream = &stream;
					v.index = i;
					v.cmp = cmp;
					visit(v);
				}
			}


			e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const override
			{
				ASSERT(index == -1);
				e_int32 count = getCount(cmp);
				stream.write(count);
				for (e_int32 i = 0; i < count; ++i)
				{
					struct : ISimpleComponentVisitor
					{
						e_void visitProperty(const PropertyBase& prop) override
						{
							prop.getValue(cmp, index, *stream);
						}

						WriteBinary* stream;
						e_int32 index;
						ComponentUID cmp;

					} v;
					v.stream = &stream;
					v.index = i;
					v.cmp = cmp;
					visit(v);
				}
			}


			e_void addItem(ComponentUID cmp, e_int32 index) const override
			{
				using C = typename ClassOf<Counter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				(inst->*adder)(cmp.handle, index);
			}


			e_void removeItem(ComponentUID cmp, e_int32 index) const override
			{
				using C = typename ClassOf<Counter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				(inst->*remover)(cmp.handle, index);
			}


			e_int32 getCount(ComponentUID cmp) const override
			{
				using C = typename ClassOf<Counter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				return (inst->*counter)(cmp.handle);
			}


			e_void visit(IPropertyVisitor& visitor) const override
			{
				apply([&](auto& x) { visitor.visit(x); }, properties);
			}


			e_void visit(IAttributeVisitor& visitor) const override {}


			Tuple<Props...> properties;
			Counter counter;
			Adder adder;
			Remover remover;
		};


		template <typename Counter, typename... Props>
		struct ConstArrayProperty : IArrayProperty
		{
			ConstArrayProperty() {}


			bool canAddRemove() const override { return false; }


			e_void setValue(ComponentUID cmp, e_int32 index, ReadBinary& stream) const override
			{
				ASSERT(index == -1);

				e_int32 count;
				stream.read(count);
				if (getCount(cmp) != count) return;

				for (e_int32 i = 0; i < count; ++i)
				{
					struct : ISimpleComponentVisitor
					{
						e_void visitProperty(const PropertyBase& prop) override
						{
							prop.setValue(cmp, index, *stream);
						}

						ReadBinary* stream;
						e_int32 index;
						ComponentUID cmp;

					} v;
					v.stream = &stream;
					v.index = i;
					v.cmp = cmp;
					visit(v);
				}
			}


			e_void getValue(ComponentUID cmp, e_int32 index, WriteBinary& stream) const override
			{
				ASSERT(index == -1);
				e_int32 count = getCount(cmp);
				stream.write(count);
				for (e_int32 i = 0; i < count; ++i)
				{
					struct : ISimpleComponentVisitor
					{
						e_void visitProperty(const PropertyBase& prop) override
						{
							prop.getValue(cmp, index, *stream);
						}

						WriteBinary* stream;
						e_int32 index;
						ComponentUID cmp;

					} v;
					v.stream = &stream;
					v.index = i;
					v.cmp = cmp;
					visit(v);
				}
			}


			e_void addItem(ComponentUID cmp, e_int32 index) const override { ASSERT(false); }
			e_void removeItem(ComponentUID cmp, e_int32 index) const override { ASSERT(false); }


			e_int32 getCount(ComponentUID cmp) const override
			{
				using C = typename ClassOf<Counter>::Type;
				C* inst = static_cast<C*>(cmp.scene);
				return (inst->*counter)(cmp.handle);
			}


			e_void visit(IPropertyVisitor& visitor) const override
			{
				apply([&](auto& x) { visitor.visit(x); }, properties);
			}


			e_void visit(IAttributeVisitor& visitor) const override {

			}


			Tuple<Props...> properties;
			Counter counter;
		};


		struct IComponentVisitor
		{
			virtual ~IComponentVisitor() {}
			virtual e_void visit(const ComponentBase& cmp) = 0;
		};


		struct SceneBase
		{
			virtual ~SceneBase() {}

			virtual e_int32 getFunctionCount() const = 0;
			virtual e_void visit(IFunctionVisitor&) const = 0;
			virtual e_void visit(IComponentVisitor& visitor) const = 0;

			const e_char* name;
		};


		template <typename Components, typename Funcs>
		struct Scene : SceneBase
		{
			e_int32 getFunctionCount() const override { return TupleSize<Funcs>::result; }


			e_void visit(IFunctionVisitor& visitor) const override
			{
				apply([&](const auto& func) { visitor.visit(func); }, functions);
			}


			e_void visit(IComponentVisitor& visitor) const override
			{
				apply([&](const auto& cmp) { visitor.visit(cmp); }, components);
			}


			Components components;
			Funcs functions;
		};


		template <typename Funcs, typename Props>
		struct Component : ComponentBase
		{
			e_int32 getPropertyCount() const override { return TupleSize<Props>::result; }
			e_int32 getFunctionCount() const override { return TupleSize<Funcs>::result; }


			e_void visit(IPropertyVisitor& visitor) const override
			{
				visitor.begin(*this);
				apply([&](auto& x) { visitor.visit(x); }, properties);
				visitor.end(*this);
			}


			e_void visit(IFunctionVisitor& visitor) const override
			{
				apply([&](auto& x) { visitor.visit(x); }, functions);
			}


			Props properties;
			Funcs functions;
		};


		template <typename... Components, typename... Funcs>
		auto scene(const e_char* name, Tuple<Funcs...> funcs, Components... components)
		{
			Scene<Tuple<Components...>, Tuple<Funcs...>> scene;
			scene.name = name;
			scene.functions = funcs;
			scene.components = makeTuple(components...);
			return scene;
		}


		template <typename... Components>
		auto scene(const e_char* name, Components... components)
		{
			Scene<Tuple<Components...>, Tuple<>> scene;
			scene.name = name;
			scene.components = makeTuple(components...);
			return scene;
		}


		struct FunctionBase
		{
			virtual ~FunctionBase() {}

			virtual e_int32 getArgCount() const = 0;
			virtual const e_char* getReturnType() const = 0;
			virtual const e_char* getArgType(e_int32 i) const = 0;

			const e_char* decl_code;
		};


		namespace internal
		{
			static const e_uint32 FRONT_SIZE = sizeof("Lumix::Reflection::internal::GetTypeNameHelper<") - 1u;
			static const e_uint32 BACK_SIZE = sizeof(">::GetTypeName") - 1u;

			template <typename T>
			struct GetTypeNameHelper
			{
				static const e_char* GetTypeName()
				{
					static const size_t size = sizeof(__FUNCTION__) - FRONT_SIZE - BACK_SIZE;
					static e_char typeName[size] = {};
					memcpy(typeName, __FUNCTION__ + FRONT_SIZE, size - 1u);

					return typeName;
				}
			};
		}


		template <typename T>
		const e_char* getTypeName()
		{
			return internal::GetTypeNameHelper<T>::GetTypeName();
		}


		template <typename F> struct Function;


		template <typename R, typename C, typename... Args>
		struct Function<R(C::*)(Args...)> : FunctionBase
		{
			using F = R(C::*)(Args...);
			F function;

			e_int32 getArgCount() const override { return ArgCount<F>::result; }
			const e_char* getReturnType() const override { return getTypeName<typename ResultOf<F>::Type>(); }

			const e_char* getArgType(e_int32 i) const override
			{
				const e_char* expand[] = {
					getTypeName<Args>()...
				};
				return expand[i];
			}
		};


		template <typename F>
		auto function(F func, const e_char* decl_code)
		{
			Function<F> ret;
			ret.function = func;
			ret.decl_code = decl_code;
			return ret;
		}


		template <typename... F>
		auto functions(F... functions)
		{
			Tuple<F...> f = makeTuple(functions...);
			return f;
		}


		template <typename... Props, typename... Funcs>
		auto component(const e_char* name, Tuple<Funcs...> functions, Props... props)
		{
			Component<Tuple<Funcs...>, Tuple<Props...>> cmp;
			cmp.name = name;
			cmp.functions = functions;
			cmp.properties = makeTuple(props...);
			cmp.component_type = getComponentType(name);
			return cmp;
		}


		template <typename... Props>
		auto component(const e_char* name, Props... props)
		{
			Component<Tuple<>, Tuple<Props...>> cmp;
			cmp.name = name;
			cmp.properties = makeTuple(props...);
			cmp.component_type = getComponentType(name);
			return cmp;
		}


		template <typename Getter, typename Setter, typename... Attributes>
		auto blob_property(const e_char* name, Getter getter, const e_char* getter_code, Setter setter, const e_char* setter_code, Attributes... attributes)
		{
			BlobProperty<Getter, Setter, Attributes...> p;
			p.attributes = makeTuple(attributes...);
			p.getter = getter;
			p.setter = setter;
			p.getter_code = getter_code;
			p.setter_code = setter_code;
			p.name = name;
			return p;
		}


		template <typename Getter, typename Setter, typename Counter>
		auto sampled_func_property(const e_char* name, Getter getter, const e_char* getter_code, Setter setter, const e_char* setter_code, Counter counter, e_float max_x)
		{
			using R = typename ResultOf<Getter>::Type;
			SampledFuncProperty<Getter, Setter, Counter> p;
			p.getter = getter;
			p.setter = setter;
			p.getter_code = getter_code;
			p.setter_code = setter_code;
			p.counter = counter;
			p.name = name;
			p.max_x = max_x;
			return p;
		}


		template <typename Getter, typename Setter, typename... Attributes>
		auto property(const e_char* name, Getter getter, const e_char* getter_code, Setter setter, const e_char* setter_code, Attributes... attributes)
		{
			using R = typename ResultOf<Getter>::Type;
			CommonProperty<R, Getter, Setter, Attributes...> p;
			p.attributes = makeTuple(attributes...);
			p.getter = getter;
			p.setter = setter;
			p.getter_code = getter_code;
			p.setter_code = setter_code;
			p.name = name;
			return p;
		}


		template <typename Getter, typename Setter, typename Namer, typename... Attributes>
		auto enum_property(const e_char* name, Getter getter, const e_char* getter_code, Setter setter, const e_char* setter_code, e_int32 count, Namer namer, Attributes... attributes)
		{
			EnumProperty<Getter, Setter, Namer> p;
			p.getter = getter;
			p.setter = setter;
			p.getter_code = getter_code;
			p.setter_code = setter_code;
			p.namer = namer;
			p.count = count;
			p.name = name;
			return p;
		}


		template <typename Getter, typename Setter, typename Counter, typename Namer, typename... Attributes>
		auto dyn_enum_property(const e_char* name, Getter getter, const e_char* getter_code, Setter setter, const e_char* setter_code, Counter counter, Namer namer, Attributes... attributes)
		{
			DynEnumProperty<Getter, Setter, Counter, Namer> p;
			p.getter = getter;
			p.setter = setter;
			p.getter_code = getter_code;
			p.setter_code = setter_code;
			p.namer = namer;
			p.counter = counter;
			p.name = name;
			return p;
		}


		template <typename Counter, typename Adder, typename Remover, typename... Props>
		auto array(const e_char* name, Counter counter, Adder adder, Remover remover, Props... properties)
		{
			ArrayProperty<Counter, Adder, Remover, Props...> p;
			p.name = name;
			p.counter = counter;
			p.adder = adder;
			p.remover = remover;
			p.properties = makeTuple(properties...);
			return p;
		}


		template <typename Counter, typename... Props>
		auto const_array(const e_char* name, Counter counter, Props... properties)
		{
			ConstArrayProperty<Counter, Props...> p;
			p.name = name;
			p.counter = counter;
			p.properties = makeTuple(properties...);
			return p;
		}

	}
}
#endif
