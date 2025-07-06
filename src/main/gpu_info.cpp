#include "main/gpu_info.hpp"

#include <cassert>

#include "helper/utils.hpp"

#pragma comment(lib, "dxgi.lib")

namespace nysys {
namespace detail {

std::string WideStringToUtf8(const wchar_t *wstr) noexcept {
  if (!wstr) {
    return {};
  }

  const int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);

  if (size_needed <= 1) {
    return {};
  }

  std::string result(size_needed - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), size_needed, nullptr, nullptr);

  return result;
}
}  // namespace detail

GPUInfo::GPUInfo(std::string gpuName, double dedicatedMem, double sharedMem, bool integrated, uint32_t index) noexcept
    : m_name(std::move(gpuName)),
      m_dedicatedMemory(dedicatedMem),
      m_sharedMemory(sharedMem),
      m_isIntegrated(integrated),
      m_adapterIndex(index) {}

const std::string &GPUInfo::GetName() const noexcept { return m_name; }

double GPUInfo::GetDedicatedMemory() const noexcept { return m_dedicatedMemory; }

double GPUInfo::GetSharedMemory() const noexcept { return m_sharedMemory; }

bool GPUInfo::IsIntegrated() const noexcept { return m_isIntegrated; }

uint32_t GPUInfo::GetAdapterIndex() const noexcept { return m_adapterIndex; }

GPUList::GPUList() noexcept { Initialize(); }

void GPUList::Initialize() noexcept {
  Microsoft::WRL::ComPtr<IDXGIFactory> factory;
  HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), &factory);
  if (FAILED(hr)) {
    m_lastError = GPUError::DXGIFactoryCreationFailed;
    return;
  }

  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

  for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
    DXGI_ADAPTER_DESC adapterDesc;
    hr = adapter->GetDesc(&adapterDesc);
    if (FAILED(hr)) {
      m_lastError = GPUError::AdapterDescriptionFailed;
      continue;
    }

    const std::string gpuName = detail::WideStringToUtf8(adapterDesc.Description);
    const double dedicatedMemory = utils::BytesToGB(adapterDesc.DedicatedVideoMemory);
    const double sharedMemory = utils::BytesToGB(adapterDesc.SharedSystemMemory);
    const bool isIntegrated = (adapterDesc.DedicatedVideoMemory < detail::kIntegratedGpuMemoryThreshold);

    m_gpus.emplace_back(gpuName, dedicatedMemory, sharedMemory, isIntegrated, i);
  }

  m_initialized = true;
  m_lastError = GPUError::Success;
}

size_t GPUList::GetCount() const noexcept { return m_gpus.size(); }

const GPUInfo *GPUList::GetGPU(size_t index) const noexcept {
  if (index < m_gpus.size()) {
    return &m_gpus[index];
  }
  return nullptr;
}

const std::vector<GPUInfo> &GPUList::GetGPUs() const noexcept { return m_gpus; }

bool GPUList::IsInitialized() const noexcept { return m_initialized; }

GPUError GPUList::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<GPUList> GetGPUList() { return std::make_unique<GPUList>(); }

}  // namespace nysys