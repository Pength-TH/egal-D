#ifndef _platform_interface_h_
#define _platform_interface_h_
#pragma once

#include "common/egal-d.h"

namespace egal
{
	struct IAllocator;
	namespace PlatformInterface
	{
		struct FileInfo
		{
			bool is_directory;
			char filename[MAX_PATH_LENGTH];
		};

		struct FileIterator;
		struct Process;

		FileIterator* createFileIterator(const char* path, IAllocator& allocator);
		void destroyFileIterator(FileIterator* iterator);
		bool getNextFile(FileIterator* iterator, FileInfo* info);

		void setCurrentDirectory(const char* path);
		void getCurrentDirectory(char* buffer, int buffer_size);
		bool getOpenFilename(char* out, int max_size, const char* filter, const char* starting_file);
		bool getSaveFilename(char* out, int max_size, const char* filter, const char* default_extension);
		bool getOpenDirectory(char* out, int max_size, const char* starting_dir);
		bool shellExecuteOpen(const char* path, const char* parameters);
		void copyToClipboard(const char* text);

		bool deleteFile(const char* path);
		bool moveFile(const char* from, const char* to);
		size_t getFileSize(const char* path);
		bool fileExists(const char* path);
		bool dirExists(const char* path);
		e_uint64 getLastModified(const char* file);
		bool makePath(const char* path);

		void clipCursor(int x, int y, int w, int h);
		void unclipCursor();

		Process* createProcess(const char* cmd, const char* args, IAllocator& allocator);
		void destroyProcess(Process& process);
		bool isProcessFinished(Process& process);
		int getProcessExitCode(Process& process);
		int getProcessOutput(Process& process, char* buf, int buf_size);
	}
}
#endif
