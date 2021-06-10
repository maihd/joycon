#define _CRT_SECURE_NO_WARNINGS

#include "JoyconUtils.h"

#include <Windows.h>
#include <ViGEm/Client.h>
#include <hid/hidapi.h>
#include <hid/hidapi_log.h>

#include <stdio.h>
#include <string.h>

//
// Data structures
//  
#pragma pack(push, 1)
struct brcm_hdr
{
    uint8_t cmd;
    uint8_t timer;
    uint8_t rumble_l[4];
    uint8_t rumble_r[4];
};

struct brcm_cmd_01
{
    uint8_t subcmd;
    union {
        struct {
            uint32_t offset;
            uint8_t size;
        } spi_data;

        struct {
            uint8_t arg1;
            uint8_t arg2;
        } subcmd_arg;

        struct {
            uint8_t  mcu_cmd;
            uint8_t  mcu_subcmd;
            uint8_t  mcu_mode;
        } subcmd_21_21;

        struct {
            uint8_t  mcu_cmd;
            uint8_t  mcu_subcmd;
            uint8_t  no_of_reg;
            uint16_t reg1_addr;
            uint8_t  reg1_val;
            uint16_t reg2_addr;
            uint8_t  reg2_val;
            uint16_t reg3_addr;
            uint8_t  reg3_val;
            uint16_t reg4_addr;
            uint8_t  reg4_val;
            uint16_t reg5_addr;
            uint8_t  reg5_val;
            uint16_t reg6_addr;
            uint8_t  reg6_val;
            uint16_t reg7_addr;
            uint8_t  reg7_val;
            uint16_t reg8_addr;
            uint8_t  reg8_val;
            uint16_t reg9_addr;
            uint8_t  reg9_val;
        } subcmd_21_23_04;

        struct {
            uint8_t  mcu_cmd;
            uint8_t  mcu_subcmd;
            uint8_t  mcu_ir_mode;
            uint8_t  no_of_frags;
            uint16_t mcu_major_v;
            uint16_t mcu_minor_v;
        } subcmd_21_23_01;
    };
};
#pragma pack(pop)

namespace JoyconUtils
{
    static uint8_t timming_byte = 0;

    bool ReadDeviceInfo(void* device, uint8_t* buffer, int bufferLength)
    {
        hid_device* handle = (hid_device*)device;

        int res;
        uint8_t buf[49];
        int error_reading = 0;
        bool loop = true;
        while (true)
        {
            memset(buf, 0, sizeof(buf));
            auto hdr = (brcm_hdr*)buf;
            auto pkt = (brcm_cmd_01*)(hdr + 1);
            hdr->cmd = 1;
            hdr->timer = timming_byte++ & 0xF;
            pkt->subcmd = 0x02;
            res = hid_write(handle, buf, sizeof(buf));
            int retries = 0;
            while (true) {
                res = hid_read_timeout(handle, buf, sizeof(buf), 64);
                if (*(uint16_t*)&buf[0xD] == 0x0282) {
                    loop = false;
                    break;
                }

                retries++;
                if (retries > 8 || res == 0)
                    break;
            }
            if (!loop) {
                break;
            }

            error_reading++;
            if (error_reading > 20)
                break;
        }
        
        memcpy(buffer, &buf[0xf], max(0xA, bufferLength));
        return true;
    }

    bool ReadMacAddress(void* device, MacAddress* macAddress)
    {
        uint8_t device_info[10];
        if (ReadDeviceInfo(device, device_info, sizeof(device_info)))
        {
            memcpy(macAddress, &device_info[4], sizeof(*macAddress));
            return true;
        }

        return false;
    }

    const char* ReadMacAddressString(void* device)
    {
        MacAddress macAddress;
        if (ReadMacAddress(device, &macAddress))
        {
            thread_local char macAddressString[32];
            sprintf(macAddressString, "%02X:%02X:%02X:%02X:%02X:%02X",
                macAddress.data[0], 
                macAddress.data[1], 
                macAddress.data[2],
                macAddress.data[3], 
                macAddress.data[4],
                macAddress.data[5]
            );

            return macAddressString;
        }

        return "";
    }

    int set_led_busy(hid_device* handle, const unsigned short product_id)
    {
        int res;
        uint8_t buf[49];
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr*)buf;
        auto pkt = (brcm_cmd_01*)(hdr + 1);
        hdr->cmd = 0x01;
        hdr->timer = timming_byte++ & 0xF;
        pkt->subcmd = 0x30;
        pkt->subcmd_arg.arg1 = 0x81;
        res = hid_write(handle, buf, sizeof(buf));
        res = hid_read_timeout(handle, buf, 0, 64);

