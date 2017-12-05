#include "stdafx.h"
#include "Client.h"

Client::Client()
{
	m_frame_work = new framework();
}

Client::~Client()
{
	exit();
}

void Client::init(HWND hwnd)
{
	m_frame_work->init(hwnd);


}

void Client::resize(int width, int height)
{
	m_frame_work->resize(width, height);
}

void Client::update(float fEspl)
{
	m_frame_work->update(fEspl);
}

void Client::exit()
{
	if (m_frame_work)
	{
		delete m_frame_work;
		m_frame_work = NULL;
	}
}