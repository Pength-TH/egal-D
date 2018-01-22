#ifndef _file_system_watcher_h_
#define _file_system_watcher_h_
#pragma once

#include "common/egal-d.h"

namespace egal
{
	struct IAllocator;
	class FileSystemWatcher
	{
	public:
		virtual ~FileSystemWatcher() {}

		static FileSystemWatcher* create(const char* path, IAllocator& allocator);
		static void destroy(FileSystemWatcher* watcher);

		virtual TDelegate<void(const char*)>& getCallback() = 0;
	};
}
#endif
