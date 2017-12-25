#include "common/utils/geometry.h"
#include "common/egal-d.h"

#include <cmath>

namespace egal
{
	Frustum::Frustum()
	{
		xs[6] = xs[7] = 1;
		ys[6] = ys[7] = 0;
		zs[6] = zs[7] = 0;
		ds[6] = ds[7] = 0;
	}

	bool Frustum::intersectAABB(const AABB& aabb) const
	{
		float3 box[] = { aabb._min, aabb._max };

		for (int i = 0; i < 6; ++i)
		{
			int px = (int)(xs[i] > 0.0f);
			int py = (int)(ys[i] > 0.0f);
			int pz = (int)(zs[i] > 0.0f);

			float dp =
				(xs[i] * box[px].x) +
				(ys[i] * box[py].y) +
				(zs[i] * box[pz].z);

			if (dp < -ds[i]) { return false; }
		}
		return true;
	}

	void Frustum::transform(const Matrix& mtx)
	{
		for (float3& p : points)
		{
			p = mtx.transformPoint(p);
		}

		for (int i = 0; i < TlengthOf(xs); ++i)
		{
			float3 p;
			if (xs[i] != 0) p.set(-ds[i] / xs[i], 0, 0);
			else if (ys[i] != 0) p.set(0, -ds[i] / ys[i], 0);
			else p.set(0, 0, -ds[i] / zs[i]);

			float3 n = { xs[i], ys[i], zs[i] };
			n = mtx.transformVector(n);
			p = mtx.transformPoint(p);

			xs[i] = n.x;
			ys[i] = n.y;
			zs[i] = n.z;
			ds[i] = -dotProduct(p, n);
		}
	}


	Sphere Frustum::computeBoundingSphere() const
	{
		Sphere sphere;
		sphere.position = points[0];
		for (int i = 1; i < TlengthOf(points); ++i)
		{
			sphere.position += points[i];
		}
		sphere.position *= 1.0f / TlengthOf(points);

		sphere.radius = 0;
		for (int i = 0; i < TlengthOf(points); ++i)
		{
			float len_sq = (points[i] - sphere.position).squaredLength();
			if (len_sq > sphere.radius) sphere.radius = len_sq;
		}
		sphere.radius = sqrtf(sphere.radius);
		return sphere;
	}


	bool Frustum::isSphereInside(const float3& center, float radius) const
	{
		float4 px = f4Load(xs);
		float4 py = f4Load(ys);
		float4 pz = f4Load(zs);
		float4 pd = f4Load(ds);

		float4 cx = f4Splat(center.x);
		float4 cy = f4Splat(center.y);
		float4 cz = f4Splat(center.z);

		float4 t = f4Mul(cx, px);
		t = f4Add(t, f4Mul(cy, py));
		t = f4Add(t, f4Mul(cz, pz));
		t = f4Add(t, pd);
		t = f4Sub(t, f4Splat(-radius));
		if (f4MoveMask(t)) return false;

		px = f4Load(&xs[4]);
		py = f4Load(&ys[4]);
		pz = f4Load(&zs[4]);
		pd = f4Load(&ds[4]);

		t = f4Mul(cx, px);
		t = f4Add(t, f4Mul(cy, py));
		t = f4Add(t, f4Mul(cz, pz));
		t = f4Add(t, pd);
		t = f4Sub(t, f4Splat(-radius));
		if (f4MoveMask(t)) return false;

		return true;
	}


	void Frustum::computeOrtho(const float3& position,
		const float3& direction,
		const float3& up,
		float width,
		float height,
		float near_distance,
		float far_distance)
	{
		computeOrtho(position, direction, up, width, height, near_distance, far_distance, { -1, -1 }, { 1, 1 });
	}


	static void setPlanesFromPoints(Frustum& frustum)
	{
		float3* points = frustum.points;

		float3 normal_near = -crossProduct(points[0] - points[1], points[0] - points[2]).normalized();
		float3 normal_far = crossProduct(points[4] - points[5], points[4] - points[6]).normalized();
		frustum.setPlane(Frustum::Planes::EXTRA0, normal_near, points[0]);
		frustum.setPlane(Frustum::Planes::EXTRA1, normal_near, points[0]);
		frustum.setPlane(Frustum::Planes::NEAR, normal_near, points[0]);
		frustum.setPlane(Frustum::Planes::FAR, normal_far, points[4]);

		frustum.setPlane(Frustum::Planes::LEFT, crossProduct(points[1] - points[2], points[1] - points[5]).normalized(), points[1]);
		frustum.setPlane(Frustum::Planes::RIGHT, -crossProduct(points[0] - points[3], points[0] - points[4]).normalized(), points[0]);
		frustum.setPlane(Frustum::Planes::TOP, crossProduct(points[0] - points[1], points[0] - points[4]).normalized(), points[0]);
		frustum.setPlane(Frustum::Planes::BOTTOM, crossProduct(points[2] - points[3], points[2] - points[6]).normalized(), points[2]);
	}


