#ifndef __egal_math_h__
#define __egal_math_h__
#pragma once

#include "egal_type.h"

#include "mathfu/constants.h"
#include "mathfu/rect.h"

namespace egal
{
	typedef mathfu::Vector<e_float, 2>		float2;
	typedef mathfu::Vector<e_float, 3>		float3;
	typedef mathfu::Vector<e_float, 4>		float4;

	typedef mathfu::Matrix<e_float, 2, 2>	float2x2;
	typedef mathfu::Matrix<e_float, 3, 3>	float3x3;
	typedef mathfu::Matrix<e_float, 4, 4>	float4x4;

	typedef mathfu::Quaternion<e_float>		Quaternion;

	typedef mathfu::Rect<e_float>			Rect;

}

#endif
