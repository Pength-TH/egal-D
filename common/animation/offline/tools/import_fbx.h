#ifndef _import_fbx_h_
#define _import_fbx_h_

#include "common/egal-d.h"

namespace egal
{
	enum endian
	{
		native,
		little,
		big,
	};

	//导入选项
	struct ImportOption
	{
		String file_path_name;
		String out_mesh_path_name;
		String out_skeleton_path_name;
		String out_animation_path_name;
		
		endian m_endian;
		e_bool m_raw;

		/**animation*/
		e_bool m_additive; //false
		e_float m_hierarchical; //0.001;
		e_bool m_optimize; //true
		e_float m_rotation; //0.00174533
		e_float m_sampling_rate; //0
		e_float m_scale;// 0.001
		e_float m_translation;// 0.001

	};

	e_void import_fbx(const char* file_path, ImportOption& option);

}
#endif