	static void setPoints(Frustum& frustum
		, const float3& near_center
		, const float3& far_center
		, const float3& right_near
		, const float3& up_near
		, const float3& right_far
		, const float3& up_far
		, const float2& viewport_min
		, const float2& viewport_max)
	{
		ASSERT(viewport_max.x >= viewport_min.x);
		ASSERT(viewport_max.y >= viewport_min.y);

		float3* points = frustum.points;

		points[0] = near_center + right_near * viewport_max.x + up_near * viewport_max.y;
		points[1] = near_center + right_near * viewport_min.x + up_near * viewport_max.y;
		points[2] = near_center + right_near * viewport_min.x + up_near * viewport_min.y;
		points[3] = near_center + right_near * viewport_max.x + up_near * viewport_min.y;

		points[4] = far_center + right_far * viewport_max.x + up_far * viewport_max.y;
		points[5] = far_center + right_far * viewport_min.x + up_far * viewport_max.y;
		points[6] = far_center + right_far * viewport_min.x + up_far * viewport_min.y;
		points[7] = far_center + right_far * viewport_max.x + up_far * viewport_min.y;

		setPlanesFromPoints(frustum);
	}


	void Frustum::computeOrtho(const float3& position,
		const float3& direction,
		const float3& up,
		float width,
		float height,
		float near_distance,
		float far_distance,
		const float2& viewport_min,
		const float2& viewport_max)
	{
		float3 z = direction;
		z.normalize();
		float3 near_center = position - z * near_distance;
		float3 far_center = position - z * far_distance;

		float3 x = crossProduct(up, z).normalized() * width;
		float3 y = crossProduct(z, x).normalized() * height;

		setPoints(*this, near_center, far_center, x, y, x, y, viewport_min, viewport_max);
	}


	void Frustum::setPlane(Planes side, const float3& normal, const float3& point)
	{
		xs[(u32)side] = normal.x;
		ys[(u32)side] = normal.y;
		zs[(u32)side] = normal.z;
		ds[(u32)side] = -dotProduct(point, normal);
	}


	void Frustum::setPlane(Planes side, const float3& normal, float d)
	{
		xs[(u32)side] = normal.x;
		ys[(u32)side] = normal.y;
		zs[(u32)side] = normal.z;
		ds[(u32)side] = d;
	}


	void Frustum::computePerspective(const float3& position,
		const float3& direction,
		const float3& up,
		float fov,
		float ratio,
		float near_distance,
		float far_distance,
		const float2& viewport_min,
		const float2& viewport_max)
	{
		ASSERT(near_distance > 0);
		ASSERT(far_distance > 0);
		ASSERT(near_distance < far_distance);
		ASSERT(fov > 0);
		ASSERT(ratio > 0);
		float scale = (float)tan(fov * 0.5f);
		float3 right = crossProduct(direction, up);
		float3 up_near = up * near_distance * scale;
		float3 right_near = right * (near_distance * scale * ratio);
		float3 up_far = up * far_distance * scale;
		float3 right_far = right * (far_distance * scale * ratio);

		float3 z = direction.normalized();

		float3 near_center = position + z * near_distance;
		float3 far_center = position + z * far_distance;

		setPoints(*this, near_center, far_center, right_near, up_near, right_far, up_far, viewport_min, viewport_max);
	}


	void Frustum::computePerspective(const float3& position,
		const float3& direction,
		const float3& up,
		float fov,
		float ratio,
		float near_distance,
		float far_distance)
	{
		computePerspective(position, direction, up, fov, ratio, near_distance, far_distance, { -1, -1 }, { 1, 1 });
	}


	void AABB::transform(const Matrix& matrix)
	{
		float3 points[8];
		points[0] = _min;
		points[7] = _max;
		points[1].set(points[0].x, points[0].y, points[7].z);
		points[2].set(points[0].x, points[7].y, points[0].z);
		points[3].set(points[0].x, points[7].y, points[7].z);
		points[4].set(points[7].x, points[0].y, points[0].z);
		points[5].set(points[7].x, points[0].y, points[7].z);
		points[6].set(points[7].x, points[7].y, points[0].z);

		for (int j = 0; j < 8; ++j)
		{
			points[j] = matrix.transformPoint(points[j]);
		}

		float3 new_min = points[0];
		float3 new_max = points[0];

		for (int j = 0; j < 8; ++j)
		{
			new_min = minCoords(points[j], new_min);
			new_max = maxCoords(points[j], new_max);
		}

		_min = new_min;
		_max = new_max;
	}

	void AABB::getCorners(const Matrix& matrix, float3* points) const
	{
		float3 p(_min.x, _min.y, _min.z);
		points[0] = matrix.transformPoint(p);
		p.set(_min.x, _min.y, _max.z);
		points[1] = matrix.transformPoint(p);
		p.set(_min.x, _max.y, _min.z);
		points[2] = matrix.transformPoint(p);
		p.set(_min.x, _max.y, _max.z);
		points[3] = matrix.transformPoint(p);
		p.set(_max.x, _min.y, _min.z);
		points[4] = matrix.transformPoint(p);
		p.set(_max.x, _min.y, _max.z);
		points[5] = matrix.transformPoint(p);
		p.set(_max.x, _max.y, _min.z);
		points[6] = matrix.transformPoint(p);
		p.set(_max.x, _max.y, _max.z);
		points[7] = matrix.transformPoint(p);
	}


	float3 AABB::minCoords(const float3& a, const float3& b)
	{
		return float3(Math::minimum(a.x, b.x), Math::minimum(a.y, b.y), Math::minimum(a.z, b.z));
	}


	float3 AABB::maxCoords(const float3& a, const float3& b)
	{
		return float3(Math::maximum(a.x, b.x), Math::maximum(a.y, b.y), Math::maximum(a.z, b.z));
	}

}
