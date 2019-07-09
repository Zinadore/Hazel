#pragma once

#include <glm/glm.hpp>

#include "VertexArray.h"

namespace Hazel {
    class RendererAPI {
    public:
        enum class API
        {
            None = 0, OpenGL = 1, D3D12 = 2
        };

    public:
        virtual void Clear() = 0;
        virtual void SetClearColor(const glm::vec4& color) = 0;

        virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) = 0;
        inline static API GetAPI() { return s_API; }
        inline static bool IsInitialized() { return s_Instance != nullptr; }
        static void SelectAPI(API api);

    private:
        static API s_API;
        static RendererAPI* s_Instance;

    };
};