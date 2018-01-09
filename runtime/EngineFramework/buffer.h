#ifndef _buffer_generator_h_
#define _buffer_generator_h_
#pragma once

#include "common/type.h"
#include "common/math/egal_math.h"
#include "common/allocator/egal_allocator.h"
#include "common/resource/resource_define.h"

#define BUFFER_SIZE 1024
struct lua_State;

namespace egal
{
	enum class BufferCommands : e_uint8
	{
		END,
		SET_TEXTURE,
		SET_UNIFORM_VEC4,
		SET_UNIFORM_TIME,
		SET_UNIFORM_ARRAY,
		SET_GLOBAL_SHADOWMAP,
		SET_LOCAL_SHADOWMAP,

		COUNT
	};

	struct SetTextureCommand
	{
		SetTextureCommand() : type(BufferCommands::SET_TEXTURE) {}
		
		BufferCommands		type;
		e_uint8				stage;
		bgfx::UniformHandle uniform;
		bgfx::TextureHandle texture;
		e_uint32			flags;
	};

	struct SetUniformVec4Command
	{
		SetUniformVec4Command() : type(BufferCommands::SET_UNIFORM_VEC4) {}
		
		BufferCommands		type;
		bgfx::UniformHandle uniform;
		float4				value;
	};

	struct SetUniformTimeCommand
	{
		SetUniformTimeCommand() : type(BufferCommands::SET_UNIFORM_TIME) {}
		
		BufferCommands		type;
		bgfx::UniformHandle uniform;
	};

	struct SetLocalShadowmapCommand
	{
		SetLocalShadowmapCommand() : type(BufferCommands::SET_LOCAL_SHADOWMAP) {}
		
		BufferCommands		type;
		bgfx::TextureHandle texture;
	};

	struct SetUniformArrayCommand
	{
		SetUniformArrayCommand() : type(BufferCommands::SET_UNIFORM_ARRAY) {}
		
		BufferCommands		type;
		bgfx::UniformHandle uniform;
		e_uint16			size;
		e_uint16			count;
	};

	struct CommandBufferGenerator
	{
		CommandBufferGenerator();

		e_void setTexture(e_uint8 stage, const UniformHandle& uniform, const TextureHandle& texture, e_uint32 flags = 0xffffFFFF);
		e_void setUniform(const UniformHandle& uniform, const float4& value);
		e_void setUniform(const UniformHandle& uniform, const float4* values, e_int32 count);
		e_void setUniform(const UniformHandle& uniform, const float4x4* values, e_int32 count);
		e_void setTimeUniform(const UniformHandle& uniform);
		e_void setLocalShadowmap(const TextureHandle& shadowmap);
		e_void setGlobalShadowmap();
		e_int32 getSize() const { return e_int32(pointer - buffer); }
		e_void getData(e_uint8* data);
		e_void clear();
		e_void beginAppend();
		e_void end();

		e_uint8		buffer[BUFFER_SIZE];
		e_uint8*	pointer;
	};

	static bgfx::TextureHandle invalid = BGFX_INVALID_HANDLE;
	class JsonSerializer;
	class FrameBuffer
	{
	public:
		struct RenderBuffer
		{
			static const e_uint32 DEFAULT_FLAGS = BGFX_TEXTURE_RT
				| BGFX_TEXTURE_U_CLAMP
				| BGFX_TEXTURE_V_CLAMP
				| BGFX_TEXTURE_MIP_POINT
				| BGFX_TEXTURE_MIN_POINT
				| BGFX_TEXTURE_MAG_POINT;

			bgfx::TextureFormat::Enum	m_format;
			bgfx::TextureHandle			m_handle;
			RenderBuffer*				m_shared = nullptr;

			e_void parse(lua_State* state);
		};

		struct Declaration
		{
			e_int32			m_width;
			e_int32			m_height;
			float2			m_size_ratio = float2(-1, -1);
			RenderBuffer	m_renderbuffers[16];
			e_int32			m_renderbuffers_count = 0;
			e_char			m_name[64];
		};

	public:
		explicit FrameBuffer(const Declaration& decl);
		FrameBuffer(const e_char* name, e_int32 width, e_int32 height, e_void* window_handle);
		~FrameBuffer();

		bgfx::FrameBufferHandle getHandle() const { return m_handle; }
		e_int32 getWidth() const {					return m_declaration.m_width; }
		e_int32 getHeight() const {					return m_declaration.m_height; }
		float2 getSizeRatio() const {				return m_declaration.m_size_ratio; }
		const e_char* getName() const {				return m_declaration.m_name; }
		e_int32 getRenderbuffersCounts() const {	return m_declaration.m_renderbuffers_count; }

		RenderBuffer& getRenderbuffer(e_int32 idx)
		{
			ASSERT(idx < m_declaration.m_renderbuffers_count);
			return m_declaration.m_renderbuffers[idx];
		}

		bgfx::TextureHandle& getRenderbufferHandle(e_int32 idx)
		{
			if (idx >= m_declaration.m_renderbuffers_count)
				return invalid;
			return m_declaration.m_renderbuffers[idx].m_handle;
		}

		e_void resize(e_int32 width, e_int32 height);
	private:
		e_void destroyRenderbuffers();

	private:
		e_bool						m_autodestroy_handle;
		e_void*						m_window_handle;
		bgfx::FrameBufferHandle		m_handle;
		Declaration					m_declaration;
	};
}
#endif
