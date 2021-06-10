#pragma once

#include "JoyconTypes.h"

namespace JoyconUtils
{
    bool        ReadDeviceInfo(void* device, uint8_t* buffer, int bufferLength);

    bool        ReadMacAddress(void* device, MacAddress* macAddress);

    Color3B     ReadColor(void* device);
    
    // --------------------------------
    // Get display string info
    // --------------------------------

    const char* GetColorName(Color3B color);
    const char* ReadMacAddressString(void* device);
    const char* ReadBatteryStatusString(void* device);
    const char* ReadSerialNumber(void* device, uint32_t offset, const uint16_t read_len);
}