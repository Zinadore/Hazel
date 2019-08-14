#include "hzpch.h"

#include "D3D12Buffer.h"
#include "Platform/DirectX12/d3dx12.h"
#include "Platform/DirectX12/D3D12Helpers.h"

#include <d3d12.h>


namespace Hazel {
    /////////////////////////////////////////////////////////////////////////////
    // VertexBuffer /////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    D3D12VertexBuffer::D3D12VertexBuffer(TComPtr<ID3D12Device2> parentDevice, float * vertices, uint32_t count)
        : D3D12DeviceChild(parentDevice), m_Count(count), m_Data(nullptr)
    {
        uint64_t bufferSize = count * sizeof(float);

        ThrowIfFailed(m_ParentDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_UploadBuffer.GetAddressOf())
        ));

        // We are using the whole resource since it is the size of vertices
        // no need for a range
        m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_Data));

        ::memcpy(m_Data, vertices, bufferSize);
    }

    D3D12VertexBuffer::~D3D12VertexBuffer()
    {
        m_UploadBuffer->Unmap(0, nullptr);
    }

    void D3D12VertexBuffer::Bind() const
    {
    }

    void D3D12VertexBuffer::Unbind() const
    {
    }

    /////////////////////////////////////////////////////////////////////////////
    // IndexBuffer //////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    D3D12IndexBuffer::D3D12IndexBuffer(TComPtr<ID3D12Device2> parentDevice, uint32_t * indices, uint32_t count)
        : D3D12DeviceChild(parentDevice), m_Count(count), m_Data(nullptr)
    {
        uint64_t bufferSize = count * sizeof(uint32_t);

        ThrowIfFailed(m_ParentDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_UploadBuffer.GetAddressOf())
        ));

        // We are using the whole resource since it is the size of vertices
        // no need for a range
        m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_Data));

        ::memcpy(m_Data, indices, bufferSize);
    }

    D3D12IndexBuffer::~D3D12IndexBuffer()
    {
        m_UploadBuffer->Unmap(0, nullptr);
    }

    void D3D12IndexBuffer::Bind() const
    {
    }

    void D3D12IndexBuffer::Unbind() const
    {
    }
}