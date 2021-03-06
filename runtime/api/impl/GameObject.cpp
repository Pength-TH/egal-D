#include "runtime/api/impl/GameObject.h"
#include "common/egal-d.h"

namespace SeedEngine
{
	GameObject::GameObject()
	{
		m_nRayQueryLevel = ERQL_UNKNOWN;
		m_Data = NULL;
		m_eenum_game_object_type = EGOT_DEFAULT;
	}

	GameObject::~GameObject()
	{
		m_nRayQueryLevel = ERQL_UNKNOWN;
		m_Data = NULL;
		m_eenum_game_object_type = EGOT_DEFAULT;
	}

	VOID GameObject::Destroy(VOID)
	{
		g_SeedApplication->destroy_game_object(this);
	}
}