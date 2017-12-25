
#include "common/math/default/float_x.h"

#include <cmath>
#include <float.h>

namespace egal
{
	const float2 float2::MAX = float2(FLT_MAX);
	const float2 float2::MIN = float2(-FLT_MAX);
	const float2 float2::ZERO = float2(0);

	const float3 float3::MAX = float3(FLT_MAX);
	const float3 float3::MIN = float3(-FLT_MAX);
	const float3 float3::ZERO = float3(0);

	const float4 float4::MAX = float4(FLT_MAX);
	const float4 float4::MIN = float4(-FLT_MAX);
	const float4 float4::ZERO = float4(0);

	float float2::squaredLength() const
	{
		float x = this->x;
		float y = this->y;
		return x * x + y * y;
	}

	void float2::normalize()
	{
		float x = this->x;
		float y = this->y;
		float inv_len = 1 / sqrt(x * x + y * y);
		x *= inv_len;
		y *= inv_len;
		this->x = x;
		this->y = y;
	}

	float2 float2::normalized() const
	{
		float x = this->x;
		float y = this->y;
		float inv_len = 1 / sqrt(x * x + y * y);
		x *= inv_len;
		y *= inv_len;
		return float2(x, y);
	}

	float float2::length() const
	{
		float x = this->x;
		float y = this->y;
		return sqrt(x * x + y * y);
	}

	void float3::normalize()
	{
		float x = this->x;
		float y = this->y;
		float z = this->z;
		float inv_len = 1 / sqrt(x * x + y * y + z * z);
		x *= inv_len;
		y *= inv_len;
		z *= inv_len;
		this->x = x;
		this->y = y;
		this->z = z;
	}

	float3 float3::normalized() const
	{
		float x = this->x;
		float y = this->y;
		float z = this->z;
		float inv_len = 1 / sqrt(x * x + y * y + z * z);
		x *= inv_len;
		y *= inv_len;
		z *= inv_len;
		return float3(x, y, z);
	}

	float float3::length() const
	{
		float x = this->x;
		float y = this->y;
		float z = this->z;
		return sqrt(x * x + y * y + z * z);
	}

	void float4::normalize()
	{
		float x = this->x;
		float y = this->y;
		float z = this->z;
		float w = this->w;
		float inv_len = 1 / sqrt(x * x + y * y + z * z + w * w);
		x *= inv_len;
		y *= inv_len;
		z *= inv_len;
		w *= inv_len;
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	float4 float4::normalized() const
	{
		float x = this->x;
		float y = this->y;
		float z = this->z;
		float w = this->w;
		float inv_len = 1 / sqrt(x * x + y * y + z * z + w * w);
		x *= inv_len;
		y *= inv_len;
		z *= inv_len;
		w *= inv_len;
		return float4(x, y, z, w);
	}

	float float4::length() const
	{
		float x = this->x;
		float y = this->y;
		float z = this->z;
		float w = this->w;
		return sqrt(x * x + y * y + z * z + w * w);
	}
}
