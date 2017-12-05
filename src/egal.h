#ifndef _egal_h_
#define _egal_h_
#pragma once

#include "egal_type.h"
#include "egal_math.h"
#include "egal_setting.h"
#include "egal_alloc.h"

//********************************************************************************
#define SAFE_RENDER(p)       { if (p) { p->Render(); } }
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#define SAFE_FREE(p)		 { if (p) { free(p);     (p)=NULL; } }

//*********************************************************************************



#endif
