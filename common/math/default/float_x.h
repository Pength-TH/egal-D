#ifndef _float_x_h_
#define _float_x_h_
#pragma once
#include "common/type.h"

namespace egal
{
	struct Int2
	{
		int x;
		int y;
	};


	struct float2
	{
		float2() {}

		explicit float2(float a)
			: x(a)
			, y(a)
		{
		}

		float2(float a, float b)
			: x(a)
			, y(b)
		{
		}

		void set(float a, float b)
		{
			x = a;
			y = b;
		}

		template<typename L>
		float& operator[](L i)
		{
			ASSERT(i >= 0 && i < 2);
			return (&x)[i];
		}

		template<typename L>
		float operator[](L i) const
		{
			ASSERT(i >= 0 && i < 2);
			return (&x)[i];
		}

		bool operator==(const float2& rhs) const
		{
			return x == rhs.x && y == rhs.y;
		}

		bool operator!=(const float2& rhs) const
		{
			return x != rhs.x || y != rhs.y;
		}

		void operator/=(float rhs)
		{
			*this *= 1.0f / rhs;
		}

		void operator*=(float f)
		{
			x *= f;
			y *= f;
		}

		float2 operator *(const float2& v) const { return float2(x * v.x, y * v.y); }
		float2 operator *(float f) const { return float2(x * f, y * f); }
		float2 operator /(float f) const { return float2(x / f, y / f); }
		float2 operator +(const float2& v) const { return float2(x + v.x, y + v.y); }
		void operator +=(const float2& v) { x += v.x; y += v.y; }
		float2 operator -(const float2& v) const { return float2(x - v.x, y - v.y); }
		float2 operator -(float f) const { return float2(x - f, y - f); }

		void normalize();
		float2 normalized() const;
		float length() const;
		float squaredLength() const;

		float x, y;

		static const float2 MAX;
		static const float2 MIN;
		static const float2 ZERO;
	};


	struct float3
	{
		float3() {}

		float3(const float2& v, float c)
			: x(v.x)
			, y(v.y)
			, z(c)
		{}

		explicit float3(float a)
			: x(a)
			, y(a)
			, z(a)
		{
		}

		float3(float a, float b, float c)
			: x(a)
			, y(b)
			, z(c)
		{
		}

		template<typename L>
		float& operator[](L i)
		{
			ASSERT(i >= 0 && i < 3);
			return (&x)[i];
		}

		template<typename L>
		float operator[](L i) const
		{
			ASSERT(i >= 0 && i < 3);
			return (&x)[i];
		}

		bool operator==(const float3& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}

		bool operator!=(const float3& rhs) const
		{
			return x != rhs.x || y != rhs.y || z != rhs.z;
		}

		float3 operator+(const float3& rhs) const { return float3(x + rhs.x, y + rhs.y, z + rhs.z); }

		float3 operator-() const { return float3(-x, -y, -z); }

		float3 operator-(const float3& rhs) const { return float3(x - rhs.x, y - rhs.y, z - rhs.z); }

		void operator+=(const float3& rhs)
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			this->x = x;
			this->y = y;
			this->z = z;
		}

