#ifndef _log_h_
#define _log_h_
#pragma once

#include "common/egal-d.h"

namespace egal
{
	class LogUI
	{
	public:
		explicit LogUI(IAllocator& allocator);
		~LogUI();

		void onGUI();
		void update(float time_delta);
		int addNotification(const char* text);
		void setNotificationTime(int uid, float time);
		int getUnreadErrorCount() const;

	public:
		bool m_is_open;

	private:
		enum Type
		{
			LOG_INFO,
			LOG_WARNING,
			LOG_ERROR,

			LOG_COUNT
		};

		struct Notification
		{
			explicit Notification(IAllocator& alloc)
				: message(alloc)
			{
			}
			float time;
			int uid;
			String message;
		};

	private:
		void onLog(const int32_t logType, const char* message);

		void onInfo(const char* system, const char* message);
		void onWarning(const char* system, const char* message);
		void onError(const char* system, const char* message);
		void push(Type type, const char* message);
		void showNotifications();

	private:
		struct Message
		{
			Message(IAllocator& allocator)
				: text(allocator)
			{}

			String text;
			Type type;
		};

		IAllocator& m_allocator;
		TArrary<Message> m_messages;
		TArrary<Notification> m_notifications;
		int m_new_message_count[LOG_COUNT];
		e_uint8 m_level_filter;
		int m_last_uid;
		bool m_move_notifications_to_front;
		bool m_are_notifications_hovered;
		MT::SpinMutex m_guard;
	};


}
#endif
