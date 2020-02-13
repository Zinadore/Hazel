#pragma once

#include "Hazel/Core/Layer.h"

#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/ImGui/ImGuiImplementation.h"


namespace Hazel {

	class HAZEL_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;
        virtual void OnEvent(Event& event) override;

		void Begin();
		void End();
        void UpdateDockedWindows();
        void RenderDockedWindows();
        void OnResizeBegin();
        void OnResizeEnd();
	private:

        ImGuiImplementation* m_Implementation;
	};

}