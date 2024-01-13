#pragma once

#include "FlareCore/Core.h"

#include <array>

#include <glm/glm.hpp>

namespace Flare::Math
{
	FLARE_API bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

	struct Basis
	{
		inline glm::mat3 AsMatrix()
		{
			return glm::mat3(Right, Up, Forward);
		}

		glm::vec3 Right;
		glm::vec3 Up;
		glm::vec3 Forward;
	};

	struct Plane
	{
		inline float SignedDistance(const glm::vec3& point) const
		{
			return glm::dot(point, Normal) + Offset;
		}

		inline float Distance(const glm::vec3& point) const
		{
			return glm::abs(glm::dot(point, Normal) + Offset);
		}

		inline bool IntersectsSegment(const glm::vec3& segmentStart, const glm::vec3& segmentEnd) const
		{
			float startValue = SignedDistance(segmentStart);
			float endValue = SignedDistance(segmentEnd);
			return glm::sign(startValue) != glm::sign(endValue);
		}

		inline static Plane TroughPoint(glm::vec3 point, glm::vec3 planeNormal)
		{
			return { planeNormal, -glm::dot(point, planeNormal)};
		}

		inline static Plane FromPoints(glm::vec3 a, glm::vec3 b, glm::vec3 c)
		{
			glm::vec3 normal = glm::normalize(glm::cross(b - a, c - b));
			return Plane::TroughPoint(a, normal);
		}

		glm::vec3 Normal;
		float Offset;
	};

	struct Ray
	{
		glm::vec3 Origin;
		glm::vec3 Direction;
	};

	inline float IntersectPlane(glm::vec3 planeNormal, glm::vec3 rayDirection, glm::vec3 rayOrigin)
	{
		return -glm::dot(planeNormal, rayOrigin) / glm::dot(rayDirection, planeNormal);
	}

	inline float IntersectPlane(const Plane& plane, const Ray& ray)
	{
		return (-plane.Offset - glm::dot(plane.Normal, ray.Origin)) / glm::dot(plane.Normal, ray.Direction);
	}

	struct AABB
	{
		constexpr AABB()
			: Min(0.0f), Max(0.0f) {}

		constexpr AABB(glm::vec3 min, glm::vec3 max)
			: Min(min), Max(max) {}

		inline void GetCorners(glm::vec3* corners) const
		{
			corners[0] = Min;
			corners[1] = glm::vec3(Min.x, Min.y, Max.z);
			corners[2] = glm::vec3(Max.x, Min.y, Min.z);
			corners[3] = glm::vec3(Max.x, Min.y, Max.z);

			corners[4] = glm::vec3(Min.x, Max.y, Min.z);
			corners[5] = glm::vec3(Min.x, Max.y, Max.z);
			corners[6] = glm::vec3(Max.x, Max.y, Min.z);
			corners[7] = Max;
		}

		inline glm::vec3 GetCenter() const { return (Max + Min) / 2.0f; }
		inline glm::vec3 GetSize() const { return Max - Min; }
		inline glm::vec3 GetExtents() const { return Max - GetCenter(); }

		// From https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
		inline bool IntersectsPlane(const Plane& plane) const
		{
			glm::vec3 center = GetCenter();
			glm::vec3 extents = Max - center;

			float projectedDistance = glm::dot(glm::abs(plane.Normal), extents);
			return plane.Distance(center) <= projectedDistance;
		}

		inline bool IntersectsOrInFrontOfPlane(const Plane& plane) const
		{
			glm::vec3 center = GetCenter();
			glm::vec3 extents = Max - center;

			float projectedDistance = glm::dot(glm::abs(plane.Normal), extents);
			return -projectedDistance <= plane.SignedDistance(center);
		}

		inline AABB Transformed(const glm::mat4& transform) const
		{
			AABB transformed;

			std::array<glm::vec3, 8> corners;
			GetCorners(corners.data());

			for (size_t i = 0; i < 8; i++)
			{
				corners[i] = (glm::vec3)(transform * glm::vec4(corners[i], 1.0f));
			}

			transformed.Min = corners[0];
			transformed.Max = corners[0];

			for (size_t i = 1; i < 8; i++)
			{
				transformed.Min = glm::min(transformed.Min, corners[i]);
				transformed.Max = glm::max(transformed.Max, corners[i]);
			}

			return transformed;
		}

		glm::vec3 Min;
		glm::vec3 Max;
	};
}