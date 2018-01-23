#ifndef _application_h_
#define _application_h_
#pragma once

#include "common/egal-d.h"

#include "common/filesystem/file_device.h"

#include "runtime/EngineFramework/engine_root.h"
#include "runtime/EngineFramework/component_manager.h"

#include "runtime/IEngine.h"

namespace egal
{
	class Resource;
	class App : public NoneCopyable, public IEngine
	{
	public:
		App(const egal_params& param);
		~App();

		void init();

		virtual e_bool resize(uint32_t width, uint32_t height);

		virtual e_bool run();
		virtual e_bool pause();
		virtual e_bool resume();

		virtual void frame_scale(e_float scale_time = 1.0f)
		{
			m_frame_scale_time = scale_time;
		}

		/** input system */
		virtual bool on_mouse_moved(double x, double y, double distance);
		virtual bool on_mouse_pressed(const MouseButtonID &mouseid, double distance);
		virtual bool on_mouse_released(const MouseButtonID &mouseid, double distance);

		virtual bool on_key_pressed(unsigned int textid, const KeyCode &keyCode);
		virtual bool on_key_released(unsigned int textid, const KeyCode &keyCode);

		virtual void update(double timeSinceLastFrame);
		/** input system - end */
	private:
		uint32_t m_width;
		uint32_t m_height;
		e_uint32 m_async_op;

		e_float m_frame_scale_time;

		DefaultAllocator m_main_allocator; //默认分配，可以采用第三方
		Debug::Allocator m_allocator;

		EngineRoot*			m_p_engine_root;
		Timer*				m_p_frame_timer;
		ComponentManager*	m_p_component_manager;

		MouseButtonID       m_mouse_button_id;
	};
}

#endif

