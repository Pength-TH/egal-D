#ifndef _buffer_generator_h_
#define _buffer_generator_h_
#pragma once

#include "common/type.h"
#include "common/math/egal_math.h"
#include "common/allocator/allocator.h"
#include "common/resource/resource_define.h"

#define BUFFER_SIZE 1024
struct lua_State;

namespace egal
{
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

		e_uint8 buffer[BUFFER_SIZE];
		e_uint8* pointer;
	};

	class Entity;
	struct Pose
	{
		explicit Pose(IAllocator& allocator);
		~Pose();

		e_void resize(e_int32 count);
		e_void computeAbsolute(Entity& entity);
		e_void computeRelative(Entity& entity);
		e_void blend(Pose& rhs, e_float weight);

		IAllocator& allocator;
		e_bool is_absolute;
		e_int32 count;
		float3* positions;
		Quaternion* rotations;

	private:
		Pose(const Pose&);
		e_void operator =(const Pose&);
	};


}

#endif
