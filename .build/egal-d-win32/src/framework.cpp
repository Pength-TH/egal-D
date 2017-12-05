#include "stdafx.h"
#include "framework.hpp"
#include <sstream>

bool g_key_board[KC_BOARD_END];
bool g_mouse_board[MB_END];

typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char> > _StringStreamBase;
framework::framework()
{
    m_pInputMgr			= 0;
    m_pKeyboard			= 0;
    m_pMouse			= 0;

	for (int i = 0; i < KC_BOARD_END; i++)
		g_key_board[i] = false;
	for (int i = 0; i < MB_END; i++)
		g_mouse_board[i] = false;
}

framework::~framework()
{
	for (int i = 0; i < KC_BOARD_END; i++)
		g_key_board[i] = false;
	for (int i = 0; i < MB_END; i++)
		g_mouse_board[i] = false;

	if(m_pInputMgr)	
		OIS::InputManager::destroyInputSystem(m_pInputMgr);
}

bool framework::init(HWND wnd, OIS::KeyListener *pKeyListener, OIS::MouseListener *pMouseListener)
{
	OIS::ParamList paramList;
	_StringStreamBase windowHndStr;
	windowHndStr << "0x" <<wnd;
	paramList.insert(std::make_pair(std::string("WINDOW"),		windowHndStr.str().c_str()));
	paramList.insert(std::make_pair(std::string("w32_mouse"),   std::string("DISCL_FOREGROUND")));
	paramList.insert(std::make_pair(std::string("w32_mouse"),	std::string("DISCL_NONEXCLUSIVE")));
	paramList.insert(std::make_pair(std::string("w32_keyboard"),std::string("DISCL_FOREGROUND")));
	paramList.insert(std::make_pair(std::string("w32_keyboard"),std::string("DISCL_NONEXCLUSIVE")));
	m_pInputMgr = OIS::InputManager::createInputSystem(paramList);

    m_pKeyboard = static_cast<OIS::Keyboard*>(m_pInputMgr->createInputObject(OIS::OISKeyboard, true));
    m_pMouse	= static_cast<OIS::Mouse*>(m_pInputMgr->createInputObject(OIS::OISMouse, true));

    if(pKeyListener == 0)
        m_pKeyboard->setEventCallback(this);
    else
        m_pKeyboard->setEventCallback(pKeyListener);

    if(pMouseListener == 0)
        m_pMouse->setEventCallback(this);
    else
        m_pMouse->setEventCallback(pMouseListener);

    return true;
}

bool framework::keyPressed(const OIS::KeyEvent &keyEventRef)
{
	g_key_board[keyEventRef.key] = true;
	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		(*i)->on_key_pressed(keyEventRef.text, (KeyCode)keyEventRef.key);
	}
    return true;
}

bool framework::keyReleased(const OIS::KeyEvent &keyEventRef)
{
	g_key_board[keyEventRef.key] = false;
	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		(*i)->on_key_released(keyEventRef.text, (KeyCode)keyEventRef.key);
	}
    return true;
}

bool framework::mouseMoved(const OIS::MouseEvent &evt)
{
	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		(*i)->on_mouse_moved(evt.state.X.abs, evt.state.Y.abs, evt.state.Z.rel);
	}
    return true;
}

bool framework::mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
{
	g_mouse_board[id] = true;
	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		(*i)->on_mouse_pressed((MouseButtonID)id, evt.state.Z.rel);
	}
    return true;
}

bool framework::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
{
	g_mouse_board[id] = false;
	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		(*i)->on_mouse_released((MouseButtonID)id, evt.state.Z.rel);
	}
    return true;
}

void framework::addFrameWorkListener(framework_listener* newListener)
{
	m_frame_work_listener.push_back(newListener);
}

void framework::removeFrameWorkListener(framework_listener* delListener)
{
	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		if (*i == delListener)
		{
			m_frame_work_listener.erase(i);
			break;
		}
	}
}

void framework::update(double timeSinceLastFrame)
{
	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		(*i)->update(timeSinceLastFrame);
	}

	m_pKeyboard->capture();
	m_pMouse->capture();
}

void framework::resize(int width, int height)
{
	if (m_pMouse)
	{
		m_pMouse->getMouseState().height = height;
		m_pMouse->getMouseState().width = width;
	}

	FrameWorkListenerList::iterator i, iend;
	iend = m_frame_work_listener.end();
	for (i = m_frame_work_listener.begin(); i != iend; ++i)
	{
		(*i)->resize(width, height);
	}
}
