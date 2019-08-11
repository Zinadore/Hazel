#pragma once

#include <wrl.h>
namespace Hazel {
template<typename T>
using TComPtr = Microsoft::WRL::ComPtr<T>;
}