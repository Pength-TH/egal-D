
static const float NODE_SLOT_RADIUS = 4.0f;

namespace ImGui
{
	ImString::ImString()
		: Ptr(NULL)
	{
	}

	ImString::ImString(const ImString& rhs)
		: Ptr(NULL)
	{
		if (NULL != rhs.Ptr
			&& 0 != strcmp(rhs.Ptr, ""))
		{
			Ptr = ImStrdup(rhs.Ptr);
		}
	}

	ImString::ImString(const char* rhs)
		: Ptr(NULL)
	{
		if (NULL != rhs
			&& 0 != strcmp(rhs, ""))
		{
			Ptr = ImStrdup(rhs);
		}
	}

	ImString::~ImString()
	{
		Clear();
	}

	ImString& ImString::operator=(const ImString& rhs)
	{
		if (this != &rhs)
		{
			*this = rhs.Ptr;
		}

		return *this;
	}

	ImString& ImString::operator=(const char* rhs)
	{
		if (Ptr != rhs)
		{
			Clear();

			if (NULL != rhs
				&& 0 != strcmp(rhs, ""))
			{
				Ptr = ImStrdup(rhs);
			}
		}

		return *this;
	}

	void ImString::Clear()
	{
		if (NULL != Ptr)
		{
			MemFree(Ptr);
			Ptr = NULL;
		}
	}

	bool ImString::IsEmpty() const
	{
		return NULL == Ptr;
	}


