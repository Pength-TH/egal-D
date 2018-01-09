#ifndef __FRAMEWORK_HPP__
#define __FRAMEWORK_HPP__

#include <OISEvents.h>
#include <OISInputManager.h>
#include <OISKeyboard.h>
#include <OISMouse.h>
#include <assert.h>

#include <windows.h>

#include "framework_listener.h"
#include "runtime/IEngine.h"

typedef std::vector<framework_listener*> FrameWorkListenerList;

class framework : OIS::KeyListener, OIS::MouseListener
{
public:
	framework();
	virtual ~framework();

	bool init(HWND wndTitle, OIS::KeyListener *pKeyListener = 0, OIS::MouseListener *pMouseListener = 0);
	void update(double timeSinceLastFrame);
	void resize(int width, int height);

	bool keyPressed(const OIS::KeyEvent &keyEventRef);
	bool keyReleased(const OIS::KeyEvent &keyEventRef);

	bool mouseMoved(const OIS::MouseEvent &evt);
	bool mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id);

	virtual void addFrameWorkListener(framework_listener* newListener);
	virtual void removeFrameWorkListener(framework_listener* delListener);
private:
	OIS::InputManager*			m_pInputMgr;
	OIS::Keyboard*				m_pKeyboard;
	OIS::Mouse*					m_pMouse;

	FrameWorkListenerList       m_frame_work_listener;

	egal::IEngine* app;
};
#endif
