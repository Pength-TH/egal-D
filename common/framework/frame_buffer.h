#ifndef _frame_buffer_h_
#define _frame_buffer_h_
#pragma once

#include "common/egal.h"
#include <bgfx/bgfx.h>

struct lua_State;
namespace egal
{
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

			bgfx::TextureFormat::Enum m_format;
			bgfx::TextureHandle m_handle;
			RenderBuffer* m_shared = nullptr;

			e_void parse(lua_State* state);
		};

		struct Declaration
		{
			e_int32 m_width;
			e_int32 m_height;
			float2 m_size_ratio = float2(-1, -1);
			RenderBuffer m_renderbuffers[16];
			e_int32 m_renderbuffers_count = 0;
			e_char m_name[64];
		};

	public:
		explicit FrameBuffer(const Declaration& decl);
		FrameBuffer(const e_char* name, e_int32 width, e_int32 height, e_void* window_handle);
		~FrameBuffer();

		bgfx::FrameBufferHandle getHandle() const { return m_handle; }
		e_int32 getWidth() const { return m_declaration.m_width; }
		e_int32 getHeight() const { return m_declaration.m_height; }
		e_void resize(e_int32 width, e_int32 height);
		float2 getSizeRatio() const { return m_declaration.m_size_ratio; }
		const e_char* getName() const { return m_declaration.m_name; }
		e_int32 getRenderbuffersCounts() const { return m_declaration.m_renderbuffers_count; }

		RenderBuffer& getRenderbuffer(e_int32 idx)
		{
			ASSERT(idx < m_declaration.m_renderbuffers_count);
			return m_declaration.m_renderbuffers[idx];
		}

		bgfx::TextureHandle& getRenderbufferHandle(e_int32 idx)
		{
			static bgfx::TextureHandle invalid = BGFX_INVALID_HANDLE;
			if (idx >= m_declaration.m_renderbuffers_count) return invalid;
			return m_declaration.m_renderbuffers[idx].m_handle;
		}

	private:
		e_void destroyRenderbuffers();

	private:
		e_bool m_autodestroy_handle;
		e_void* m_window_handle;
		bgfx::FrameBufferHandle m_handle;
		Declaration m_declaration;
	};
}

#endif
