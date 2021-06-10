#pragma once

#include "JoyconTypes.h"

constexpr int DATA_BUFFER_SIZE  = 64;
constexpr int OUT_BUFFER_SIZE   = 64;

struct XboxController
{
    PVIGEM_CLIENT   client;
    PVIGEM_TARGET   target;
    XUSB_REPORT     report;

    int             leftButtons;
    int             rightButtons;
};

struct Joystick
{
    void*           mac;
    XboxController* xbox;
};

struct hid_device_;

struct Joycon
{
    hid_device_* device;
    MacAddress  macAddress;

    uint8_t     commandCounter;

    int         leftButtons;
    int         rightButtons;

    int         leftTrigger;
    int         rightTrigger;

    Vector2     leftStick;
    Vector2     rightStick;

    uint8_t     data[DATA_BUFFER_SIZE];
    uint16_t    stickCalibration[8];

    bool        IsValid();

    void        Setup(uint8_t leds, bool is_left);
    void        Cleanup();

    void        Update();
    void        Command(const uint8_t* input, int inputLength, int command);

    uint8_t*    ReadSPI(uint8_t addr1, uint8_t addr2, int length);
    void        ReadStickCalibration(bool is_left);

    void        ProcessLeftRegion(XboxController* xbox);
    void        ProcessRightRegion(XboxController* xbox);
};
