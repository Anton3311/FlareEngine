#pragma once

#include "Flare/Math/Math.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flare::Math::SIMD
{
	inline __m128 MultiplyMatrix4x4ByVector4(const __m128& vector, const __m128* matrix)
	{
		__m128 resultVector = _mm_setzero_ps();
		for (int32_t i = 0; i < 4; i++)
		{
			__m128 multiplier = _mm_load_ps1(&((const float*)&vector)[i]);
			resultVector = _mm_add_ps(resultVector, _mm_mul_ps(matrix[i], multiplier));
		}

		return resultVector;
	}

	inline void MultiplyMatrixByVector(const glm::vec4& vector, glm::vec4* out, const glm::mat4& matrix)
	{
		__m128 resultVector = _mm_setzero_ps();
		for (int32_t i = 0; i < 4; i++)
		{
			__m128 column = _mm_load_ps(glm::value_ptr(matrix[i]));
			__m128 multiplier = _mm_load_ps1(&vector[i]);

			resultVector = _mm_add_ps(resultVector, _mm_mul_ps(column, multiplier));
		}

		_mm_store_ps(glm::value_ptr(*out), resultVector);
	}

	inline __m128 Abs(__m128 value)
	{
		// Mask used for computing absolute value of single precision float
		// Masks everything except the sign bit
		const float floatMask = -0.0f;
		__m128 mask = _mm_load_ps1(&floatMask);
		return _mm_andnot_ps(mask, value);
	}

	inline float Dot(__m128 a, __m128 b)
	{
		a = _mm_mul_ps(a, b);
		a = _mm_hadd_ps(a, a);
		a = _mm_hadd_ps(a, a);

		return *(float*)&a;
	}

	inline AABB TransformAABB(const AABB& aabb, const glm::mat4& transform)
	{
		__m128 min = _mm_loadu_ps(glm::value_ptr(glm::vec4(aabb.Min, 1.0f)));
		__m128 max = _mm_loadu_ps(glm::value_ptr(glm::vec4(aabb.Max, 1.0f)));

		float scale = 0.5f;
		__m128 halfScale = _mm_load_ps1(&scale);
		__m128 center = _mm_mul_ps(_mm_add_ps(min, max), halfScale);
		__m128 newCenter = MultiplyMatrix4x4ByVector4(center, (const __m128*)glm::value_ptr(transform));
		__m128 extent = _mm_mul_ps(_mm_sub_ps(max, min), halfScale);

		// Transform extent
		__m128 x = Abs(_mm_mul_ps(extent, _mm_loadu_ps(glm::value_ptr(transform[0]))));
		__m128 y = Abs(_mm_mul_ps(extent, _mm_loadu_ps(glm::value_ptr(transform[1]))));
		__m128 z = Abs(_mm_mul_ps(extent, _mm_loadu_ps(glm::value_ptr(transform[2]))));

		// Calculate distances to planes with normals (1, 0, 0), (0, 1, 0) and (0, 0, 1)
		// and store each value in x, y and z components of a vector
		// which forms an extent vector in original space
		__m128 newExtent = _mm_add_ps(x, _mm_add_ps(y, z));
		__m128 newMin = _mm_sub_ps(newCenter, newExtent);
		__m128 newMax = _mm_add_ps(newCenter, newExtent);

		glm::vec4* newMinResult = (glm::vec4*)&newMin;
		glm::vec4* newMaxResult = (glm::vec4*)&newMax;

		return AABB(*newMinResult, *newMaxResult);
	}
}
