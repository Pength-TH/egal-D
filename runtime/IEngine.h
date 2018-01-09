#ifndef _iapplication_h_
#define _iapplication_h_

#include "common/input/framework_listener.h"

namespace egal
{
	/** 初始化引擎需要的参数 */
	#include "runtime/api/IFileSystem.h"
	typedef void(*callback_on_resource_loaded)();
	struct egal_params
	{
	public:
		egal_params() 
		: hWnd(0)
		, bWebPage(FALSE)
		, bOnlyEngine(FALSE)
		, szLogFileName(NULL)
		, szResourceLoadFile(NULL)

#if !DISABLE_FILE_AND_UI
		, pFileDownload(NULL)
		, pFileList(NULL)
#endif
		{
		}

	public:
		HWND hWnd;
		BOOL bWebPage;
		BOOL bOnlyEngine;

		char*  szLogFileName;
		char*  szResourceLoadFile;
		char*  szConfigFile;

#if !DISABLE_FILE_AND_UI
		IFileDownload*  pFileDownload;
		IFileList*		pFileList;
#endif
		callback_on_resource_loaded cbo_res_loaded;
	};

	/** 引擎入口 */
	class IEngine
	{
	public:
		/** 初始化引擎，并开启所有系统，
		* 调用该方法成功后，引擎正式工作
		* 返回值为空，表示引擎初始化失败，或者系统开启失败，均视为引擎无法使用
		*/
		static IEngine* create(const egal_params& param);

		/** 卸载引擎,关闭所有系统 */
		static void destroy(IEngine* engine);

	public:
		virtual ~IEngine() {}
		virtual bool resize(uint32_t width, uint32_t height) = 0;
		
		virtual bool run() = 0;
		virtual bool pause() = 0;
		virtual bool resume() = 0;

		virtual void frame_scale(float scale_time = 1.0) = 0;

		/** 获取场景 */
		//virtual ILevel* getLevel() = 0;




		/** imput system **/
		virtual bool on_mouse_moved(double x, double y, double distance) = 0;
		virtual bool on_mouse_pressed(const MouseButtonID &mouseid, double distance) = 0;
		virtual bool on_mouse_released(const MouseButtonID &mouseid, double distance) = 0;

		virtual bool on_key_pressed(unsigned int textid, const KeyCode &keyCode) = 0;
		virtual bool on_key_released(unsigned int textid, const KeyCode &keyCode) = 0;

		virtual void update(double timeSinceLastFrame) = 0;
	};
}
#endif
