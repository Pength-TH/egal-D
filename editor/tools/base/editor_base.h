#ifndef _editor_base_h_
#define _editor_base_h_
#pragma once


namespace egal
{
	struct IWinodw
	{
		virtual ~IWinodw() {}

		virtual void onWindowGUI() = 0;
		virtual bool hasFocus() { return false; }
		virtual void update(float) {}
		virtual const char* getName() const = 0;
		virtual bool onDropFile(const char* file) { return false; }
	};

}
#endif

