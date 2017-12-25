#include "common/math/default/math_utils.h"
#include <cmath>
#include <random>

namespace egal
{
	namespace Math
	{
		float3 degreesToRadians(const float3& v)
		{
			return float3(degreesToRadians(v.x), degreesToRadians(v.y), degreesToRadians(v.z));
		}

		float3 radiansToDegrees(const float3& v)
		{
			return float3(radiansToDegrees(v.x), radiansToDegrees(v.y), radiansToDegrees(v.z));
		}

		float angleDiff(float a, float b)
		{
			float delta = a - b;
			delta = fmodf(delta, C_Pi * 2);
			if (delta > C_Pi) return -C_Pi * 2 + delta;
			if (delta < -C_Pi) return C_Pi * 2 + delta;
			return delta;
		}

		e_bool getRayPlaneIntersecion(const float3& origin,
			const float3& dir,
			const float3& plane_point,
			const float3& normal,
			float& out)
		{
			float d = dotProduct(dir, normal);
			if (d == 0)
			{
				return false;
			}
			d = dotProduct(plane_point - origin, normal) / d;
			out = d;
			return true;
		}

		e_bool getRaySphereIntersection(const float3& origin,
			const float3& dir,
			const float3& center,
			float radius,
			float3& out)
		{
			ASSERT(dir.length() < 1.01f && dir.length() > 0.99f);
			float3 L = center - origin;
			float tca = dotProduct(L, dir);
			if (tca < 0) return false;
			float d2 = dotProduct(L, L) - tca * tca;
			if (d2 > radius * radius) return false;
			float thc = sqrt(radius * radius - d2);
			float t0 = tca - thc;
			out = origin + dir * t0;
			return true;
		}

		e_bool getRayAABBIntersection(const float3& origin,
			const float3& dir,
			const float3& min,
			const float3& size,
			float3& out)
		{
			float3 dirfrac;

			dirfrac.x = 1.0f / (dir.x == 0 ? 0.00000001f : dir.x);
			dirfrac.y = 1.0f / (dir.y == 0 ? 0.00000001f : dir.y);
			dirfrac.z = 1.0f / (dir.z == 0 ? 0.00000001f : dir.z);

			float3 max = min + size;
			float t1 = (min.x - origin.x) * dirfrac.x;
			float t2 = (max.x - origin.x) * dirfrac.x;
			float t3 = (min.y - origin.y) * dirfrac.y;
			float t4 = (max.y - origin.y) * dirfrac.y;
			float t5 = (min.z - origin.z) * dirfrac.z;
			float t6 = (max.z - origin.z) * dirfrac.z;

			float tmin = Math::maximum(
				Math::maximum(Math::minimum(t1, t2), Math::minimum(t3, t4)), Math::minimum(t5, t6));
			float tmax = Math::minimum(
				Math::minimum(Math::maximum(t1, t2), Math::maximum(t3, t4)), Math::maximum(t5, t6));

			if (tmax < 0)
			{
				return false;
			}

			if (tmin > tmax)
			{
				return false;
			}

			out = tmin < 0 ? origin : origin + dir * tmin;
			return true;
		}


		float getLineSegmentDistance(const float3& origin, const float3& dir, const float3& a, const float3& b)
		{
			float3 a_origin = origin - a;
			float3 ab = b - a;

			float dot1 = dotProduct(ab, a_origin);
			float dot2 = dotProduct(ab, dir);
			float dot3 = dotProduct(dir, a_origin);
			float dot4 = dotProduct(ab, ab);
			float dot5 = dotProduct(dir, dir);

			float denom = dot4 * dot5 - dot2 * dot2;
			if (abs(denom) < 1e-5f)
			{
				float3 X = origin + dir * dotProduct(b - origin, dir);
				return (b - X).length();
			}

			float numer = dot1 * dot2 - dot3 * dot4;
			float param_a = numer / denom;
			float param_b = (dot1 + dot2 * param_a) / dot4;

			if (param_b < 0 || param_b > 1)
			{
				param_b = Math::clamp(param_b, 0.0f, 1.0f);
				float3 B = a + ab * param_b;
				float3 X = origin + dir * dotProduct(b - origin, dir);
				return (B - X).length();
			}

			float3 vec = (origin + dir * param_a) - (a + ab * param_b);
			return vec.length();
		}


		e_bool getRayTriangleIntersection(const float3& origin,
			const float3& dir,
			const float3& p0,
			const float3& p1,
			const float3& p2,
			float* out_t)
		{
			float3 normal = crossProduct(p1 - p0, p2 - p0);
			float q = dotProduct(normal, dir);
			if (q == 0) return false;

			float d = -dotProduct(normal, p0);
			float t = -(dotProduct(normal, origin) + d) / q;
			if (t < 0) return false;

			float3 hit_point = origin + dir * t;

			float3 edge0 = p1 - p0;
			float3 VP0 = hit_point - p0;
			if (dotProduct(normal, crossProduct(edge0, VP0)) < 0)
			{
				return false;
			}

			float3 edge1 = p2 - p1;
			float3 VP1 = hit_point - p1;
			if (dotProduct(normal, crossProduct(edge1, VP1)) < 0)
			{
				return false;
			}

			float3 edge2 = p0 - p2;
			float3 VP2 = hit_point - p2;
			if (dotProduct(normal, crossProduct(edge2, VP2)) < 0)
			{
				return false;
			}

			if (out_t) *out_t = t;
			return true;
		}


		e_bool getSphereTriangleIntersection(const float3& center,
			float radius,
			const float3& v0,
			const float3& v1,
			const float3& v2)
		{
			float3 normal = crossProduct(v0 - v1, v2 - v1).normalized();
			float D = -dotProduct(v0, normal);

			float dist = dotProduct(center, normal) + D;

			if (fabs(dist) > radius) return false;

			float squared_radius = radius * radius;
			if ((v0 - center).squaredLength() < squared_radius) return true;
			if ((v1 - center).squaredLength() < squared_radius) return true;
			if ((v2 - center).squaredLength() < squared_radius) return true;

			return false;
		}


		static std::mt19937_64& getGUIDRandomGenerator()
		{
			static std::random_device seed;
			static std::mt19937_64 gen(seed());

			return gen;
		}

		static std::mt19937& getRandomGenerator()
		{
			static std::random_device seed;
			static std::mt19937 gen(seed());

			return gen;
		}


		float pow(float base, float exponent)
		{
			return ::pow(base, exponent);
		}


		e_uint64 randGUID()
		{
			return getGUIDRandomGenerator()();
		}


		e_uint32 rand()
		{
			return getRandomGenerator()();
		}


		e_uint32 rand(e_uint32 from_incl, e_uint32 to_incl)
		{
			std::uniform_int_distribution<> dist(from_incl, to_incl);
			return dist(getRandomGenerator());
		}


		float randFloat()
		{
			std::uniform_real_distribution<float> dist;
			return dist(getRandomGenerator());
		}


		void seedRandom(e_uint32 seed)
		{
			getRandomGenerator().seed(seed);
		}


		float randFloat(float from, float to)
		{
			std::uniform_real_distribution<float> dist(from, to);
			return dist(getRandomGenerator());
		}
	}
}