        //Set breathing HOME Led
        if (product_id != PRODUCT_JOYCON_LEFT)
        {
            memset(buf, 0, sizeof(buf));
            hdr = (brcm_hdr*)buf;
            pkt = (brcm_cmd_01*)(hdr + 1);
            hdr->cmd = 0x01;
            hdr->timer = timming_byte++ & 0xF;
            pkt->subcmd = 0x38;
            pkt->subcmd_arg.arg1 = 0x28;
            pkt->subcmd_arg.arg2 = 0x20;
            buf[13] = 0xF2;
            buf[14] = buf[15] = 0xF0;
            res = hid_write(handle, buf, sizeof(buf));
            res = hid_read_timeout(handle, buf, 0, 64);
        }

        return 0;
    }

    int silence_input_report(hid_device* handle)
    {
        int res;
        uint8_t buf[49];
        int error_reading = 0;

        bool loop = true;

        while (1) {
            memset(buf, 0, sizeof(buf));
            auto hdr = (brcm_hdr*)buf;
            auto pkt = (brcm_cmd_01*)(hdr + 1);
            hdr->cmd = 1;
            hdr->timer = timming_byte++ & 0xF;
            pkt->subcmd = 0x03;
            pkt->subcmd_arg.arg1 = 0x3f;
            res = hid_write(handle, buf, sizeof(buf));
            int retries = 0;
            while (1) {
                res = hid_read_timeout(handle, buf, sizeof(buf), 64);
                if (*(uint16_t*)&buf[0xD] == 0x0380) {
                    loop = false;
                    break;
                }

                retries++;
                if (retries > 8 || res == 0)
                    break;
            }
            if (!loop) {
                break;
            }
            error_reading++;
            if (error_reading > 4)
                break;
        }

        return 0;
    }

    const char* ReadSerialNumber(void* device, uint32_t offset, const uint16_t read_len)
    {
        hid_device* handle = (hid_device*)device;

        int res;
        int error_reading = 0;
        uint8_t buf[49];
        bool loop = true;
        while (1) {
            memset(buf, 0, sizeof(buf));
            auto hdr = (brcm_hdr*)buf;
            auto pkt = (brcm_cmd_01*)(hdr + 1);
            hdr->cmd = 1;
            hdr->timer = timming_byte++ & 0xF;
            pkt->subcmd = 0x10;
            pkt->spi_data.offset = offset;
            pkt->spi_data.size = read_len;
            res = hid_write(handle, buf, sizeof(buf));

            int retries = 0;
            while (1) {
                res = hid_read_timeout(handle, buf, sizeof(buf), 64);
                if ((*(uint16_t*)&buf[0xD] == 0x1090) && (*(uint32_t*)&buf[0xF] == offset)) {
                    loop = false;
                    break;
                }
                retries++;
                if (retries > 8 || res == 0)
                    break;
            }

            if (!loop) {
                break;
            }

            error_reading++;
            if (error_reading > 20)
                return "Error!";
        }

        if (res >= 0x14 + read_len)
        {
            thread_local char serialNumber[64];
            
            int charIndex = 0;
            for (int i = 0; i < read_len; i++)
            {
                if (buf[0x14 + i] != 0x000)
                {
                    serialNumber[charIndex++] = buf[0x14 + i];
                }
            }

            return serialNumber;
        }
        else
        {
            return "Error!";
        }
    }

    int ReadBatteryInfo(uint8_t* test_buf, hid_device* handle)
    {
        int res;
        uint8_t buf[49];

        int error_reading = 0;
        bool loop = true;
        while (1) {
            memset(buf, 0, sizeof(buf));
            auto hdr = (brcm_hdr*)buf;
            auto pkt = (brcm_cmd_01*)(hdr + 1);
            hdr->cmd = 1;
            hdr->timer = timming_byte & 0xF;
            timming_byte++;
            pkt->subcmd = 0x50;
            res = hid_write(handle, buf, sizeof(buf));
            int retries = 0;
            while (1) {
                res = hid_read_timeout(handle, buf, sizeof(buf), 64);
                if (*(uint16_t*)&buf[0xD] == 0x50D0) {
                    loop = false;
                    break;
                }

                retries++;
                if (retries > 8 || res == 0)
                    break;
            }

            if (!loop) {
                break;
            }
            error_reading++;
            if (error_reading > 20)
                break;
        }

        test_buf[0] = buf[0x2];
        test_buf[1] = buf[0xF];
        test_buf[2] = buf[0x10];

        return 0;
    }

    inline int CalcBatteryPercent(int batterryVolt)
    {
        int batteryPercent = 0;
        //int battery = ((uint8_t)battery_info[0] & 0xF0) >> 4;

        // Calculate aproximate battery percent from regulated voltage
        if (batterryVolt < 0x560)
        {
            batteryPercent = 1;
        }
        else if (batterryVolt > 0x55F && batterryVolt < 0x5A0)
        {
            batteryPercent = (int)(((batterryVolt - 0x60) & 0xFF) / 7.0f + 1);
        }
        else if (batterryVolt > 0x59F && batterryVolt < 0x5E0)
        {
            batteryPercent = (int)(((batterryVolt - 0xA0) & 0xFF) / 2.625f + 11);
        }
        else if (batterryVolt > 0x5DF && batterryVolt < 0x618)
        {
            batteryPercent = (int)((batterryVolt - 0x5E0) / 1.8965f + 36);
        }
        else if (batterryVolt > 0x617 && batterryVolt < 0x658)
        {
            batteryPercent = (int)(((batterryVolt - 0x18) & 0xFF) / 1.8529f + 66);
        }
        else if (batterryVolt > 0x657)
        {
            batteryPercent = 100;
        }

        return batteryPercent;
    }

    inline int ReadBatteryPercent(hid_device* handle)
    {
        uint8_t battery_info[3] = {};
        ReadBatteryInfo(battery_info, handle);

        int batterry_volt = battery_info[1] + (battery_info[2] << 8);
        return CalcBatteryPercent(batterry_volt);
    }

    const char* ReadBatteryStatusString(void* device)
    {
        uint8_t battery_info[3] = {};
        ReadBatteryInfo(battery_info, (hid_device*)device);

        int batterry_volt = battery_info[1] + (battery_info[2] << 8);
        int battery_percent = ReadBatteryPercent((hid_device*)device);

        thread_local char buffer[20];
        sprintf(buffer, "%.2fV - %d%%", (batterry_volt * 2.5f) / 1000, battery_percent);
        return buffer;
    }

    int ReadSpiData(uint32_t offset, const uint16_t read_len, uint8_t* test_buf, hid_device* handle)
    {
        int res;
        uint8_t buf[49];
        int error_reading = 0;
        while (1)
        {
            memset(buf, 0, sizeof(buf));
            auto hdr = (brcm_hdr*)buf;
            auto pkt = (brcm_cmd_01*)(hdr + 1);
            hdr->cmd = 1;
            hdr->timer = timming_byte & 0xF;
            timming_byte++;
            pkt->subcmd = 0x10;
            pkt->spi_data.offset = offset;
            pkt->spi_data.size = read_len;
            res = hid_write(handle, buf, sizeof(buf));

            int retries = 0;
            bool loop = true;
            while (true)
            {
                res = hid_read_timeout(handle, buf, sizeof(buf), 64);
                if ((*(uint16_t*)&buf[0xD] == 0x1090) && (*(uint32_t*)&buf[0xF] == offset))
                {
                    loop = false;
                    break;
                }

                retries++;
                if (retries > 8 || res == 0)
                    break;
            }
            if (!loop)
            {
                break;
            }

            error_reading++;
            if (error_reading > 20)
            {
                return 1;
            }
        }

        if (res >= 0x14 + read_len) {
            for (int i = 0; i < read_len; i++) {
                test_buf[i] = buf[0x14 + i];
            }
        }

        return 0;
    }

    Color3B ReadColor(void* device)
    {
        Color3B color;
        int res = ReadSpiData(0x6050, sizeof(color), (uint8_t*)&color, (hid_device*)device);

        return color;
    }

    const char* GetColorName(Color3B color)
    {
        const ColorU32 colorU32 = COLOR_U32(color.r, color.g, color.b);
        switch (colorU32)
        {
        case ANIMAL_CROSSING_BLUE:  return "Animal crossing Blue";
        case ANIMAL_CROSSING_GREEN: return "Animal crossing Green";
        case NEON_BLUE:             return "Neon Blue";
        case NEON_RED:              return "Neon Red";
        case HORI_RED:              return "HORI Red";
        case GRAY:                  return "Gray";
        case NEON_YELLOW:           return "Neon Yellow";
        case NEON_GREEN:            return "Neon Green";
        case NEON_PINK:             return "Neon Pink";
        case BLUE:                  return "Blue";
        case NEON_PURPLE:           return "Neon Purple";
        case NEON_ORANGE:           return "Neon Orange";
        }

        thread_local char colorName[16];
        sprintf(colorName, "#%02X%02X%02X", color.r, color.g, color.b);
        return colorName;
    }
}