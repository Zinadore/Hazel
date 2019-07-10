#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT
#include <system_error>
#include <string>

#include "Hazel/Log.h"

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        std::string message = std::system_category().message(hr);
        HZ_CORE_ASSERT(false, "D3D12 Error: ({0:x}): {1}", hr, message);
    }
}