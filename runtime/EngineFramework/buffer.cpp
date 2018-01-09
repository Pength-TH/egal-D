#include "runtime/EngineFramework/buffer.h"
#include "common/egal-d.h"
#include "common/resource/entity_manager.h"

#include <bgfx/bgfx.h>
#include <cmath>

namespace egal
{
	CommandBufferGenerator::CommandBufferGenerator()
	{
		pointer = buffer;
	}

	e_void CommandBufferGenerator::setTexture(e_uint8 stage, const UniformHandle& uniform, const TextureHandle& texture, e_uint32 flags)
	{
		SetTextureCommand cmd;
		cmd.stage = stage;
		cmd.uniform = uniform;
		cmd.texture = texture;
		cmd.flags = flags;
		ASSERT(pointer + sizeof(cmd) - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, &cmd, sizeof(cmd));
		pointer += sizeof(cmd);
	}

	e_void CommandBufferGenerator::setUniform(const UniformHandle& uniform, const float4& value)
	{
		SetUniformVec4Command cmd;
		cmd.uniform = uniform;
		cmd.value = value;
		ASSERT(pointer + sizeof(cmd) - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, &cmd, sizeof(cmd));
		pointer += sizeof(cmd);
	}

	e_void CommandBufferGenerator::setUniform(const UniformHandle& uniform, const float4* values, e_int32 count)
	{
		SetUniformArrayCommand cmd;
		cmd.uniform = uniform;
		cmd.count = count;
		cmd.size = e_uint16(count * sizeof(float4));
		ASSERT(pointer + sizeof(cmd) - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, &cmd, sizeof(cmd));
		pointer += sizeof(cmd);
		ASSERT(pointer + cmd.size - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, values, cmd.size);
		pointer += cmd.size;
	}

	e_void CommandBufferGenerator::setUniform(const UniformHandle& uniform, const float4x4* values, e_int32 count)
	{
		SetUniformArrayCommand cmd;
		cmd.uniform = uniform;
		cmd.count = count;
		cmd.size = e_uint16(count * sizeof(float4x4));
		ASSERT(pointer + sizeof(cmd) - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, &cmd, sizeof(cmd));
		pointer += sizeof(cmd);
		ASSERT(pointer + cmd.size - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, values, cmd.size);
		pointer += cmd.size;
	}

	e_void CommandBufferGenerator::setGlobalShadowmap()
	{
		ASSERT(pointer + 1 - buffer <= sizeof(buffer));
		*pointer = (e_uint8)BufferCommands::SET_GLOBAL_SHADOWMAP;
		pointer += 1;
	}

	e_void CommandBufferGenerator::setLocalShadowmap(const TextureHandle& shadowmap)
	{
		SetLocalShadowmapCommand cmd;
		cmd.texture = shadowmap;
		ASSERT(pointer + sizeof(cmd) - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, &cmd, sizeof(cmd));
		pointer += sizeof(cmd);
	}

	e_void CommandBufferGenerator::setTimeUniform(const UniformHandle& uniform)
	{
		SetUniformTimeCommand cmd;
		cmd.uniform = uniform;
		ASSERT(pointer + sizeof(cmd) - buffer <= sizeof(buffer));
		StringUnitl::copyMemory(pointer, &cmd, sizeof(cmd));
		pointer += sizeof(cmd);
	}

	e_void CommandBufferGenerator::getData(e_uint8* data)
	{
		StringUnitl::copyMemory(data, buffer, pointer - buffer);
	}

	e_void CommandBufferGenerator::clear()
	{
		buffer[0] = (e_uint8)BufferCommands::END;
		pointer = buffer;
	}

	e_void CommandBufferGenerator::beginAppend()
	{
		if (pointer != buffer) --pointer;
	}

	e_void CommandBufferGenerator::end()
	{
		ASSERT(pointer + 1 - buffer <= sizeof(buffer));
		*pointer = (e_uint8)BufferCommands::END;
		++pointer;
	}
}

namespace egal
{
	FrameBuffer::FrameBuffer(const Declaration& decl)
		: m_declaration(decl)
	{
		m_autodestroy_handle = true;
		bgfx::TextureHandle texture_handles[16];
		for (e_int32 i = 0; i < decl.m_renderbuffers_count; ++i)
		{
			const RenderBuffer& renderbuffer = decl.m_renderbuffers[i];
			if (renderbuffer.m_shared)
			{
				texture_handles[i] = renderbuffer.m_shared->m_handle;
			}
			else
			{
				texture_handles[i] = bgfx::createTexture2D((uint16_t)decl.m_width,
					(uint16_t)decl.m_height,
					false,
					1,
					renderbuffer.m_format,
					RenderBuffer::DEFAULT_FLAGS);
			}
			bgfx::setName(texture_handles[i], StaticString<128>(m_declaration.m_name, " - ", i));
			m_declaration.m_renderbuffers[i].m_handle = texture_handles[i];
		}

		m_window_handle = nullptr;
		m_handle = bgfx::createFrameBuffer((uint8_t)decl.m_renderbuffers_count, texture_handles);
		ASSERT(bgfx::isValid(m_handle));
	}

