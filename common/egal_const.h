#ifndef _egal_const_h_
#define _egal_const_h_

#include "common/struct.h"
#include "common/input/reflection.h"

namespace egal
{
	static			e_uint8	  DEFAULT_COMMAND_BUFFER = 0;

	static const e_uint32	  SERIALIZED_ENGINE_MAGIC = 0x5f4c454e;
	static const e_float	  DEFAULT_ALPHA_REF_VALUE = 0.3f;

	static const ResourceType MATERIAL_TYPE("material");
	static const ResourceType TEXTURE_TYPE("texture");
	static const ResourceType ENTITY_TYPE("entity");

	static const ResourceType SHADER_TYPE("shader");
	static const ResourceType SHADER_BINARY_TYPE("shader_binary");

	/***********************************************************************************************/
	/***********************************************************************************************/
	/***********************************************************************************************/
	static const ComponentType ENTITY_INSTANCE_TYPE			  = Reflection::getComponentType("render_able");
	static const ComponentType DECAL_TYPE					  = Reflection::getComponentType("decal");

	static const ComponentType POINT_LIGHT_TYPE				  = Reflection::getComponentType("point_light");
	static const ComponentType GLOBAL_LIGHT_TYPE			  = Reflection::getComponentType("global_light");

	static const ComponentType PARTICLE_EMITTER_TYPE		  = Reflection::getComponentType("particle_emitter");
	static const ComponentType SCRIPTED_PARTICLE_EMITTER_TYPE = Reflection::getComponentType("scripted_particle_emitter");
	
	static const ComponentType CAMERA_TYPE					  = Reflection::getComponentType("camera");
	static const ComponentType TERRAIN_TYPE					  = Reflection::getComponentType("terrain");
	static const ComponentType BONE_ATTACHMENT_TYPE			  = Reflection::getComponentType("bone_attachment");
	static const ComponentType ENVIRONMENT_PROBE_TYPE		  = Reflection::getComponentType("environment_probe");
	

	
}

#endif
