#include "editor.h"
//#include "runtime/IEngine.h"
namespace egal
{
	Editor::Editor(const char* _name, const char* _description)
		: entry::AppI(_name, _description)
	{

		entry::WindowHandle handle = entry::createWindow(rand() % 1280, rand() % 720, 640, 480);
		if (entry::isValid(handle))
		{
			char str[256];
			bx::snprintf(str, BX_COUNTOF(str), "Window - handle %d", handle.idx);
			entry::setWindowTitle(handle, str);
			m_windows[handle.idx].m_handle = handle;
		}

		//egal_params param;
		//param.hWnd = (HWND)handle.idx;
		//m_p_engine = IEngine::create(param);
	}

	void Editor::init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height)
	{
		
	}

	int Editor::shutdown()
	{
	

		return 0;
	}

	bool Editor::update()
	{
		//m_p_engine->run();
		return true;
	}

	void Editor::createWindow()
	{
		entry::WindowHandle handle = entry::createWindow(rand() % 1280, rand() % 720, 640, 480);
		if (entry::isValid(handle))
		{
			char str[256];
			bx::snprintf(str, BX_COUNTOF(str), "Window - handle %d", handle.idx);
			entry::setWindowTitle(handle, str);
			m_windows[handle.idx].m_handle = handle;
		}
	}

	void Editor::destroyWindow()
	{
		for (uint32_t ii = 0; ii < 50; ++ii)
		{
			if (bgfx::isValid(m_fbh[ii]))
			{
				bgfx::destroy(m_fbh[ii]);
				m_fbh[ii].idx = bgfx::kInvalidHandle;

				// Flush destruction of swap chain before destroying window!
				bgfx::frame();
				bgfx::frame();
			}

			if (entry::isValid(m_windows[ii].m_handle))
			{
				entry::destroyWindow(m_windows[ii].m_handle);
				m_windows[ii].m_handle.idx = UINT16_MAX;
				return;
			}
		}
	}
}