	FrameBuffer::FrameBuffer(const e_char* name, e_int32 width, e_int32 height, e_void* window_handle)
	{
		m_autodestroy_handle = false;
		StringUnitl::copyString(m_declaration.m_name, name);
		m_declaration.m_width = width;
		m_declaration.m_height = height;
		m_declaration.m_renderbuffers_count = 0;
		m_window_handle = window_handle;
		m_handle = bgfx::createFrameBuffer(window_handle, (uint16_t)width, (uint16_t)height);
		ASSERT(bgfx::isValid(m_handle));
	}

	FrameBuffer::~FrameBuffer()
	{
		if (m_autodestroy_handle)
		{
			destroyRenderbuffers();
			bgfx::destroy(m_handle);
		}
	}

	e_void FrameBuffer::destroyRenderbuffers()
	{
		for (e_int32 i = 0; i < m_declaration.m_renderbuffers_count; ++i)
		{
			RenderBuffer& rb = m_declaration.m_renderbuffers[i];
			if (!rb.m_shared)
				bgfx::destroy(rb.m_handle);
		}
	}

	e_void FrameBuffer::resize(e_int32 width, e_int32 height)
	{
		if (bgfx::isValid(m_handle))
		{
			destroyRenderbuffers();
			bgfx::destroy(m_handle);
		}

		m_declaration.m_width  = width;
		m_declaration.m_height = height;
		if (m_window_handle)
		{
			m_handle = bgfx::createFrameBuffer(m_window_handle, (uint16_t)width, (uint16_t)height);
		}
		else
		{
			bgfx::TextureHandle texture_handles[16];

			for (e_int32 i = 0; i < m_declaration.m_renderbuffers_count; ++i)
			{
				const RenderBuffer& renderbuffer = m_declaration.m_renderbuffers[i];
				if (renderbuffer.m_shared)
				{
					texture_handles[i] = renderbuffer.m_shared->m_handle;
				}
				else
				{
					texture_handles[i] = bgfx::createTexture2D(
						(uint16_t)width, (uint16_t)height, false, 1, renderbuffer.m_format, RenderBuffer::DEFAULT_FLAGS);
				}
				bgfx::setName(texture_handles[i], StaticString<128>(m_declaration.m_name, " - ", i));
				m_declaration.m_renderbuffers[i].m_handle = texture_handles[i];
			}

			m_window_handle = nullptr;
			m_handle = bgfx::createFrameBuffer((uint8_t)m_declaration.m_renderbuffers_count, texture_handles);
		}
	}

/**************************************************************************************/
/**************************************************************************************/
	static bgfx::TextureFormat::Enum getFormat(const e_char* name)
	{
		static const struct 
		{ 
			const e_char* name; 
			bgfx::TextureFormat::Enum value; 
		}
		FORMATS[] = 
		{
			{ "depth32",			bgfx::TextureFormat::D32	 },
			{ "depth24",			bgfx::TextureFormat::D24	 },
			{ "depth24stencil8",	bgfx::TextureFormat::D24S8   },
			{ "rgba8",				bgfx::TextureFormat::RGBA8   },
			{ "rgba16f",			bgfx::TextureFormat::RGBA16F },
			{ "r16f",				bgfx::TextureFormat::R16F	 },
			{ "r16",				bgfx::TextureFormat::R16	 },
			{ "r32f",				bgfx::TextureFormat::R32F	 },
		};

		for (auto& i : FORMATS)
		{
			if (StringUnitl::equalStrings(i.name, name))
				return i.value;
		}

		log_error("Renderer Uknown texture format %s. will use defatult format RGBA8", name);
		return bgfx::TextureFormat::RGBA8;
	}

	e_void FrameBuffer::RenderBuffer::parse(lua_State* L)
	{
		if (lua_getfield(L, -1, "format") == LUA_TSTRING)
		{
			m_format = getFormat(lua_tostring(L, -1));
		}
		else
		{
			m_format = bgfx::TextureFormat::RGBA8;
		}
		lua_pop(L, 1);
	}

/**************************************************************************************/
/**************************************************************************************/

}
