#include "GpuDetect.h"

#include <algorithm>
#include <string>
#include <windows.h>
#include <devguid.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")

namespace Upscaler {
    GpuDetectResult GpuDetect::Detect() {
        GpuDetectResult result;

        // Use SetupAPI to get GPU names and vendor info
        HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
        if (hDevInfo != INVALID_HANDLE_VALUE) {
            SP_DEVINFO_DATA devInfoData;
            devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
                wchar_t nameBuf[1024];
                DWORD size = 0;
                if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC,
                                                       nullptr, (PBYTE)nameBuf, sizeof(nameBuf), &size)) {
                    std::wstring wname(nameBuf);
                    std::string name(wname.begin(), wname.end());
                    result.gpuNames.push_back(name);
                }

                // Check vendor via hardware ID
                wchar_t hwIdBuf[1024];
                if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID,
                                                       nullptr, (PBYTE)hwIdBuf, sizeof(hwIdBuf), &size)) {
                    std::wstring whwId(hwIdBuf);
                    std::string hwId(whwId.begin(), whwId.end());
                    std::transform(hwId.begin(), hwId.end(), hwId.begin(), [](unsigned char c) {
                        return static_cast<char>(std::toupper(c));
                    });
                    if (hwId.find("VEN_10DE") != std::string::npos) {
                        result.hasNvidia = true;
                    }
                    if (hwId.find("VEN_1002") != std::string::npos || hwId.find("VEN_1022") != std::string::npos) {
                        result.hasAmd = true;
                    }
                }
            }
            SetupDiDestroyDeviceInfoList(hDevInfo);
        }

        
        DISPLAY_DEVICEA displayDevice;
        displayDevice.cb = sizeof(DISPLAY_DEVICEA);
        for (DWORD deviceIndex = 0; EnumDisplayDevicesA(nullptr, deviceIndex, &displayDevice, 0); ++deviceIndex) {
            if (displayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) {
                continue;
            }
            std::string deviceId = displayDevice.DeviceID;
            std::transform(deviceId.begin(), deviceId.end(), deviceId.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
            if (deviceId.find("VEN_10DE") != std::string::npos) {
                result.hasNvidia = true;
            }
            if (deviceId.find("VEN_1002") != std::string::npos || deviceId.find("VEN_1022") != std::string::npos) {
                result.hasAmd = true;
            }
        }
        return result;
    }
}
