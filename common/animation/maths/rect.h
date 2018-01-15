
#ifndef MATHS_RECT_H_
#define MATHS_RECT_H_
#include "common/animation/platform.h"
namespace egal
{
	namespace math
	{

		// Defines a rectangle by the integer coordinates of its lower-left and
		// width-height.
		struct RectInt
		{
			// Constructs a uninitialized rectangle.
			RectInt()
			{}

			// Constructs a rectangle with the specified arguments.
			RectInt(int _left, int _bottom, int _width, int _height)
				: left(_left), bottom(_bottom), width(_width), height(_height)
			{}

			// Tests whether _x and _y coordinates are within rectangle bounds.
			bool is_inside(int _x, int _y) const
			{
				return _x >= left && _x < left + width && _y >= bottom &&
					_y < bottom + height;
			}

			// Gets the rectangle x coordinate of the right rectangle side.
			int right() const
			{
				return left + width;
			}

			// Gets the rectangle y coordinate of the top rectangle side.
			int top() const
			{
				return bottom + height;
			}

			// Specifies the x-coordinate of the lower side.
			int left;
			// Specifies the x-coordinate of the left side.
			int bottom;
			// Specifies the width of the rectangle.
			int width;
			// Specifies the height of the rectangle..
			int height;
		};

		// Defines a rectangle by the floating point coordinates of its lower-left
		// and width-height.
		struct RectFloat
		{
			// Constructs a uninitialized rectangle.
			RectFloat()
			{}

			// Constructs a rectangle with the specified arguments.
			RectFloat(float _left, float _bottom, float _width, float _height)
				: left(_left), bottom(_bottom), width(_width), height(_height)
			{}

			// Tests whether _x and _y coordinates are within rectangle bounds
			bool is_inside(float _x, float _y) const
			{
				return _x >= left && _x < left + width && _y >= bottom &&
					_y < bottom + height;
			}

			// Gets the rectangle x coordinate of the right rectangle side.
			float right() const
			{
				return left + width;
			}

			// Gets the rectangle y coordinate of the top rectangle side.
			float top() const
			{
				return bottom + height;
			}

			// Specifies the x-coordinate of the lower side.
			float left;
			// Specifies the x-coordinate of the left side.
			float bottom;
			// Specifies the width of the rectangle.
			float width;
			// Specifies the height of the rectangle.
			float height;
		};
	}  // namespace math
}  // namespace egal
#endif  // MATHS_RECT_H_
