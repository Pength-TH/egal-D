#ifndef _shader_complier_h_
#define _shader_complier_h_
#pragma once

#include "common/egal-d.h"

namespace egal
{
	struct ShaderCombinations;
	class WorldEditor;
	class FileSystemWatcher;
	class LogUI;
	class StudioApp;
	namespace PlatformInterface { struct Process; }

	class ShaderCompiler
	{
	public:
		ShaderCompiler(StudioApp& app, LogUI& log_ui);
		~ShaderCompiler();

		void makeUpToDate(bool wait);
		void update();
		const TVector<String>& getSHDFiles() const { return m_shd_files; }
		void compile(const char* path, bool debug);

	private:
		void compileTask();
		void findShaderFiles(const char* src_dir);
		bool getSourceFromBinaryBasename(char* out, int max_size, const char* binary_basename);
		void wait();
		void reloadShaders();
		void updateNotifications();
		void compileAllPasses(const char* path,
			bool is_vertex_shader,
			const int* define_masks,
			const ShaderCombinations& combinations,
			bool debug);
		void compilePass(const char* path,
			bool is_vertex_shader,
			const char* pass,
			int define_mask,
			const ShaderCombinations::Defines& all_defines,
			bool debug);
		bool isChanged(const ShaderCombinations& combinations, const char* bin_base_path, const char* shd_path) const;

		void onFileChanged(const char* path);
		void parseDependencies();
		Renderer& getRenderer();
		void addDependency(const char* key, const char* value);
		void processChangedFiles();
		void queueCompile(const char* path);

	private:
		struct LoadHook : ResourceManagerBase::LoadHook
		{
			LoadHook(ResourceManagerBase& manager, ShaderCompiler& compiler);
			bool onBeforeLoad(Resource& resource) override;
			ShaderCompiler& m_compiler;
		};

		struct ToCompile
		{
			StaticString<MAX_PATH_LENGTH> shd_file_path;
			Resource* resource;

			bool operator ==(const ToCompile& rhs) const { return shd_file_path == rhs.shd_file_path; }
		};

	private:
		StudioApp& m_app;
		WorldEditor& m_editor;
		FileSystemWatcher* m_watcher;
		int m_notifications_id;
		TMap<String, TVector<String>> m_dependencies;
		TVector<StaticString<MAX_PATH_LENGTH>> m_to_compile;
		StaticString<MAX_PATH_LENGTH> m_compiling;
		TVector<Resource*> m_hooked_files;
		TVector<String> m_to_reload;
		TVector<String> m_shd_files;
		TVector<String> m_changed_files;
		LoadHook m_load_hook;
		MT::SpinMutex m_mutex;
		LogUI& m_log_ui;
		bool m_is_opengl;
		volatile int m_empty_queue;
		volatile bool m_job_exit_request = false;
		volatile int m_job_runnig = 0;
	};
}
#endif
