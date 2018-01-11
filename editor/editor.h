#ifndef _editor_h_
#define _editor_h_

#include "common.h"
#include <entry/input.h>
#include <bx/string.h>
#include "imgui.h"

#include <bx/pixelformat.h>
#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

namespace egal
{
	class Editor : public entry::AppI
	{
	public:
		Editor(const char* _name, const char* _description);

		void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override;

		virtual int shutdown() override;

		bool update() override;

		void createWindow();

		void destroyWindow();

	private:
		entry::MouseState m_mouseState;
		entry::WindowState m_windows[50];
		bgfx::FrameBufferHandle m_fbh[50];


		//IEngine*  m_p_engine;
	};
}
#endif
