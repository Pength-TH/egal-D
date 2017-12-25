#ifndef _struct_h_
#define _struct_h_
#pragma once

#include "common/type.h"
namespace egal
{
	class NoneCopyable
	{
	protected:
		NoneCopyable() {}
		~NoneCopyable() {}
	protected:
		NoneCopyable(const NoneCopyable&) {}
		NoneCopyable& operator=(const NoneCopyable&) {}
	};

	struct FrameTime
	{
		// Gets total simulation time in seconds.
		double TotalTime;
		// Gets elapsed time in seconds. since last update.
		e_float ElapsedTime;
	};

	namespace UpdateType
	{
		enum Enum
		{
			Editing,
			GamePlay,
			Paused,
		};
	}
	typedef UpdateType::Enum UpdateTypeEnum;

	struct ComponentType
	{
		enum { MAX_TYPES_COUNT = 64 };

		e_int32 index;
		bool operator==(const ComponentType& rhs) const { return rhs.index == index; }
		bool operator<(const ComponentType& rhs)  const { return rhs.index < index; }
		bool operator>(const ComponentType& rhs)  const { return rhs.index > index; }
		bool operator!=(const ComponentType& rhs) const { return rhs.index != index; }
	};
	const ComponentType INVALID_COMPONENT_TYPE = { -1 };

	struct ComponentHandle
	{
		e_int32 index;
		e_bool operator==(const ComponentHandle& rhs) const { return rhs.index == index; }
		e_bool operator<(const ComponentHandle& rhs)  const { return rhs.index < index; }
		e_bool operator>(const ComponentHandle& rhs)  const { return rhs.index > index; }
		e_bool operator!=(const ComponentHandle& rhs) const { return rhs.index != index; }
		e_bool isValid() const { return index >= 0; }
	};
	const ComponentHandle INVALID_COMPONENT = { -1 };

	struct GameObject
	{
		e_int32 index;
		e_bool operator==(const GameObject& rhs) const { return rhs.index == index; }
		e_bool operator<(const GameObject& rhs) const { return rhs.index < index; }
		e_bool operator>(const GameObject& rhs) const { return rhs.index > index; }
		e_bool operator!=(const GameObject& rhs) const { return rhs.index != index; }
		e_bool isValid() const { return index >= 0; }
	};
	const GameObject INVALID_ENTITY = { -1 };

	struct IScene;
	struct ComponentUID final
	{
		ComponentUID()
		{
			handle = INVALID_COMPONENT;
			scene = nullptr;
			entity = INVALID_ENTITY;
			type = { -1 };
		}

		ComponentUID(GameObject _entity, ComponentType _type, IScene* _scene, ComponentHandle _handle)
			: entity(_entity)
			, type(_type)
			, scene(_scene)
			, handle(_handle)
		{
		}

		GameObject entity;
		ComponentType type;
		IScene* scene;
		ComponentHandle handle;

		static const ComponentUID INVALID;

		bool operator==(const ComponentUID& rhs) const
		{
			return type == rhs.type && scene == rhs.scene && handle == rhs.handle;
		}
		bool isValid() const { return handle.isValid(); }
	};


}
#endif
