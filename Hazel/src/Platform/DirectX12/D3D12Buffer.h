#pragma once

#include "Hazel/Renderer/Buffer.h"
#include "Platform/DirectX12/ComPtr.h"

#include <cstdint>
struct ID3D12Resource;
struct ID3D12Device2;

namespace Hazel {

    struct D3D12DeviceChild 
    {
    public:
        inline TComPtr<ID3D12Device2> GetDevice() { return m_ParentDevice; }
        D3D12DeviceChild(TComPtr<ID3D12Device2> parentDevice): m_ParentDevice(parentDevice) { }
    protected:
        TComPtr<ID3D12Device2> m_ParentDevice;
    };

    class D3D12VertexBuffer : public VertexBuffer, public D3D12DeviceChild
    {
    public:
        D3D12VertexBuffer(TComPtr<ID3D12Device2> parentDevice, float* vertices, uint32_t count);
        virtual ~D3D12VertexBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual const BufferLayout& GetLayout() const override { return m_Layout; }
        virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
    
        inline TComPtr<ID3D12Resource> const GetUploadBuffer() { return m_UploadBuffer; }
    
    private:
        uint32_t m_Count;
        BYTE* m_Data;
        TComPtr<ID3D12Resource> m_UploadBuffer;
        BufferLayout m_Layout;

    };

    class D3D12IndexBuffer : public IndexBuffer, public D3D12DeviceChild
    {
    public:
        D3D12IndexBuffer(TComPtr<ID3D12Device2> parentDevice, uint32_t* indices, uint32_t count);
        virtual ~D3D12IndexBuffer();

        virtual void Bind() const;
        virtual void Unbind() const;

        virtual uint32_t GetCount() const { return m_Count; }
        inline TComPtr<ID3D12Resource> const GetUploadBuffer() { return m_UploadBuffer; }
    private:
        uint32_t m_Count;
        BYTE* m_Data;
        TComPtr<ID3D12Resource> m_UploadBuffer;
    };


}