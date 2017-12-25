#include "common/utils/logger.h"

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "common/filesystem/ifile_device.h"

#include "common/egal_string.h"

namespace egal
{
	class Logger
	{
	public:
		Logger()
		{
			mLogFile = nullptr;
		}
		~Logger()
		{
			if (mLogFile)
			{
				mLogFile->close();
			}
		}
	public:
		void init()
		{
			if (!mLogFile)
				mLogFile = g_file_system->open(g_file_system->getDefaultDevice(), "./egal.log", FS::Mode::CREATE_AND_WRITE);
		}
		e_void write(const e_char * pcszLog)
		{
			init();
#if PLATFORM_WINDOWS
			mLogFile->write(pcszLog, StringUnitl::stringLength(pcszLog));
#elif PLATFORM_ANDROID

#elif PLATFORM_APPLE

#endif
		}
	private:
		FS::IFile*  mLogFile;
	};

	Logger logger;
	
	e_int32 _check_error(const e_char* filename, const e_int32 fileline, const e_char * functionName, const e_char* op)
	{
		e_int32 n = 0;
		e_int32 error = 0;
		//for (GLint error = glGetError(); error; error = glGetError())
		{

			if (error == 0x0500)
				_log_error(filename, functionName, fileline, "after %s() glError (0x%x GL_INVALID_ENUM)", op, error);
			else if (error == 0x0501)
				_log_error(filename, functionName, fileline, "after %s() glError (0x%x GL_INVALID_VALUE)", op, error);
			else if (error == 0x0502)
				_log_error(filename, functionName, fileline, "after %s() glError (0x%x GL_INVALID_OPERATION)", op, error);
			else if (error == 0x0505)
				_log_error(filename, functionName, fileline, "after %s() glError (0x%x GL_OUT_OF_MEMORY)", op, error);
			else
				_log_error(filename, functionName, fileline, "after %s() glError (0x%x UNKNOWN)", op, error);
			n++;
		}
		return n;
	}

	egal::e_void _log(e_int32 n, const e_char* filename, const e_char * functionName, const e_int32 fileline, const e_char *fmt, ...)
	{
		char szParam[MAX_LOG_LENGTH];
		va_list pArgList;
		va_start(pArgList, fmt);
		vsnprintf(szParam, MAX_LOG_LENGTH, fmt, pArgList);
		va_end(pArgList);
		char szBuf[MAX_LOG_LENGTH];

		struct timeval
		{
			int tv_sec;
			int tv_usec;
		};

		time_t time_seconds = time(0);
		struct tm now_time;
#if PLATFORM_WINDOWS
		localtime_s(&now_time, &time_seconds);
#endif

		switch (n)
		{
		case 1:
			snprintf(szBuf, MAX_LOG_LENGTH, "[info][%d-%d-%d %d:%d:%d]: %s\r\n",
				now_time.tm_year + 1900, now_time.tm_mon + 1, now_time.tm_mday,
				now_time.tm_hour, now_time.tm_min, now_time.tm_sec, szParam);
			break;
		case 2:
			snprintf(szBuf, MAX_LOG_LENGTH, "[waring][%d-%d-%d %d:%d:%d Fun:%s Line:%d] : %s\r\n",
				now_time.tm_year + 1900, now_time.tm_mon + 1, now_time.tm_mday,
				now_time.tm_hour, now_time.tm_min, now_time.tm_sec,
				functionName, fileline, szParam);
			break;
		case 3:
			snprintf(szBuf, MAX_LOG_LENGTH, "[error][%d-%d-%d %d:%d:%d Fun:%s Line:%d]:%s\r\n",
				now_time.tm_year + 1900, now_time.tm_mon + 1, now_time.tm_mday,
				now_time.tm_hour, now_time.tm_min, now_time.tm_sec,
				functionName, fileline, szParam);
			break;
		default:
			break;
		}

		logger.write(szBuf);
		//printf("%s\n", szLog);
	}
}
