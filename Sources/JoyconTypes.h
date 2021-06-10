#pragma once

#include <stdint.h>

// --------------------------------------------------------------
// Enums
// --------------------------------------------------------------

enum struct JoyconRegion
{
    LeftDpad,
    LeftAnalog,
    LeftAux,
    RightButtons,
    RightAnalog,
    RightAux
};

enum JoyconButton
{
    L_DPAD_LEFT = 1,            // left dpad
    L_DPAD_DOWN = 2,
    L_DPAD_UP = 4,
    L_DPAD_RIGHT = 8,
    L_DPAD_SL = 16,
    L_DPAD_SR = 32,
    L_ANALOG_LEFT = 4,          // left 8-way analog
    L_ANALOG_UP_LEFT = 5,
    L_ANALOG_UP = 6,
    L_ANALOG_UP_RIGHT = 7,
    L_ANALOG_RIGHT = 0,
    L_ANALOG_DOWN_RIGHT = 1,
    L_ANALOG_DOWN = 2,
    L_ANALOG_DOWN_LEFT = 3,
    L_ANALOG_NONE = 8,
    L_SHOULDER = 64,            // left aux area
    L_TRIGGER = 128,
    L_CAPTURE = 32,
    L_MINUS = 1,
    L_STICK = 4,
    R_BUT_A = 1,                // right buttons area
    R_BUT_B = 4,
    R_BUT_Y = 8,
    R_BUT_X = 2,
    R_BUT_SL = 16,
    R_BUT_SR = 32,
    R_SHOULDER = 64,            // right aux area
    R_TRIGGER = 128,
    R_HOME = 16,
    R_PLUS = 2,
    R_STICK = 8,
    R_ANALOG_LEFT = 0,          // right 8-way analog
    R_ANALOG_UP_LEFT = 1,
    R_ANALOG_UP = 2,
    R_ANALOG_UP_RIGHT = 3,
    R_ANALOG_RIGHT = 4,
    R_ANALOG_DOWN_RIGHT = 5,
    R_ANALOG_DOWN = 6,
    R_ANALOG_DOWN_LEFT = 7,
    R_ANALOG_NONE = 8
};

// --------------------------------------------------------------
// Data structures
// --------------------------------------------------------------

using ColorU32 = unsigned int;

struct Color3B
{
    uint32_t r;
    uint32_t g;
    uint32_t b;
};

struct MacAddress
{
    uint8_t data[6];
};

struct Vector2
{
    float x;
    float y;
};

struct Buffer
{
    uint8_t* data;
    int      size;

    template <int length>
    Buffer(uint8_t(&array)[length])
        : data(array)
        , size(length)
    {
    }

    Buffer(uint8_t array[], int length)
        : data(array)
        , size(length)
    {
    }
};

// --------------------------------------------------------------
// Utils
// --------------------------------------------------------------

#define COLOR_U32(r, g, b)              (((r) << 24) | ((g) << 16) | (b))

// --------------------------------------------------------------
// Constants
// --------------------------------------------------------------

constexpr unsigned short VENDOR_NINTENDO                = 0x057e;

constexpr unsigned short PRODUCT_NINTENDO_SWITCH        = 0x2000;
constexpr unsigned short PRODUCT_JOYCON_LEFT            = 0x2006;
constexpr unsigned short PRODUCT_JOYCON_RIGHT           = 0x2007;
constexpr unsigned short PRODUCT_SWITCH_PRO_CONTROLLER  = 0x2009;

constexpr ColorU32 GRAY                     = COLOR_U32(0x82, 0x82, 0x82);
constexpr ColorU32 HORI_RED                 = COLOR_U32(0xF6, 0x22, 0x00);
constexpr ColorU32 NEON_RED                 = COLOR_U32(0xFF, 0x3C, 0x28);
constexpr ColorU32 NEON_BLUE                = COLOR_U32(0x0A, 0xB9, 0xE6);
constexpr ColorU32 NEON_YELLOW              = COLOR_U32(0xE6, 0xFF, 0x00);
constexpr ColorU32 NEON_GREEN               = COLOR_U32(0x1E, 0xDC, 0x00);
constexpr ColorU32 NEON_PINK                = COLOR_U32(0xFF, 0x32, 0x78);
constexpr ColorU32 BLUE                     = COLOR_U32(0x46, 0x55, 0xE6);
constexpr ColorU32 NEON_PURPLE              = COLOR_U32(0xB4, 0x00, 0xF5);
constexpr ColorU32 NEON_ORANGE              = COLOR_U32(0xFA, 0xA0, 0x05);

constexpr ColorU32 ANIMAL_CROSSING_BLUE     = COLOR_U32(0x96, 0xF5, 0xF5);
constexpr ColorU32 ANIMAL_CROSSING_GREEN    = COLOR_U32(0x82, 0xFF, 0x92);
