#ifndef __math_const_h__
#define __math_const_h__
#pragma once

#include "common/type.h"
namespace egal
{
	/** const */
	namespace Math
	{
		static const e_float C_2Pi = 6.283185307179586476925286766559f;
		static const e_float C_Pi = 3.1415926535897932384626433832795f;
		static const e_float C_Pi_2 = 1.5707963267948966192313216916398f;
		static const e_float C_Pi_4 = 0.78539816339744830961566084581988f;
		static const e_float C_Sqrt3 = 1.7320508075688772935274463415059f;
		static const e_float C_Sqrt3_2 = 0.86602540378443864676372317075294f;
		static const e_float C_Sqrt2 = 1.4142135623730950488016887242097f;
		static const e_float C_Sqrt2_2 = 0.70710678118654752440084436210485f;

		// Angle unit conversion constants.
		static const e_float C_DegreeToRadian = C_Pi / 180.f;
		static const e_float C_RadianToDegree = 180.f / C_Pi;

		// Defines the square normalization tolerance value.
		static const e_float C_NormalizationToleranceSq = 1e-6f;
		static const e_float C_NormalizationToleranceEstSq = 2e-3f;

		// Defines the square orthogonalisation tolerance value.
		static const e_float C_OrthogonalisationToleranceSq = 1e-16f;
	}

}

#endif
