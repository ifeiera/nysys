#include "main/cpu_info.hpp"

#include <algorithm>
#include <iostream>

#include "helper/wmi_helper.hpp"

#pragma comment(lib, "pdh.lib")

namespace nysys {
namespace detail {

[[nodiscard]] std::string SafeVariantToString(const VARIANT &var) noexcept {
  try {
    if (var.vt == VT_BSTR && var.bstrVal != nullptr) {
      return wmi::detail::BstrToUtf8(var.bstrVal);
    }
  } catch (...) {
  }
  return std::string{kUnknownCpuName};
}

[[nodiscard]] uint32_t SafeVariantToUint32(const VARIANT &var) noexcept {
  try {
    if (var.vt == VT_I4) {
      return static_cast<uint32_t>(var.lVal >= 0 ? var.lVal : 0);
    } else if (var.vt == VT_UI4) {
      return var.ulVal;
    }
  } catch (...) {
  }
  return 0;
}
}  // namespace detail

CPUInfo::CPUInfo(std::string cpuName, uint32_t cpuCores, uint32_t cpuThreads, uint32_t cpuClockSpeed) noexcept
    : m_name(std::move(cpuName)), m_cores(cpuCores), m_threads(cpuThreads), m_clockSpeed(cpuClockSpeed) {}

const std::string &CPUInfo::GetName() const noexcept { return m_name; }

uint32_t CPUInfo::GetCores() const noexcept { return m_cores; }

uint32_t CPUInfo::GetThreads() const noexcept { return m_threads; }

uint32_t CPUInfo::GetClockSpeed() const noexcept { return m_clockSpeed; }

CPUList::CPUList() noexcept { Initialize(); }

void CPUList::Initialize() noexcept {
  try {
    wmi::WMISession wmiSession;
    if (!wmiSession.IsInitialized()) {
      m_lastError = CPUError::WMISessionFailed;
      return;
    }

    auto pEnumerator = wmiSession.ExecuteQuery(
        L"SELECT Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed "
        L"FROM Win32_Processor");
    if (!pEnumerator) {
      m_lastError = CPUError::QueryExecutionFailed;
      return;
    }

    ULONG uReturn = 0;
    IWbemClassObject *pclsObj = nullptr;

    while (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn)) && uReturn != 0) {
      VARIANT vtProp;
      VariantInit(&vtProp);

      try {
        std::string cpuName{detail::kUnknownCpuName};
        if (SUCCEEDED(pclsObj->Get(L"Name", 0, &vtProp, 0, 0))) {
          cpuName = detail::SafeVariantToString(vtProp);
        }
        VariantClear(&vtProp);

        uint32_t cores = 0;
        if (SUCCEEDED(pclsObj->Get(L"NumberOfCores", 0, &vtProp, 0, 0))) {
          cores = detail::SafeVariantToUint32(vtProp);
        }
        VariantClear(&vtProp);

        uint32_t threads = 0;
        if (SUCCEEDED(pclsObj->Get(L"NumberOfLogicalProcessors", 0, &vtProp, 0, 0))) {
          threads = detail::SafeVariantToUint32(vtProp);
        }
        VariantClear(&vtProp);

        uint32_t clockSpeed = 0;
        if (SUCCEEDED(pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0))) {
          clockSpeed = detail::SafeVariantToUint32(vtProp);
        }
        VariantClear(&vtProp);

        m_cpus.emplace_back(std::move(cpuName), cores, threads, clockSpeed);
      } catch (...) {
        VariantClear(&vtProp);
      }

      if (pclsObj) {
        pclsObj->Release();
        pclsObj = nullptr;
      }
    }

    m_initialized = true;
    m_lastError = CPUError::Success;
  } catch (...) {
    m_lastError = CPUError::PropertyRetrievalFailed;
    m_cpus.clear();
  }
}

size_t CPUList::GetCount() const noexcept { return m_cpus.size(); }

const CPUInfo *CPUList::GetCPU(size_t index) const noexcept {
  if (index < m_cpus.size()) {
    return &m_cpus[index];
  }
  return nullptr;
}

const std::vector<CPUInfo> &CPUList::GetCPUs() const noexcept { return m_cpus; }

bool CPUList::IsInitialized() const noexcept { return m_initialized; }

CPUError CPUList::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<CPUList> GetCPUList() { return std::make_unique<CPUList>(); }

}  // namespace nysys