	bool CheckboxEx(const char* label, bool* v)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		ImVec2 dummy_size = ImVec2(CalcItemWidth() - label_size.y - style.FramePadding.y * 2 - style.ItemSpacing.x, label_size.y);
		Dummy(dummy_size);
		SameLine();
		const ImRect check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(label_size.y + style.FramePadding.y * 2, label_size.y + style.FramePadding.y * 2)); // We want a square shape to we use Y twice
		ItemSize(check_bb, style.FramePadding.y);

		ImRect total_bb = check_bb;
		if (label_size.x > 0)
			SameLine(0, style.ItemInnerSpacing.x);
		const ImRect text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + label_size);
		if (label_size.x > 0)
		{
			ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
			total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
		}

		if (!ItemAdd(total_bb, &id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
			*v = !(*v);

		RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
		if (*v)
		{
			const float check_sz = ImMin(check_bb.GetWidth(), check_bb.GetHeight());
			const float pad = ImMax(1.0f, (float)(int)(check_sz / 6.0f));
			//RenderCheckMark(check_bb.Min + ImVec2(pad, pad), GetColorU32(ImGuiCol_CheckMark), check_bb.GetWidth() - pad*2.0f);
			RenderCheckMark(check_bb.Min + ImVec2(pad, pad), GetColorU32(ImGuiCol_CheckMark));
		}

		if (g.LogEnabled)
			LogRenderedText(text_bb.Min, *v ? "[x]" : "[ ]");
		if (label_size.x > 0.0f)
			RenderText(text_bb.Min, label);

		return pressed;
	}


	bool ToolbarButton(ImTextureID texture, const ImVec4& bg_color, const char* tooltip)
	{
		auto frame_padding = ImGui::GetStyle().FramePadding;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, frame_padding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

		bool ret = false;
		ImGui::SameLine();
		ImVec4 tint_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
		if (ImGui::ImageButton(texture, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, bg_color, tint_color))
		{
			ret = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
		return ret;
	}



	bool BeginToolbar(const char* str_id, ImVec2 screen_pos, ImVec2 size)
	{
		bool is_global = GImGui->CurrentWindowStack.Size == 1;
		SetNextWindowPos(screen_pos);
		ImVec2 frame_padding = GetStyle().FramePadding;
		PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		PushStyleVar(ImGuiStyleVar_WindowPadding, frame_padding);
		PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		float padding = frame_padding.y * 2;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
		if (size.x == 0) size.x = GetContentRegionAvailWidth();
		SetNextWindowSize(size);

		bool ret = is_global ? Begin(str_id, nullptr, flags) : BeginChild(str_id, size, false, flags);
		PopStyleVar(3);

		return ret;
	}


	void EndToolbar()
	{
		auto frame_padding = ImGui::GetStyle().FramePadding;
		PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		PushStyleVar(ImGuiStyleVar_WindowPadding, frame_padding);
		PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImVec2 pos = GetWindowPos();
		ImVec2 size = GetWindowSize();
		if (GImGui->CurrentWindowStack.Size == 2) End(); else EndChild();
		PopStyleVar(3);
		ImGuiWindow* win = GetCurrentWindowRead();
		if (GImGui->CurrentWindowStack.Size > 1) SetCursorScreenPos(pos + ImVec2(0, size.y + GetStyle().FramePadding.y * 2));
	}


	ImVec2 GetOsImePosRequest()
	{
		return ImGui::GetCurrentContext()->OsImePosRequest;
	}


	void ResetActiveID()
	{
		SetActiveID(0, nullptr);
	}



	static inline bool IsWindowContentHoverableEx(ImGuiWindow* window)
	{
		// An active popup disable hovering on other windows (apart from its own children)
		// FIXME-OPT: This could be cached/stored within the window.
		ImGuiContext& g = *GImGui;
		if (g.NavWindow)
			if (ImGuiWindow* focused_root_window = g.NavWindow->RootWindow)
				if (focused_root_window->WasActive && focused_root_window != window->RootWindow)
				{
					// For the purpose of those flags we differentiate "standard popup" from "modal popup"
					// NB: The order of those two tests is important because Modal windows are also Popups.
					if (focused_root_window->Flags & ImGuiWindowFlags_Modal)
						return false;
					if ((focused_root_window->Flags & ImGuiWindowFlags_Popup) )
						return false;
				}

		return true;
	}

	int PlotHistogramEx(const char* label,
		float(*values_getter)(void* data, int idx),
		void* data,
		int values_count,
		int values_offset,
		const char* overlay_text,
		float scale_min,
		float scale_max,
		ImVec2 graph_size,
		int selected_index)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return -1;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		if (graph_size.x == 0.0f) graph_size.x = CalcItemWidth() + (style.FramePadding.x * 2);
		if (graph_size.y == 0.0f) graph_size.y = label_size.y + (style.FramePadding.y * 2);

		const ImRect frame_bb(
			window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
		const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
		const ImRect total_bb(frame_bb.Min,
			frame_bb.Max +
			ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, 0)) return -1;

		// Determine scale from values if not specified
		if (scale_min == FLT_MAX || scale_max == FLT_MAX)
		{
			float v_min = FLT_MAX;
			float v_max = -FLT_MAX;
			for (int i = 0; i < values_count; i++)
			{
				const float v = values_getter(data, i);
				v_min = ImMin(v_min, v);
				v_max = ImMax(v_max, v);
			}
			if (scale_min == FLT_MAX) scale_min = v_min;
			if (scale_max == FLT_MAX) scale_max = v_max;
		}

		RenderFrame(
			frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

		int res_w = ImMin((int)graph_size.x, values_count);

		// Tooltip on hover
		int v_hovered = -1;
		ImGuiID id = 0;
		if (IsHovered(inner_bb, id))
		{
			const float t = ImClamp(
				(g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
			const int v_idx = (int)(t * (values_count + 0));
			IM_ASSERT(v_idx >= 0 && v_idx < values_count);

			const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
			SetTooltip("%d: %8.4g", v_idx, v0);
			v_hovered = v_idx;
		}

		const float t_step = 1.0f / (float)res_w;

		float v0 = values_getter(data, (0 + values_offset) % values_count);
		float t0 = 0.0f;
		ImVec2 p0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) / (scale_max - scale_min)));

		const ImU32 col_base = GetColorU32(ImGuiCol_PlotHistogram);
		const ImU32 col_hovered = GetColorU32(ImGuiCol_PlotHistogramHovered);

		for (int n = 0; n < res_w; n++)
		{
			const float t1 = t0 + t_step;
			const int v_idx = (int)(t0 * values_count + 0.5f);
			IM_ASSERT(v_idx >= 0 && v_idx < values_count);
			const float v1 = values_getter(data, (v_idx + values_offset + 1) % values_count);
			const ImVec2 p1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) / (scale_max - scale_min)));

			window->DrawList->AddRectFilled(ImLerp(inner_bb.Min, inner_bb.Max, p0),
				ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(p1.x, 1.0f)) + ImVec2(-1, 0),
				selected_index == v_idx ? col_hovered : col_base);

			t0 = t1;
			p0 = p1;
		}

		if (overlay_text)
		{
			RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y),
				frame_bb.Max,
				overlay_text,
				NULL,
				NULL,
				ImVec2(0.5f, 0.5f));
		}

		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);

		if (v_hovered >= 0 && IsMouseClicked(0))
		{
			return v_hovered;
		}
		return -1;
	}


	bool ListBox(const char* label,
		int* current_item,
		int scroll_to_item,
		bool(*items_getter)(void*, int, const char**),
		void* data,
		int items_count,
		int height_in_items)
	{
		if (!ListBoxHeader(label, items_count, height_in_items)) return false;

		// Assume all items have even height (= 1 line of text). If you need items of different or
		// variable sizes you can create a custom version of ListBox() in your code without using the
		// clipper.
		bool value_changed = false;
		if (scroll_to_item != -1)
		{
			SetScrollY(scroll_to_item * GetTextLineHeightWithSpacing());
		}
		ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing());
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			const bool item_selected = (i == *current_item);
			const char* item_text;
			if (!items_getter(data, i, &item_text)) item_text = "*Unknown item*";

			PushID(i);
			if (Selectable(item_text, item_selected))
			{
				*current_item = i;
				value_changed = true;
			}
			PopID();
		}
		clipper.End();
		ListBoxFooter();
		return value_changed;
	}


	static bool isPredecessor(ImGuiWindow* win, ImGuiWindow* predecessor)
	{
		if (!win) return false;
		if (win == predecessor) return true;
		return isPredecessor(win->ParentWindow, predecessor);
	}


	bool IsFocusedHierarchy()
	{
		ImGuiContext& g = *GImGui;
		return isPredecessor(g.CurrentWindow, g.NavWindow) || isPredecessor(g.NavWindow, g.CurrentWindow);
	}


	void BringToFront()
	{
		ImGuiContext& g = *GImGui;

		ImGuiWindow* window = GImGui->CurrentWindow;

		if ((window->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus) || g.Windows.back() == window)
		{
			return;
		}
		for (int i = 0; i < g.Windows.Size; i++)
		{
			if (g.Windows[i] == window)
			{
				g.Windows.erase(g.Windows.begin() + i);
				break;
			}
		}
		g.Windows.push_back(window);
	}


	static ImVec2 node_pos;
	static ImGuiID last_node_id;


	void BeginNode(ImGuiID id, ImVec2 screen_pos)
	{
		PushID(id);
		last_node_id = id;
		node_pos = screen_pos;
		SetCursorScreenPos(screen_pos + GetStyle().WindowPadding);
		PushItemWidth(150);
		ImDrawList* draw_list = GetWindowDrawList();
		draw_list->ChannelsSplit(2);
		draw_list->ChannelsSetCurrent(1);
		BeginGroup();
	}


	void EndNode(ImVec2& pos)
	{
		ImDrawList* draw_list = GetWindowDrawList();
		ImGui::SameLine();
		float width = GetCursorScreenPos().x - node_pos.x;
		EndGroup();
		PopItemWidth();
		float height = GetCursorScreenPos().y - node_pos.y;
		ImVec2 size(width + GetStyle().WindowPadding.x, height + GetStyle().WindowPadding.y);
		SetCursorScreenPos(node_pos);

		SetNextWindowPos(node_pos);
		SetNextWindowSize(size);
		BeginChild((ImGuiID)last_node_id, size, false, ImGuiWindowFlags_NoInputs);
		EndChild();

		SetCursorScreenPos(node_pos);
		InvisibleButton("bg", size);
		if (IsItemActive() && IsMouseDragging(0))
		{
			pos += GetIO().MouseDelta;
		}

		draw_list->ChannelsSetCurrent(0);
		draw_list->AddRectFilled(node_pos, node_pos + size, ImColor(230, 230, 230), 4.0f);
		draw_list->AddRect(node_pos, node_pos + size, ImColor(150, 150, 150), 4.0f);

		PopID();
		draw_list->ChannelsMerge();
	}


	ImVec2 GetNodeInputPos(ImGuiID id, int input)
	{
		PushID(id);

		ImGuiWindow* parent_win = GetCurrentWindow();
		char title[256];
		ImFormatString(title, IM_ARRAYSIZE(title), "%s.child_%08x", parent_win->Name, id);
		ImGuiWindow* win = FindWindowByName(title);
		if (!win)
		{
			PopID();
			return ImVec2(0, 0);
		}

		ImVec2 pos = win->Pos;
		pos.x -= NODE_SLOT_RADIUS;
		ImGuiStyle& style = GetStyle();
		pos.y += (GetTextLineHeight() + style.ItemSpacing.y) * input;
		pos.y += style.WindowPadding.y + GetTextLineHeight() * 0.5f;


		PopID();
		return pos;
	}


	ImVec2 GetNodeOutputPos(ImGuiID id, int output)
	{
		PushID(id);

		ImGuiWindow* parent_win = GetCurrentWindow();
		char title[256];
		ImFormatString(title, IM_ARRAYSIZE(title), "%s.child_%08x", parent_win->Name, id);
		ImGuiWindow* win = FindWindowByName(title);
		if (!win)
		{
			PopID();
			return ImVec2(0, 0);
		}

		ImVec2 pos = win->Pos;
		pos.x += win->Size.x + NODE_SLOT_RADIUS;
		ImGuiStyle& style = GetStyle();
		pos.y += (GetTextLineHeight() + style.ItemSpacing.y) * output;
		pos.y += style.WindowPadding.y + GetTextLineHeight() * 0.5f;

		PopID();
		return pos;
	}


	bool NodePin(ImGuiID id, ImVec2 screen_pos)
	{
		ImDrawList* draw_list = GetWindowDrawList();
		SetCursorScreenPos(screen_pos - ImVec2(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS));
		PushID(id);
		InvisibleButton("", ImVec2(2 * NODE_SLOT_RADIUS, 2 * NODE_SLOT_RADIUS));
		bool hovered = IsItemHovered();
		PopID();
		draw_list->AddCircleFilled(screen_pos,
			NODE_SLOT_RADIUS,
			hovered ? ImColor(0, 150, 0, 150) : ImColor(150, 150, 150, 150));
		return hovered;
	}


	void NodeLink(ImVec2 from, ImVec2 to)
	{
		ImVec2 p1 = from;
		ImVec2 t1 = ImVec2(+80.0f, 0.0f);
		ImVec2 p2 = to;
		ImVec2 t2 = ImVec2(+80.0f, 0.0f);
		const int STEPS = 12;
		ImDrawList* draw_list = GetWindowDrawList();
		for (int step = 0; step <= STEPS; step++)
		{
			float t = (float)step / (float)STEPS;
			float h1 = +2 * t * t * t - 3 * t * t + 1.0f;
			float h2 = -2 * t * t * t + 3 * t * t;
			float h3 = t * t * t - 2 * t * t + t;
			float h4 = t * t * t - t * t;
			draw_list->PathLineTo(ImVec2(h1 * p1.x + h2 * p2.x + h3 * t1.x + h4 * t2.x,
				h1 * p1.y + h2 * p2.y + h3 * t1.y + h4 * t2.y));
		}
		draw_list->PathStroke(ImColor(200, 200, 100), false, 3.0f);
	}


	ImVec2 operator*(float f, const ImVec2& v)
	{
		return ImVec2(f * v.x, f * v.y);
	}


	const float CurveEditor::GRAPH_MARGIN = 14;
	const float CurveEditor::HEIGHT = 100;

	CurveEditor BeginCurveEditor(const char* label)
	{
		CurveEditor editor;
		editor.valid = false;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return editor;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		editor.beg_pos = GetCursorScreenPos();
		ImVec2 cursor_pos = editor.beg_pos + ImVec2(CurveEditor::GRAPH_MARGIN, CurveEditor::GRAPH_MARGIN);
		SetCursorScreenPos(cursor_pos);

		const ImVec2 label_size = CalcTextSize(label, nullptr, true);

		editor.graph_size.x = CalcItemWidth() + (style.FramePadding.x * 2);
		editor.graph_size.y = CurveEditor::HEIGHT;

		const ImRect frame_bb(cursor_pos, cursor_pos + editor.graph_size);
		const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
		editor.inner_bb_min = inner_bb.Min;
		editor.inner_bb_max = inner_bb.Max;

		const ImRect total_bb(frame_bb.Min,
			frame_bb.Max +
			ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));

		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, 0)) return editor;

		editor.valid = true;
		PushID(label);

		RenderFrame(
			frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);

		SetCursorScreenPos(cursor_pos);

		editor.point_idx = -1;

		return editor;
	}


	void EndCurveEditor(const CurveEditor& editor)
	{
		SetCursorScreenPos(editor.inner_bb_min);
		PopID();

		InvisibleButton("bg", editor.inner_bb_max - editor.inner_bb_min);
		SetCursorScreenPos(editor.beg_pos + ImVec2(0, editor.graph_size.y + 2 * CurveEditor::GRAPH_MARGIN + 4));
	}


	bool CurveSegment(ImVec2* points, CurveEditor& editor)
	{
		ImGuiWindow* window = GetCurrentWindow();

		const ImRect inner_bb(editor.inner_bb_min, editor.inner_bb_max);

		ImVec2 p_last = points[0];
		ImVec2 tangent_last = points[1];
		ImVec2 tangent = points[2];
		ImVec2 p = points[3];

		auto transform = [inner_bb](const ImVec2& p) -> ImVec2
		{
			return ImVec2(inner_bb.Min.x * (1 - p.x) + inner_bb.Max.x * p.x,
				inner_bb.Min.y * p.y + inner_bb.Max.y * (1 - p.y));
		};

		auto handlePoint = [&window, &editor, transform, inner_bb](ImVec2& p) -> bool
		{
			static const float SIZE = 3;

			ImVec2 cursor_pos = GetCursorScreenPos();
			ImVec2 pos = transform(p);

			SetCursorScreenPos(pos - ImVec2(SIZE, SIZE));
			PushID(editor.point_idx);
			++editor.point_idx;
			InvisibleButton("", ImVec2(2 * NODE_SLOT_RADIUS, 2 * NODE_SLOT_RADIUS));

			ImU32 col = IsItemHovered() ? GetColorU32(ImGuiCol_PlotLinesHovered) : GetColorU32(ImGuiCol_PlotLines);

			window->DrawList->AddLine(pos + ImVec2(-SIZE, 0), pos + ImVec2(0, SIZE), col);
			window->DrawList->AddLine(pos + ImVec2(SIZE, 0), pos + ImVec2(0, SIZE), col);
			window->DrawList->AddLine(pos + ImVec2(SIZE, 0), pos + ImVec2(0, -SIZE), col);
			window->DrawList->AddLine(pos + ImVec2(-SIZE, 0), pos + ImVec2(0, -SIZE), col);

			bool changed = false;
			if (IsItemActive() && IsMouseDragging(0))
			{
				pos += GetIO().MouseDelta;
				ImVec2 v;
				v.x = (pos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x);
				v.y = (inner_bb.Max.y - pos.y) / (inner_bb.Max.y - inner_bb.Min.y);

				v = ImClamp(v, ImVec2(0, 0), ImVec2(1, 1));
				p = v;
				changed = true;
			}
			PopID();

			SetCursorScreenPos(cursor_pos);
			return changed;
		};

		auto handleTangent = [&window, &editor, transform](ImVec2& t, const ImVec2& p) -> bool
		{
			static const float SIZE = 2;
			static const float LENGTH = 18;

			auto normalized = [](const ImVec2& v) -> ImVec2
			{
				float len = 1.0f / sqrtf(v.x *v.x + v.y * v.y);
				return ImVec2(v.x * len, v.y * len);
			};

			ImVec2 cursor_pos = GetCursorScreenPos();
			ImVec2 pos = transform(p);
			ImVec2 tang = pos + normalized(ImVec2(t.x, -t.y)) * LENGTH;

			SetCursorScreenPos(tang - ImVec2(SIZE, SIZE));
			PushID(editor.point_idx);
			++editor.point_idx;
			InvisibleButton("", ImVec2(2 * NODE_SLOT_RADIUS, 2 * NODE_SLOT_RADIUS));

			window->DrawList->AddLine(pos, tang, GetColorU32(ImGuiCol_PlotLines));

			ImU32 col = IsItemHovered() ? GetColorU32(ImGuiCol_PlotLinesHovered) : GetColorU32(ImGuiCol_PlotLines);

			window->DrawList->AddLine(tang + ImVec2(-SIZE, SIZE), tang + ImVec2(SIZE, SIZE), col);
			window->DrawList->AddLine(tang + ImVec2(SIZE, SIZE), tang + ImVec2(SIZE, -SIZE), col);
			window->DrawList->AddLine(tang + ImVec2(SIZE, -SIZE), tang + ImVec2(-SIZE, -SIZE), col);
			window->DrawList->AddLine(tang + ImVec2(-SIZE, -SIZE), tang + ImVec2(-SIZE, SIZE), col);

			bool changed = false;
			if (IsItemActive() && IsMouseDragging(0))
			{
				tang = GetIO().MousePos - pos;
				tang = normalized(tang);
				tang.y *= -1;

				t = tang;
				changed = true;
			}
			PopID();

			SetCursorScreenPos(cursor_pos);
			return changed;
		};

		bool changed = false;

		if (editor.point_idx < 0)
		{
			if (handlePoint(p_last))
			{
				p_last.x = 0;
				points[0] = p_last;
				changed = true;
			}
		}
		else
		{
			window->DrawList->AddBezierCurve(
				transform(p_last),
				transform(p_last + tangent_last),
				transform(p + tangent),
				transform(p),
				GetColorU32(ImGuiCol_PlotLines),
				1.0f,
				20);

			if (handleTangent(tangent_last, p_last))
			{
				points[1] = ImClamp(tangent_last, ImVec2(0, -1), ImVec2(1, 1));
				changed = true;
			}

			if (handleTangent(tangent, p))
			{
				points[2] = ImClamp(tangent, ImVec2(-1, -1), ImVec2(0, 1));
				changed = true;
			}

			if (handlePoint(p))
			{
				points[3] = p;
				changed = true;
			}
		}

		return changed;
	}


	bool BeginResizablePopup(const char* str_id, const ImVec2& size_on_first_use)
	{
		if (GImGui->OpenPopupStack.Size <= GImGui->CurrentPopupStack.Size)
		{
			ClearSetNextWindowData();
			return false;
		}
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		const ImGuiID id = window->GetID(str_id);
		if (!IsPopupOpen(id))
		{
			ClearSetNextWindowData();
			return false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGuiWindowFlags flags = ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

		char name[32];
		ImFormatString(name, 20, "##popup_%08x", id);

		ImGui::SetNextWindowSize(size_on_first_use, ImGuiCond_FirstUseEver);
		bool opened = ImGui::Begin(name, NULL, flags);
		if (!(window->Flags & ImGuiWindowFlags_ShowBorders))
			g.CurrentWindow->Flags &= ~ImGuiWindowFlags_ShowBorders;
		if (!opened)
			ImGui::EndPopup();

		return opened;
	}


	void IntervalGraph(const unsigned long long* value_pairs,
		int value_pairs_count,
		unsigned long long scale_min,
		unsigned long long scele_max)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		ImVec2 graph_size(CalcItemWidth() + (style.FramePadding.x * 2), ImGui::GetTextLineHeight());

		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
		const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
		const ImRect total_bb(frame_bb.Min, frame_bb.Max);
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, 0)) return;

		double graph_length = double(scele_max - scale_min);
		const ImU32 col_base = GetColorU32(ImGuiCol_PlotHistogram);

		for (int i = 0; i < value_pairs_count; ++i)
		{
			ImVec2 tmp = frame_bb.Min + ImVec2(float((value_pairs[i * 2] - scale_min) / graph_length * graph_size.x), 0);
			window->DrawList->AddRectFilled(
				tmp, tmp + ImVec2(ImMax(1.0f, float(value_pairs[i * 2 + 1] / graph_length * graph_size.x)), graph_size.y), col_base);
		}
	}


	bool LabellessInputText(const char* label, char* buf, size_t buf_size, float width)
	{
		auto pos = GetCursorPos();
		PushItemWidth(width < 0 ? GetContentRegionAvail().x : width);
		char tmp[32];
		strcpy(tmp, "##");
		strcat(tmp, label);
		bool ret = InputText(tmp, buf, buf_size);
		if (buf[0] == 0 && !IsItemActive())
		{
			pos.x += GetStyle().FramePadding.x;
			SetCursorPos(pos);
			//AlignTextToFramePadding();
			AlignFirstTextHeightToWidgets();
			TextColored(GetStyle().Colors[ImGuiCol_TextDisabled], "%s", label);
		}
		PopItemWidth();
		return ret;
	}


	void Rect(float w, float h, ImU32 color)
	{
		ImGuiWindow* win = GetCurrentWindow();
		ImVec2 screen_pos = GetCursorScreenPos();
		ImVec2 end_pos = screen_pos + ImVec2(w, h);
		ImRect total_bb(screen_pos, end_pos);
		ItemSize(total_bb);
		if (!ItemAdd(total_bb, 0)) return;
		win->DrawList->AddRectFilled(screen_pos, end_pos, color);
	}


	void HSplitter(const char* str_id, ImVec2* size)
	{
		ImVec2 screen_pos = GetCursorScreenPos();
		InvisibleButton(str_id, ImVec2(-1, 3));
		ImVec2 end_pos = screen_pos + GetItemRectSize();
		ImGuiWindow* win = GetCurrentWindow();
		ImVec4* colors = GetStyle().Colors;
		ImU32 color = GetColorU32(IsItemActive() || IsItemHovered() ? colors[ImGuiCol_ButtonActive] : colors[ImGuiCol_Button]);
		win->DrawList->AddRectFilled(screen_pos, end_pos, color);
		if (ImGui::IsItemActive())
		{
			size->y = ImMax(1.0f, ImGui::GetIO().MouseDelta.y + size->y);
		}
	}


	void VSplitter(const char* str_id, ImVec2* size)
	{
		ImVec2 screen_pos = GetCursorScreenPos();
		InvisibleButton(str_id, ImVec2(3, -1));
		ImVec2 end_pos = screen_pos + GetItemRectSize();
		ImGuiWindow* win = GetCurrentWindow();
		ImVec4* colors = GetStyle().Colors;
		ImU32 color = GetColorU32(IsItemActive() || IsItemHovered() ? colors[ImGuiCol_ButtonActive] : colors[ImGuiCol_Button]);
		win->DrawList->AddRectFilled(screen_pos, end_pos, color);
		if (ImGui::IsItemActive())
		{
			size->x = ImMax(1.0f, ImGui::GetIO().MouseDelta.x + size->x);
		}
	}

	static float s_max_timeline_value;


	bool BeginTimeline(const char* str_id, float max_value)
	{
		s_max_timeline_value = max_value;
		return BeginChild(str_id);
	}


	static const float TIMELINE_RADIUS = 6;


	bool TimelineEvent(const char* str_id, float* value)
	{
		ImGuiWindow* win = GetCurrentWindow();
		const ImU32 inactive_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		const ImU32 active_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
		const ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
		bool changed = false;
		ImVec2 cursor_pos = win->DC.CursorPos;

		ImVec2 pos = cursor_pos;
		pos.x += win->Size.x * *value / s_max_timeline_value + TIMELINE_RADIUS;
		pos.y += TIMELINE_RADIUS;

		SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, TIMELINE_RADIUS));
		InvisibleButton(str_id, ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
		if (IsItemActive() || IsItemHovered())
		{
			ImGui::SetTooltip("%f", *value);
			ImVec2 a(pos.x, GetWindowContentRegionMin().y + win->Pos.y);
			ImVec2 b(pos.x, GetWindowContentRegionMax().y + win->Pos.y);
			win->DrawList->AddLine(a, b, line_color);
		}
		if (IsItemActive() && IsMouseDragging(0))
		{
			*value += GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
			changed = true;
		}
		win->DrawList->AddCircleFilled(
			pos, TIMELINE_RADIUS, IsItemActive() || IsItemHovered() ? active_color : inactive_color);
		ImGui::SetCursorScreenPos(cursor_pos);
		return changed;
	}


	void EndTimeline()
	{
		ImGuiWindow* win = GetCurrentWindow();

		ImU32 color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
		ImU32 text_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
		float rounding = GImGui->Style.ScrollbarRounding;
		ImVec2 start(GetWindowContentRegionMin().x + win->Pos.x,
			GetWindowContentRegionMax().y - GetTextLineHeightWithSpacing() + win->Pos.y);
		ImVec2 end = GetWindowContentRegionMax() + win->Pos;

		win->DrawList->AddRectFilled(start, end, color, rounding);

		const int LINE_COUNT = 5;
		const ImVec2 text_offset(0, GetTextLineHeightWithSpacing());
		for (int i = 0; i < LINE_COUNT; ++i)
		{
			ImVec2 a = GetWindowContentRegionMin() + win->Pos + ImVec2(TIMELINE_RADIUS, 0);
			a.x += i * GetWindowContentRegionWidth() / LINE_COUNT;
			ImVec2 b = a;
			b.y = start.y;
			win->DrawList->AddLine(a, b, line_color);
			char tmp[256];
			ImFormatString(tmp, sizeof(tmp), "%.2f", i * s_max_timeline_value / LINE_COUNT);
			win->DrawList->AddText(b, text_color, tmp);
		}

		EndChild();
	}


} // namespace ImGui


#include "widgets/color_picker.inl"
#include "widgets/color_wheel.inl"
#include "widgets/dock.inl"
#include "widgets/file_list.inl"
#include "widgets/gizmo.inl"
#include "widgets/memory_editor.inl"
#include "widgets/range_slider.inl"

