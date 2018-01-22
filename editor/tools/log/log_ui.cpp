#include "log_ui.h"
#include "tools/base/platform_interface.h"
#include "imgui/bgfx_imgui.h"

namespace egal
{
	LogUI::LogUI(IAllocator& allocator)
		: m_allocator(allocator)
		, m_messages(allocator)
		, m_level_filter(15)
		, m_notifications(allocator)
		, m_last_uid(1)
		, m_guard(false)
		, m_is_open(false)
		, m_are_notifications_hovered(false)
		, m_move_notifications_to_front(false)
	{
		log_call_back().bind<LogUI, &LogUI::onLog>(this);

		for (int i = 0; i < LOG_COUNT; ++i)
		{
			m_new_message_count[i] = 0;
		}
	}


	LogUI::~LogUI()
	{
		log_call_back().unbind<LogUI, &LogUI::onLog>(this);
	}


	void LogUI::setNotificationTime(int uid, float time)
	{
		for (auto& notif : m_notifications)
		{
			if (notif.uid == uid)
			{
				notif.time = time;
				break;
			}
		}
	}


	int LogUI::addNotification(const char* text)
	{
		m_move_notifications_to_front = true;
		if (!m_notifications.empty() && m_notifications.back().message == text) return -1;
		auto& notif = m_notifications.emplace(m_allocator);
		notif.time = 10.0f;
		notif.message = text;
		notif.uid = ++m_last_uid;
		return notif.uid;
	}

	void LogUI::push(Type type, const char* message)
	{
		MT::SpinLock lock(m_guard);
		++m_new_message_count[type];
		Message& msg = m_messages.emplace(m_allocator);
		msg.text = message;
		msg.type = type;

		if (type == LOG_ERROR)
		{
			addNotification(message);
		}
	}


	void LogUI::onLog(const int32_t logType, const char* message)
	{
		Type type = (Type)logType;
		switch (type)
		{
		case LOG_INFO:
			onInfo("Engine", message);
			break;
		case LOG_WARNING:
			onWarning("Engine", message);
			break;
		case LOG_ERROR:
			onError("Engine", message);
			break;
		default:
			break;
		}
	}

	void LogUI::onInfo(const char* system, const char* message)
	{
		push(LOG_INFO, message);
	}


	void LogUI::onWarning(const char* system, const char* message)
	{
		push(LOG_WARNING, message);
	}


	void LogUI::onError(const char* system, const char* message)
	{
		push(LOG_ERROR, message);
	}


	void fillLabel(char* output, int max_size, const char* label, int count)
	{
		StringUnitl::copyString(output, max_size, label);
		StringUnitl::catString(output, max_size, "(");
		int len = StringUnitl::stringLength(output);
		StringUnitl::toCString(count, output + len, max_size - len);
		StringUnitl::catString(output, max_size, ")###");
		StringUnitl::catString(output, max_size, label);
	}


	void LogUI::showNotifications()
	{
		m_are_notifications_hovered = false;
		if (m_notifications.empty()) return;

		ImGui::SetNextWindowPos(ImVec2(10, 30));
		bool open;
		if (!ImGui::Begin("Notifications",
			&open,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_ShowBorders))
		{
			ImGui::End();
			return;
		}

		m_are_notifications_hovered = ImGui::IsWindowHovered();

		if (ImGui::Button("Close")) m_notifications.clear();

		if (m_move_notifications_to_front) 
			ImGui::BringToFront();
		m_move_notifications_to_front = false;
		for (int i = 0; i < m_notifications.size(); ++i)
		{
			if (i > 0) ImGui::Separator();
			ImGui::Text("%s", m_notifications[i].message.c_str());
		}
		ImGui::End();
	}


	void LogUI::update(float time_delta)
	{
		if (m_are_notifications_hovered) return;

		for (int i = 0; i < m_notifications.size(); ++i)
		{
			m_notifications[i].time -= time_delta;

			if (m_notifications[i].time < 0)
			{
				m_notifications.erase(i);
				--i;
			}
		}
	}


	int LogUI::getUnreadErrorCount() const
	{
		return m_new_message_count[LOG_ERROR];
	}


	void LogUI::onGUI()
	{
		MT::SpinLock lock(m_guard);
		showNotifications();

		if (ImGui::Begin("Log", &m_is_open))
		{
			const char* labels[] = { "Info", "Warning", "Error" };
			for (int i = 0; i < TlengthOf(labels); ++i)
			{
				char label[40];
				fillLabel(label, sizeof(label), labels[i], m_new_message_count[i]);
				if (i > 0) ImGui::SameLine();
				bool b = m_level_filter & (1 << i);
				if (ImGui::Checkbox(label, &b))
				{
					if (b) m_level_filter |= 1 << i;
					else m_level_filter &= ~(1 << i);
					m_new_message_count[i] = 0;
				}
			}

			ImGui::SameLine();
			char filter[128] = "";
			ImGui::LabellessInputText("Filter", filter, sizeof(filter));
			int len = 0;

			if (ImGui::BeginChild("log_messages", ImVec2(0, 0), true))
			{
				for (int i = 0; i < m_messages.size(); ++i)
				{
					if ((m_level_filter & (1 << m_messages[i].type)) == 0) continue;
					const char* msg = m_messages[i].text.c_str();
					if (filter[0] == '\0' || strstr(msg, filter) != nullptr)
					{
						ImGui::TextUnformatted(msg);
					}
				}
			}
			ImGui::EndChild();
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) ImGui::OpenPopup("Context");
			if (ImGui::BeginPopup("Context"))
			{
				if (ImGui::Selectable("Copy"))
				{
					for (int i = 0; i < m_messages.size(); ++i)
					{
						const char* msg = m_messages[i].text.c_str();
						if (filter[0] == '\0' || strstr(msg, filter) != nullptr)
						{
							len += StringUnitl::stringLength(msg);
						}
					}

					if (len > 0)
					{
						char* mem = (char*)m_allocator.allocate(len);
						mem[0] = '\0';
						for (int i = 0; i < m_messages.size(); ++i)
						{
							const char* msg = m_messages[i].text.c_str();
							if (filter[0] == '\0' || strstr(msg, filter) != nullptr)
							{
								StringUnitl::catString(mem, len, msg);
								StringUnitl::catString(mem, len, "\n");
							}
						}

						PlatformInterface::copyToClipboard(mem);
						m_allocator.deallocate(mem);
					}
				}
				if (ImGui::Selectable("Clear"))
				{
					TArrary<Message> filtered_messages(m_allocator);
					for (int i = 0; i < m_messages.size(); ++i)
					{
						if ((m_level_filter & (1 << m_messages[i].type)) != 0) continue;
						filtered_messages.emplace(m_messages[i]);
						m_new_message_count[m_messages[i].type] = 0;
					}
					m_messages.swap(filtered_messages);
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}


} // namespace
