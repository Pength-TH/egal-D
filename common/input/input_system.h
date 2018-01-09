#ifndef _input_system_h_
#define _input_system_h_
#pragma once

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/math/egal_math.h"

#include "common/input/keycode.h"
#include "common/input/scancode.h"

namespace egal
{
	class EngineRoot;
	class InputSystem
	{
	public:
		struct Device
		{
			enum Type : e_uint32
			{
				MOUSE,
				KEYBOARD,
				CONTROLLER
			};

			Type type;
			int index = 0;

			virtual ~Device() {}
			virtual void update(float dt) = 0;
			virtual const char* getName() const = 0;
		};

		struct ButtonEvent
		{
			e_uint32 key_id;
			e_uint32 scancode;
			float x_abs;
			float y_abs;
			enum : e_uint32
			{
				UP,
				DOWN
			} state;
		};

		struct AxisEvent
		{
			enum Axis
			{
				LTRIGGER,
				RTRIGGER,
				LTHUMB,
				RTHUMB
			};

			float x;
			float y;
			float x_abs;
			float y_abs;
			Axis axis;
		};

		struct Event
		{
			enum Type : e_uint32
			{
				BUTTON,
				AXIS,
				DEVICE_ADDED,
				DEVICE_REMOVED
			};

			Type type;
			Device* device;
			union EventData
			{
				ButtonEvent button;
				AxisEvent axis;
			} data;
		};

	public:
		static InputSystem* create(EngineRoot& engine);
		static void destroy(InputSystem& system);

		virtual ~InputSystem() {}
		virtual IAllocator& getAllocator() = 0;
		virtual void enable(bool enabled) = 0;
		virtual void update(float dt) = 0;

		virtual void injectEvent(const Event& event) = 0;
		virtual int getEventsCount() const = 0;
		virtual const Event* getEvents() const = 0;

		virtual void addDevice(Device* device) = 0;
		virtual void removeDevice(Device* device) = 0;
		virtual Device* getMouseDevice() = 0;
		virtual Device* getKeyboardDevice() = 0;
		virtual int getDevicesCount() const = 0;
		virtual Device* getDevice(int index) = 0;

		virtual float2 getCursorPosition() const = 0;
		virtual void setCursorPosition(const float2& pos) = 0;
	};
}
#endif
