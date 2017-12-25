#ifndef _resource_define_h_
#define _resource_define_h_
#pragma once

namespace egal
{

/**BGFX define*/
#define BGFX
#ifdef BGFX
	namespace
	{
		#include <bgfx/bgfx.h>
		
		#define INVALID_HANDLE BGFX_INVALID_HANDLE
		//-------------------------------------------------------------------------------------
		typedef bgfx::ProgramHandle			ProgramHandle;
		typedef bgfx::ShaderHandle			ShaderHandle;
		typedef bgfx::UniformHandle			UniformHandle;
		typedef bgfx::TextureHandle			TextureHandle;
		typedef bgfx::VertexDecl			VertexDecl;
		typedef bgfx::VertexBufferHandle	VertexBufferHandle;
		typedef bgfx::IndexBufferHandle		IndexBufferHandle;
		typedef bgfx::InstanceDataBuffer    InstanceDataBuffer;




	}
#endif


}
#endif
