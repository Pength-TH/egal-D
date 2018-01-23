#ifndef _editor_h_
#define _editor_h_

#include "common.h"
#include "common/type.h"

#include <entry/input.h>

#include <bx/string.h>
#include <bx/pixelformat.h>
#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

#include "imgui.h"

#include "tools/base/editor_base.h"

#include "tools/import_assert/import_asset_dialog.h"
#include "tools/log/log_ui.h"

namespace egal
{
	class IEngine;
	struct MouseCoords
	{
		int32_t m_mx;
		int32_t m_my;
	};
	class Editor : public entry::AppI
	{
	public:
		Editor(const char* _name, const char* _description);

		void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override;

		virtual int shutdown() override;

		bool update() override;

		void camera_unity_update();

		void createWindow();
		void destroyWindow();

	private:
		
		entry::WindowState		m_windows[50];
		bgfx::FrameBufferHandle m_fbh[50];

		ImportAssetDialog*      m_p_import_assert;
		LogUI*					m_p_log_ui;

		IEngine*  m_p_engine;

		GameObject m_camera_object;
	};
}
#endif