		void operator-=(const float3& rhs)
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			this->x = x;
			this->y = y;
			this->z = z;
		}

		float3 operator*(float s) const { return float3(x * s, y * s, z * s); }

		float3 operator/(float s) const
		{
			float tmp = 1 / s;
			return float3(x * tmp, y * tmp, z * tmp);
		}

		void operator/=(float rhs)
		{
			*this *= 1.0f / rhs;
		}

		void operator*=(float rhs)
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			x *= rhs;
			y *= rhs;
			z *= rhs;
			this->x = x;
			this->y = y;
			this->z = z;
		}

		float3 normalized() const;

		void normalize();

		void set(float x, float y, float z)
		{
			this->x = x;
			this->y = y;
			this->z = z;
		}

		float length() const;

		float squaredLength() const
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			return x * x + y * y + z * z;
		}

		union
		{
			struct { float x, y, z; };
			struct { float r, g, b; };
		};

		static const float3 MAX;
		static const float3 MIN;
		static const float3 ZERO;
	};


	inline float3 operator *(float f, const float3& v)
	{
		return float3(f * v.x, f * v.y, f * v.z);
	}


	struct float4
	{
		float4() {}

		explicit float4(float a)
			: x(a)
			, y(a)
			, z(a)
			, w(a)
		{
		}

		float4(float a, float b, float c, float d)
			: x(a)
			, y(b)
			, z(c)
			, w(d)
		{
		}

		float4(const float2& v1, const float2& v2)
			: x(v1.x)
			, y(v1.y)
			, z(v2.x)
			, w(v2.y)
		{
		}

		float4(const float3& v, float d)
			: x(v.x)
			, y(v.y)
			, z(v.z)
			, w(d)
		{
		}

		float3 xyz() const { return float3(x, y, z); }
		float3 rgb() const { return float3(x, y, z); }

		template<typename L>
		float& operator[](L i)
		{
			ASSERT(i >= 0 && i < 4);
			return (&x)[i];
		}

		template<typename L>
		float operator[](L i) const
		{
			ASSERT(i >= 0 && i < 4);
			return (&x)[i];
		}

		bool operator==(const float4& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
		}

		bool operator!=(const float4& rhs) const
		{
			return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w;
		}

		float4 operator+(const float4& rhs) const
		{
			return float4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
		}

		float4 operator-() const { return float4(-x, -y, -z, -w); }

		float4 operator-(const float4& rhs) const
		{
			return float4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
		}

		void operator+=(const float4& rhs)
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			float w = this->w;
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
		}

		void operator-=(const float4& rhs)
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			float w = this->w;
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
		}

		void operator/=(float rhs)
		{
			*this *= 1.0f / rhs;
		}

		float4 operator*(float s) { return float4(x * s, y * s, z * s, w * s); }

		void operator*=(float rhs)
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			float w = this->w;
			x *= rhs;
			y *= rhs;
			z *= rhs;
			w *= rhs;
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
		}

		void normalize();
		float4 normalized() const;

		void set(const float3& v, float w)
		{
			this->x = v.x;
			this->y = v.y;
			this->z = v.z;
			this->w = w;
		}

		void set(float x, float y, float z, float w)
		{
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
		}

		void set(const float4& rhs)
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			w = rhs.w;
		}

		float length() const;

		float squaredLength() const
		{
			float x = this->x;
			float y = this->y;
			float z = this->z;
			float w = this->w;
			return x * x + y * y + z * z + w * w;
		}

		operator float3() { return float3(x, y, z); }

		union
		{
			struct { float x, y, z, w; };
			struct { float r, g, b, a; };
		};

		static const float4 MAX;
		static const float4 MIN;
		static const float4 ZERO;
	};


	inline float4 operator*(const float4& v, float s)
	{
		return float4(v.x * s, v.y * s, v.z * s, v.w * s);
	}


	inline float dotProduct(const float4& op1, const float4& op2)
	{
		return op1.x * op2.x + op1.y * op2.y + op1.z * op2.z + op1.w * op2.w;
	}


	inline void lerp(const float4& op1, const float4& op2, float4* out, float t)
	{
		const float invt = 1.0f - t;
		out->x = op1.x * invt + op2.x * t;
		out->y = op1.y * invt + op2.y * t;
		out->z = op1.z * invt + op2.z * t;
		out->w = op1.w * invt + op2.w * t;
	}


	inline float dotProduct(const float3& op1, const float3& op2)
	{
		return op1.x * op2.x + op1.y * op2.y + op1.z * op2.z;
	}


	inline float3 crossProduct(const float3& op1, const float3& op2)
	{
		return float3(op1.y * op2.z - op1.z * op2.y, op1.z * op2.x - op1.x * op2.z, op1.x * op2.y - op1.y * op2.x);
	}


	inline void lerp(const float3& op1, const float3& op2, float3* out, float t)
	{
		const float invt = 1.0f - t;
		out->x = op1.x * invt + op2.x * t;
		out->y = op1.y * invt + op2.y * t;
		out->z = op1.z * invt + op2.z * t;
	}


	inline void lerp(const float2& op1, const float2& op2, float2* out, float t)
	{
		const float invt = 1.0f - t;
		out->x = op1.x * invt + op2.x * t;
		out->y = op1.y * invt + op2.y * t;
	}
}
#endif
