#ifndef _render_h_
#define _render_h_
#pragma once
#include "common/type.h"
#include "common/resource/resource_define.h"

namespace bgfx
{
	struct UniformHandle;
	struct VertexDecl;
}

namespace egal
{
	class Engine;
	struct FontAtlas;
	class MaterialManager;
	class EntityManager;
	class Pipeline;
	class ArchivePath;
	class Shader;
	class TextureManager;

	class Renderer
	{
	public:
		virtual ~Renderer() {}
		virtual void frame(bool capture) = 0;
		virtual void resize(int width, int height) = 0;
		virtual int getViewCounter() const = 0;
		virtual void viewCounterAdd() = 0;
		virtual void makeScreenshot(const ArchivePath& filename) = 0;
		virtual int getPassIdx(const e_char* pass) = 0;
		virtual const e_char* getPassName(int idx) = 0;
		virtual e_uint8 getShaderDefineIdx(const e_char* define) = 0;
		virtual const e_char* getShaderDefine(int define_idx) = 0;
		virtual int getShaderDefinesCount() const = 0;
		virtual const bgfx::VertexDecl& getBasicVertexDecl() const = 0;
		virtual const bgfx::VertexDecl& getBasic2DVertexDecl() const = 0;
		virtual MaterialManager& getMaterialManager() = 0;
		virtual EntityManager& getEntityManager() = 0;
		virtual TextureManager& getTextureManager() = 0;
		virtual Shader* getDefaultShader() = 0;
		virtual const UniformHandle& getMaterialColorUniform() const = 0;
		virtual const UniformHandle& getRoughnessMetallicUniform() const = 0;
		virtual int getLayersCount() const = 0;
		virtual int getLayer(const e_char* name) = 0;
		virtual const e_char* getLayerName(int idx) const = 0;
		virtual FontAtlas& getFontAtlas() = 0;
		virtual void setMainPipeline(Pipeline* pipeline) = 0;
		virtual Pipeline* getMainPipeline() = 0;

		virtual Engine& getEngine() = 0;
	};
}

#endif
