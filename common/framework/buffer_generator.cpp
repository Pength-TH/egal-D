#include "common/framework/buffer_generator.h"
#include "common/egal-d.h"
#include "common/resource/entity_manager.h"

#include <bgfx/bgfx.h>
#include <cmath>

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

#pragma pack(1)
	struct SetTextureCommand
	{
		SetTextureCommand() : type(BufferCommands::SET_TEXTURE) {}
		BufferCommands type;
		e_uint8 stage;
		bgfx::UniformHandle uniform;
		bgfx::TextureHandle texture;
		e_uint32 flags;
	};

	struct SetUniformVec4Command
	{
		SetUniformVec4Command() : type(BufferCommands::SET_UNIFORM_VEC4) {}
		BufferCommands type;
		bgfx::UniformHandle uniform;
		float4 value;
	};

	struct SetUniformTimeCommand
	{
		SetUniformTimeCommand() : type(BufferCommands::SET_UNIFORM_TIME) {}
		BufferCommands type;
		bgfx::UniformHandle uniform;
	};

	struct SetLocalShadowmapCommand
	{
		SetLocalShadowmapCommand() : type(BufferCommands::SET_LOCAL_SHADOWMAP) {}
		BufferCommands type;
		bgfx::TextureHandle texture;
	};

	struct SetUniformArrayCommand
	{
		SetUniformArrayCommand() : type(BufferCommands::SET_UNIFORM_ARRAY) {}
		BufferCommands type;
		bgfx::UniformHandle uniform;
		e_uint16 size;
		e_uint16 count;
	};

#pragma pack()
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
	Pose::Pose(IAllocator& allocator)
		: allocator(allocator)
	{
		positions = nullptr;
		rotations = nullptr;
		count = 0;
		is_absolute = false;
	}

	Pose::~Pose()
	{
		allocator.deallocate(positions);
		allocator.deallocate(rotations);
	}

	e_void Pose::blend(Pose& rhs, e_float weight)
	{
		ASSERT(count == rhs.count);
		if (weight <= 0.001f) 
			return;
		weight = Math::clamp(weight, 0.0f, 1.0f);
		e_float inv = 1.0f - weight;
		for (e_int32 i = 0, c = count; i < c; ++i)
		{
			positions[i] = positions[i] * inv + rhs.positions[i] * weight;
			nlerp(rotations[i], rhs.rotations[i], &rotations[i], weight);
		}
	}

	e_void Pose::resize(e_int32 count)
	{
		is_absolute = false;
		allocator.deallocate(positions);
		allocator.deallocate(rotations);
		this->count = count;
		if (count)
		{
			positions = static_cast<float3*>(allocator.allocate(sizeof(float3) * count));
			rotations = static_cast<Quaternion*>(allocator.allocate(sizeof(Quaternion) * count));
		}
		else
		{
			positions = nullptr;
			rotations = nullptr;
		}
	}

	e_void Pose::computeAbsolute(Entity& entity)
	{
		//PROFILE_FUNCTION();
		if (is_absolute) 
			return;
		for (e_int32 i = entity.getFirstNonrootBoneIndex(); i < count; ++i)
		{
			e_int32 parent = entity.getBone(i).parent_idx;
			positions[i] = rotations[parent].rotate(positions[i]) + positions[parent];
			rotations[i] = rotations[parent] * rotations[i];
		}
		is_absolute = true;
	}

	e_void Pose::computeRelative(Entity& entity)
	{
		//PROFILE_FUNCTION();
		if (!is_absolute) return;
		for (e_int32 i = count - 1; i >= entity.getFirstNonrootBoneIndex(); --i)
		{
			e_int32 parent = entity.getBone(i).parent_idx;
			positions[i] = -rotations[parent].rotate(positions[i] - positions[parent]);
			rotations[i] = rotations[i] * -rotations[parent];
		}
		is_absolute = false;
	}
}
