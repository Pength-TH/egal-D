#ifndef _egal_base_h__
#define _egal_base_h__
#pragma once

#include "egal.h"

namespace egal
{
	class NonCopyable
	{
	protected:
		NonCopyable() {}
		~NonCopyable() {}
	protected:
		NonCopyable(const NonCopyable&);
		NonCopyable& operator=(const NonCopyable&);
	};

	class Object : public NonCopyable
	{
	public:
		virtual ~Object(e_void) {}

		virtual const e_char* ClassName() const = 0;
		inline ObjectGUID GetInstanceID() const 
		{ 
			return (ObjectGUID)this; 
		}

		virtual void Invoke(e_wchar* /*fn*/, const e_void* /*arg*/, e_void** /*retVal*/) {}

	};

	class ObjectComponent : public Object
	{
	public:
		virtual const e_char* ClassName() const { return StaticClassName(); }
		static const e_char* StaticClassName() { return "ObjectComponent"; }

		virtual void Update(const FrameTime& fr, UpdateTypeEnum updateType) {}


		void SetActive(e_bool active) { m_active = active; }
		e_bool GetActive() const { return m_active; }

	private:
		e_bool m_active;
	};

	class TransformComponent : public ObjectComponent
	{
	public:
		const e_char* ClassName() const { return StaticClassName(); }
		static const e_char* StaticClassName() { return "TransformComponent"; }

		void SetPosition(const float3& trans) { m_position = trans; }
		const float3& GetPosition() const { return m_position; }
		void SetRotation(const float3& rot) { m_rotation = rot; }
		const float3& GetRotation() const { return m_rotation; }

		void SetScale(const float3& scale) { m_scale = scale; }
		const float3& GetScale() const { return m_scale; }

	private:
		float3 m_position;
		float3 m_rotation;
		float3 m_scale;
	};


	class RenderComponent : public TransformComponent
	{
	public:
		const e_char* ClassName() const { return StaticClassName(); }
		static const e_char* StaticClassName() { return "RenderComponent"; }

		void SetVisible(e_bool visible) { m_visible = visible; }
		e_bool GetVisible() const { return m_visible; }

		void SetCastShadow(e_bool castShadow) { m_castShadow = castShadow; }
		e_bool GetCastShadow() const { return m_castShadow; }

		void SetReceiveShadow(e_bool receiveShadow) { m_receiveShadow = receiveShadow; }
		e_bool GetReceiveShadow() const { return m_receiveShadow; }

		void SetDrawDistance(e_float drawDistance)
		{
			if (drawDistance < 0.0f) 
				drawDistance = 0.0f;
			
			m_drawDistance = drawDistance;
		}
		e_float GetDrawDistance() const { return m_drawDistance; }

	private:
		typedef TransformComponent super;
		e_bool m_visible;
		e_bool m_castShadow;
		e_bool m_receiveShadow;
		e_float m_drawDistance;
	};

}

#endif
