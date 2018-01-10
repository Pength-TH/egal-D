#ifndef _egal_const_h_
#define _egal_const_h_

#include "common/struct.h"
#include "common/input/reflection.h"

namespace egal
{
	static			e_bool	  g_is_crash_reporting_enabled = true;
	static			e_uint8	  DEFAULT_COMMAND_BUFFER = 0;
	
	static const e_int32	  MAX_TEXTURE_COUNT		  = 16;
	static const e_uint32	  SERIALIZED_ENGINE_MAGIC = 0x5f4c454e;
	static const e_float	  DEFAULT_ALPHA_REF_VALUE = 0.3f;
	static const e_int32	  RESERVED_ENTITIES_COUNT = 5000;

	static const e_float	  SHADOW_CAM_NEAR = 50.0f;
	static const e_float	  SHADOW_CAM_FAR = 5000.0f;
	static const e_int32	  MAX_SLOT_LENGTH = 30;
	static const e_int32	  MAX_INSTANCE_COUNT = 32;

	/** Resource Type */
	static const ResourceType RESOURCE_MATERIAL_TYPE("material");
	static const ResourceType RESOURCE_TEXTURE_TYPE("texture");
	static const ResourceType RESOURCE_ENTITY_TYPE("entity");
	static const ResourceType RESOURCE_SHADER_TYPE("shader");
	static const ResourceType RESOURCE_SHADER_BINARY_TYPE("shader_binary");

	/** Component Type */
	static const ComponentType COMPONENT_CAMERA_TYPE					= Reflection::getComponentType("camera");
	static const ComponentType COMPONENT_POINT_LIGHT_TYPE				= Reflection::getComponentType("point_light");
	static const ComponentType COMPONENT_GLOBAL_LIGHT_TYPE				= Reflection::getComponentType("global_light");
	static const ComponentType COMPONENT_BONE_ATTACHMENT_TYPE			= Reflection::getComponentType("bone_attachment");
	static const ComponentType COMPONENT_ENVIRONMENT_PROBE_TYPE			= Reflection::getComponentType("environment_probe");
	static const ComponentType COMPONENT_ENTITY_INSTANCE_TYPE			= Reflection::getComponentType("render_able");
	static const ComponentType COMPONENT_DECAL_TYPE						= Reflection::getComponentType("decal");

	static const ComponentType COMPONENT_PARTICLE_EMITTER_TYPE			= Reflection::getComponentType("particle_emitter");
	static const ComponentType COMPONENT_SCRIPTED_PARTICLE_EMITTER_TYPE = Reflection::getComponentType("scripted_particle_emitter");
	static const ComponentType COMPONENT_TERRAIN_TYPE					= Reflection::getComponentType("terrain");



}

#endif
