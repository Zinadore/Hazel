#pragma once

#include "Hazel/Renderer/VertexArray.h"

namespace Hazel {
    class D3D12VertexArray : public VertexArray
    {
    public:
        D3D12VertexArray();
        virtual ~D3D12VertexArray();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) override;
        virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) override;

        virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
        virtual const Ref<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
    private:
        std::vector<Ref<VertexBuffer>> m_VertexBuffers;
        Ref<IndexBuffer> m_IndexBuffer;
    };

}