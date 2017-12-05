#ifndef _CLIENT_H__
#define _CLIENT_H__
#pragma once
#include "framework_listener.h"
#include "framework.hpp"

class Client
{
public:
	Client();
	~Client();

	void init(HWND hwnd);
	void resize(int width, int height);
	void update(float fEspl);
	void exit();

private:
	framework* m_frame_work;
};

#endif


