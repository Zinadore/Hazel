#include "hzpch.h"

#include "D3D12VertexArray.h"

#include <d3d12.h>
#include <dxgi1_4.h>

namespace Hazel {
    static DXGI_FORMAT ShaderDataTypeToD3D12Format(ShaderDataType type)
    {
        switch (type)
        {
        case Hazel::ShaderDataType::Float:    return DXGI_FORMAT_R32_FLOAT;
        case Hazel::ShaderDataType::Float2:   return DXGI_FORMAT_R32G32_FLOAT;
        case Hazel::ShaderDataType::Float3:   return DXGI_FORMAT_R32G32B32_FLOAT;
        case Hazel::ShaderDataType::Float4:   return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Hazel::ShaderDataType::Mat3:     return DXGI_FORMAT_UNKNOWN;
        case Hazel::ShaderDataType::Mat4:     return DXGI_FORMAT_UNKNOWN;
        case Hazel::ShaderDataType::Int:      return DXGI_FORMAT_R32_SINT;
        case Hazel::ShaderDataType::Int2:     return DXGI_FORMAT_R32G32_SINT;
        case Hazel::ShaderDataType::Int3:     return DXGI_FORMAT_R32G32B32_SINT;
        case Hazel::ShaderDataType::Int4:     return DXGI_FORMAT_R32G32B32A32_SINT;
        case Hazel::ShaderDataType::Bool:     return DXGI_FORMAT_R32_SINT;
        }
        HZ_CORE_ASSERT(false, "Unknown ShaderDataType!");
        return DXGI_FORMAT_UNKNOWN;
    }
    D3D12VertexArray::D3D12VertexArray()
    {
    }
    D3D12VertexArray::~D3D12VertexArray()
    {
    }
    void D3D12VertexArray::Bind() const
    {
    }
    void D3D12VertexArray::Unbind() const
    {
    }
    void D3D12VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
    {
    }
    void D3D12VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
    {
    }
}