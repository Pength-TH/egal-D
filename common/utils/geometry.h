#ifndef _geometry_h_
#define _geometry_h_

#include "common/define.h"
#include "common/math/egal_math.h"

namespace egal
{
	struct AABB;
	struct Matrix;

	enum class Planes : e_uint32
	{
		EP_NEAR,
		EP_FAR,
		EP_LEFT,
		EP_RIGHT,
		EP_TOP,
		EP_BOTTOM,
		EP_EXTRA0,
		EP_EXTRA1,
		EP_COUNT
	};

	struct Sphere
	{
		Sphere() {}

		Sphere(float x, float y, float z, float _radius)
			: position(x, y, z)
			, radius(_radius)
		{
		}

		Sphere(const float3& point, float _radius)
			: position(point)
			, radius(_radius)
		{
		}

		explicit Sphere(const float4& sphere)
			: position(sphere.x, sphere.y, sphere.z)
			, radius(sphere.w)
		{
		}

		float3 position;
		float radius;
	};

	struct Frustum
	{
		Frustum();

		void computeOrtho(const float3& position,
			const float3& direction,
			const float3& up,
			float width,
			float height,
			float near_distance,
			float far_distance);

		void computeOrtho(const float3& position,
			const float3& direction,
			const float3& up,
			float width,
			float height,
			float near_distance,
			float far_distance,
			const float2& viewport_min,
			const float2& viewport_max);

		void computePerspective(const float3& position,
			const float3& direction,
			const float3& up,
			float fov,
			float ratio,
			float near_distance,
			float far_distance);


		void computePerspective(const float3& position,
			const float3& direction,
			const float3& up,
			float fov,
			float ratio,
			float near_distance,
			float far_distance,
			const float2& viewport_min,
			const float2& viewport_max);

		bool intersectNearPlane(const float3& center, float radius) const
		{
			float x = center.x;
			float y = center.y;
			float z = center.z;
			e_uint32 i = (e_uint32)Planes::EP_NEAR;
			float distance = xs[i] * x + ys[i] * y + z * zs[i] + ds[i];
			distance = distance < 0 ? -distance : distance;
			return distance < radius;
		}

		bool intersectAABB(const AABB& aabb) const;
		bool isSphereInside(const float3& center, float radius) const;
		Sphere computeBoundingSphere() const;
		void transform(const Matrix& mtx);

		float3 getNormal(Planes side) const
		{
			return float3(xs[(int)side], ys[(int)side], zs[(int)side]);
		}

		void setPlane(Planes side, const float3& normal, const float3& point);
		void setPlane(Planes side, const float3& normal, float d);


		float xs[(int)Planes::EP_COUNT];
		float ys[(int)Planes::EP_COUNT];
		float zs[(int)Planes::EP_COUNT];
		float ds[(int)Planes::EP_COUNT];

		float3 points[8];
	};

	struct AABB
	{
		AABB() {}
		AABB(const float3& __min, const float3& __max)
			: _min(__min)
			, _max(__max)
		{
		}

		void set(const float3& __min, const float3& __max)
		{
			_min = __min;
			_max = __max;
		}

		void merge(const AABB& rhs)
		{
			addPoint(rhs._min);
			addPoint(rhs._max);
		}

		void addPoint(const float3& point)
		{
			_min = minCoords(point, _min);
			_max = maxCoords(point, _max);
		}

		bool overlaps(const AABB& aabb) const
		{
			if (_min.x > aabb._max.x) return false;
			if (_min.y > aabb._max.y) return false;
			if (_min.z > aabb._max.z) return false;
			if (aabb._min.x > _max.x) return false;
			if (aabb._min.y > _max.y) return false;
			if (aabb._min.z > _max.z) return false;
			return true;
		}

		void transform(const Matrix& matrix);
		void getCorners(const Matrix& matrix, float3* points) const;
		static float3 minCoords(const float3& a, const float3& b);
		static float3 maxCoords(const float3& a, const float3& b);

		float3 _min;
		float3 _max;
	};
}

#endif
