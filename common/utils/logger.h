#ifndef _egal_logger_h
#define _egal_logger_h
#pragma once

#include "common/type.h"
#include "common/config.h"

#include "common/stl/delegate.h"

namespace egal
{
	typedef TDelegateList<void(const int32_t, const char*)> Callback;

	Callback& log_call_back();

	e_void _log(e_int32 n, 
		const e_char* filename,
		const e_char* functionName, 
		const e_int32 fileline, 
		const e_char* fmt,
		...);
	
	e_void init_log();
	e_void close_log();

	e_int32 _check_error(const e_char* filename,
						 const e_int32 fileline,
						 const e_char* functionName,
						 const e_char* op);

	#define _log_info(fl, ln, fun, ...)   do{ _log(1, fl, ln, fun, __VA_ARGS__); } while(0)
	#define _log_waring(fl, ln, fun, ...) do{ _log(2, fl, ln, fun, __VA_ARGS__); } while(0)
	#define _log_error(fl, ln, fun, ...)  do{ _log(3, fl, ln, fun, __VA_ARGS__); } while(0)

	#define log_info(...)   _log_info(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
	#define log_waring(...) _log_waring(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
	#define log_error(...)  _log_error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

	#define check_error(op) _check_error(__FILE__, __LINE__, __FUNCTION__, op)

	
}

#endif