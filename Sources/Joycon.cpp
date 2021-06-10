#include <Windows.h>

#include <ViGEm/Client.h>
#include <hid/hidapi.h>
#include <hid/hidapi_log.h>

#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#include <string>
#include <tuple>
#include <vector>
#include <regex>

#include "Joycon.h"

// Required by ViGEmClient
#pragma comment(lib, "SetupAPI")

// Required by HID Framework
#pragma comment(lib, "Hid.lib")

#define JOYCON_HUMAN_COMMAND(region, button)  (((int)(region) << 8) | (button & 0xff))

bool enable_traffic_dump = false;



// UNKNOWN
//constexpr unsigned short CHARGING_GRIP                  = 0x200e;

constexpr int XBOX_ANALOG_MIN = -32768;
constexpr int XBOX_ANALOG_MAX = 32767;
constexpr int XBOX_ANALOG_DIAG_MIN = -23170;// result of: roundf(XBOX_ANALOG_MIN * 0.5f * sqrtf(2.0f));
constexpr int XBOX_ANALOG_DIAG_MAX = 23170; // result of: roundf(XBOX_ANALOG_MAX * 0.5f * sqrtf(2.0f));

constexpr XUSB_BUTTON BUTTON_MAPPINGS[2048] = {};
constexpr bool InitButtonMappings()
{
    auto buttonMappings = (XUSB_BUTTON*)BUTTON_MAPPINGS;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftDpad, L_DPAD_UP)]      = XUSB_GAMEPAD_DPAD_UP;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftDpad, L_DPAD_DOWN)]    = XUSB_GAMEPAD_DPAD_DOWN;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftDpad, L_DPAD_LEFT)]    = XUSB_GAMEPAD_DPAD_LEFT;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftDpad, L_DPAD_RIGHT)]   = XUSB_GAMEPAD_DPAD_RIGHT;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftDpad, L_DPAD_SL)]      = XUSB_GAMEPAD_X;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftDpad, L_DPAD_SR)]      = XUSB_GAMEPAD_A;

    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftAux, L_SHOULDER)]      = XUSB_GAMEPAD_LEFT_SHOULDER;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftAux, L_CAPTURE)]       = XUSB_GAMEPAD_BACK;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftAux, L_MINUS)]         = XUSB_GAMEPAD_BACK;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::LeftAux, L_STICK)]         = XUSB_GAMEPAD_LEFT_THUMB;

    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightButtons, R_BUT_A)]    = XUSB_GAMEPAD_B;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightButtons, R_BUT_B)]    = XUSB_GAMEPAD_A;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightButtons, R_BUT_X)]    = XUSB_GAMEPAD_Y;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightButtons, R_BUT_Y)]    = XUSB_GAMEPAD_X;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightButtons, R_BUT_SL)]   = XUSB_GAMEPAD_X;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightButtons, R_BUT_SR)]   = XUSB_GAMEPAD_Y;

    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightAux, R_SHOULDER)]     = XUSB_GAMEPAD_RIGHT_SHOULDER;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightAux, R_HOME)]         = XUSB_GAMEPAD_START;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightAux, R_PLUS)]         = XUSB_GAMEPAD_START;
    buttonMappings[JOYCON_HUMAN_COMMAND(JoyconRegion::RightAux, R_STICK)]        = XUSB_GAMEPAD_RIGHT_THUMB;

    return true;
}
constexpr bool BUTTON_MAPPINGS_INITED = InitButtonMappings();

std::tuple<JoyconRegion, JoyconButton> string_to_joycon_button(const std::string& input) {
    if (input == "L_DPAD_LEFT") return std::make_tuple(JoyconRegion::LeftDpad, L_DPAD_LEFT);
    if (input == "L_DPAD_DOWN") return std::make_tuple(JoyconRegion::LeftDpad, L_DPAD_DOWN);
    if (input == "L_DPAD_UP") return std::make_tuple(JoyconRegion::LeftDpad, L_DPAD_UP);
    if (input == "L_DPAD_RIGHT") return std::make_tuple(JoyconRegion::LeftDpad, L_DPAD_RIGHT);
    if (input == "L_DPAD_SL") return std::make_tuple(JoyconRegion::LeftDpad, L_DPAD_SL);
    if (input == "L_DPAD_SR") return std::make_tuple(JoyconRegion::LeftDpad, L_DPAD_SR);
    if (input == "L_ANALOG_LEFT") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_LEFT);
    if (input == "L_ANALOG_UP_LEFT") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_UP_LEFT);
    if (input == "L_ANALOG_UP") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_UP);
    if (input == "L_ANALOG_UP_RIGHT") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_UP_RIGHT);
    if (input == "L_ANALOG_RIGHT") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_RIGHT);
    if (input == "L_ANALOG_DOWN_RIGHT") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_DOWN_RIGHT);
    if (input == "L_ANALOG_DOWN") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_DOWN);
    if (input == "L_ANALOG_DOWN_LEFT") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_DOWN_LEFT);
    if (input == "L_ANALOG_NONE") return std::make_tuple(JoyconRegion::LeftAnalog, L_ANALOG_NONE);
    if (input == "L_SHOULDER") return std::make_tuple(JoyconRegion::LeftAux, L_SHOULDER);
    if (input == "L_TRIGGER") return std::make_tuple(JoyconRegion::LeftAux, L_TRIGGER);
    if (input == "L_CAPTURE") return std::make_tuple(JoyconRegion::LeftAux, L_CAPTURE);
    if (input == "L_MINUS") return std::make_tuple(JoyconRegion::LeftAux, L_MINUS);
    if (input == "L_STICK") return std::make_tuple(JoyconRegion::LeftAux, L_STICK);
    if (input == "R_BUT_A") return std::make_tuple(JoyconRegion::RightButtons, R_BUT_A);
    if (input == "R_BUT_B") return std::make_tuple(JoyconRegion::RightButtons, R_BUT_B);
    if (input == "R_BUT_Y") return std::make_tuple(JoyconRegion::RightButtons, R_BUT_Y);
    if (input == "R_BUT_X") return std::make_tuple(JoyconRegion::RightButtons, R_BUT_X);
    if (input == "R_BUT_SL") return std::make_tuple(JoyconRegion::RightButtons, R_BUT_SL);
    if (input == "R_BUT_SR") return std::make_tuple(JoyconRegion::RightButtons, R_BUT_SR);
    if (input == "R_SHOULDER") return std::make_tuple(JoyconRegion::RightAux, R_SHOULDER);
    if (input == "R_TRIGGER") return std::make_tuple(JoyconRegion::RightAux, R_TRIGGER);
    if (input == "R_HOME") return std::make_tuple(JoyconRegion::RightAux, R_HOME);
    if (input == "R_PLUS") return std::make_tuple(JoyconRegion::RightAux, R_PLUS);
    if (input == "R_STICK") return std::make_tuple(JoyconRegion::RightAux, R_STICK);
    if (input == "R_ANALOG_LEFT") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_LEFT);
    if (input == "R_ANALOG_UP_LEFT") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_UP_LEFT);
    if (input == "R_ANALOG_UP") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_UP);
    if (input == "R_ANALOG_UP_RIGHT") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_UP_RIGHT);
    if (input == "R_ANALOG_RIGHT") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_RIGHT);
    if (input == "R_ANALOG_DOWN_RIGHT") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_DOWN_RIGHT);
    if (input == "R_ANALOG_DOWN") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_DOWN);
    if (input == "R_ANALOG_DOWN_LEFT") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_DOWN_LEFT);
    if (input == "R_ANALOG_NONE") return std::make_tuple(JoyconRegion::RightAnalog, R_ANALOG_NONE);
    throw "invalid joycon button: " + input;
}

XUSB_BUTTON string_to_xbox_button(const std::string& input) {
    if (input == "XUSB_GAMEPAD_DPAD_UP") return XUSB_GAMEPAD_DPAD_UP;
    if (input == "XUSB_GAMEPAD_DPAD_DOWN") return XUSB_GAMEPAD_DPAD_DOWN;
    if (input == "XUSB_GAMEPAD_DPAD_LEFT") return XUSB_GAMEPAD_DPAD_LEFT;
    if (input == "XUSB_GAMEPAD_DPAD_RIGHT") return XUSB_GAMEPAD_DPAD_RIGHT;
    if (input == "XUSB_GAMEPAD_START") return XUSB_GAMEPAD_START;
    if (input == "XUSB_GAMEPAD_BACK") return XUSB_GAMEPAD_BACK;
    if (input == "XUSB_GAMEPAD_LEFT_THUMB") return XUSB_GAMEPAD_LEFT_THUMB;
    if (input == "XUSB_GAMEPAD_RIGHT_THUMB") return XUSB_GAMEPAD_RIGHT_THUMB;
    if (input == "XUSB_GAMEPAD_LEFT_SHOULDER") return XUSB_GAMEPAD_LEFT_SHOULDER;
    if (input == "XUSB_GAMEPAD_RIGHT_SHOULDER") return XUSB_GAMEPAD_RIGHT_SHOULDER;
    if (input == "XUSB_GAMEPAD_GUIDE") return XUSB_GAMEPAD_GUIDE;
    if (input == "XUSB_GAMEPAD_A") return XUSB_GAMEPAD_A;
    if (input == "XUSB_GAMEPAD_B") return XUSB_GAMEPAD_B;
    if (input == "XUSB_GAMEPAD_X") return XUSB_GAMEPAD_X;
    if (input == "XUSB_GAMEPAD_Y") return XUSB_GAMEPAD_Y;
    throw "invalid xbox button: " + input;
}

std::string joycon_button_to_string(JoyconRegion region, JoyconButton button) {
    switch (region) {
    case JoyconRegion::LeftDpad:
        switch (button) {
        case L_DPAD_LEFT: return "L_DPAD_LEFT";
        case L_DPAD_DOWN: return "L_DPAD_DOWN";
        case L_DPAD_UP: return "L_DPAD_UP";
        case L_DPAD_RIGHT: return "L_DPAD_RIGHT";
        case L_DPAD_SL: return "L_DPAD_SL";
        case L_DPAD_SR: return "L_DPAD_SR";
        }
    case JoyconRegion::LeftAnalog:
        switch (button) {
        case L_ANALOG_LEFT: return "L_ANALOG_LEFT";
        case L_ANALOG_UP_LEFT: return "L_ANALOG_UP_LEFT";
        case L_ANALOG_UP: return "L_ANALOG_UP";
        case L_ANALOG_UP_RIGHT: return "L_ANALOG_UP_RIGHT";
        case L_ANALOG_RIGHT: return "L_ANALOG_RIGHT";
        case L_ANALOG_DOWN_RIGHT: return "L_ANALOG_DOWN_RIGHT";
        case L_ANALOG_DOWN: return "L_ANALOG_DOWN";
        case L_ANALOG_DOWN_LEFT: return "L_ANALOG_DOWN_LEFT";
        case L_ANALOG_NONE: return "L_ANALOG_NONE";
        }
    case JoyconRegion::LeftAux:
        switch (button) {
        case L_SHOULDER: return "L_SHOULDER";
        case L_TRIGGER: return "L_TRIGGER";
        case L_CAPTURE: return "L_CAPTURE";
        case L_MINUS: return "L_MINUS";
        case L_STICK: return "L_STICK";
        }
    case JoyconRegion::RightButtons:
        switch (button) {
        case R_BUT_A: return "R_BUT_A";
        case R_BUT_B: return "R_BUT_B";
        case R_BUT_Y: return "R_BUT_Y";
        case R_BUT_X: return "R_BUT_X";
        case R_BUT_SL: return "R_BUT_SL";
        case R_BUT_SR: return "R_BUT_SR";
        }
    case JoyconRegion::RightAux:
        switch (button) {
        case R_SHOULDER: return "R_SHOULDER";
        case R_TRIGGER: return "R_TRIGGER";
        case R_HOME: return "R_HOME";
        case R_PLUS: return "R_PLUS";
        case R_STICK: return "R_STICK";
        }
    case JoyconRegion::RightAnalog:
        switch (button) {
        case R_ANALOG_LEFT: return "R_ANALOG_LEFT";
        case R_ANALOG_UP_LEFT: return "R_ANALOG_UP_LEFT";
        case R_ANALOG_UP: return "R_ANALOG_UP";
        case R_ANALOG_UP_RIGHT: return "R_ANALOG_UP_RIGHT";
        case R_ANALOG_RIGHT: return "R_ANALOG_RIGHT";
        case R_ANALOG_DOWN_RIGHT: return "R_ANALOG_DOWN_RIGHT";
        case R_ANALOG_DOWN: return "R_ANALOG_DOWN";
        case R_ANALOG_DOWN_LEFT: return "R_ANALOG_DOWN_LEFT";
        case R_ANALOG_NONE: return "R_ANALOG_NONE";
        }
    default:
        throw "invalid region";
    }
    throw "invalid button";
}

const char* vigem_error_to_string(VIGEM_ERROR error)
{
    switch (error)
    {
    case VIGEM_ERROR_NONE:                          return "none";
    case VIGEM_ERROR_ALREADY_CONNECTED:             return "already connected";
    case VIGEM_ERROR_BUS_ACCESS_FAILED:             return "bus access failed";
    case VIGEM_ERROR_BUS_ALREADY_CONNECTED:         return "bus already connected";
    case VIGEM_ERROR_BUS_NOT_FOUND:                 return "bus not found";
    case VIGEM_ERROR_BUS_VERSION_MISMATCH:          return "bus version mismatch";
    case VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED:   return "callback already registered";
    case VIGEM_ERROR_CALLBACK_NOT_FOUND:            return "callback not found";
    case VIGEM_ERROR_INVALID_TARGET:                return "invalid target";
    case VIGEM_ERROR_NO_FREE_SLOT:                  return "no free slot";
    case VIGEM_ERROR_REMOVAL_FAILED:                return "removal failed";
    case VIGEM_ERROR_TARGET_NOT_PLUGGED_IN:         return "target not plugged in";
    case VIGEM_ERROR_TARGET_UNINITIALIZED:          return "target uninitialized";
    default:                                        return "none";
    }
}

bool Joycon::IsValid()
{
    return device != nullptr;
}

void Joycon::Setup(uint8_t leds, bool is_left)
{
    uint8_t send_buf = 0x3f;

    Command(&send_buf, 1, 0x3);
    ReadStickCalibration(is_left);

    /* TODO: improve bluetooth pairing*/
    send_buf = 0x1;
    Command(&send_buf, 1, 0x1);

    send_buf = 0x2;
    Command(&send_buf, 1, 0x1);

    send_buf = 0x3;
    Command(&send_buf, 1, 0x1);

    send_buf = leds;
    Command(&send_buf, 1, 0x30);

    send_buf = 0x30;
    Command(&send_buf, 1, 0x3);
}

void Joycon::Cleanup()
{
    uint8_t send_buf = 0x3f;
    Command(&send_buf, 1, 0x3);

    // Clear data
    memset(data, 0, sizeof(data));
    memset(stickCalibration, 0, sizeof(stickCalibration));

    hid_close(device);
    device = nullptr;
}
    
void Joycon::Update()
{
    hid_read_timeout(device, data, sizeof(data), 50);
}

void Joycon::Command(const uint8_t* input, int inputLength, int command)
{
    constexpr int       HEADER_SIZE         = 11;
    constexpr int       INDEX_COUNTER       = 1;
    constexpr int       INDEX_COMMAND       = 10;
    constexpr int       MAX_COUNTER_VALUE   = 0xf;
    constexpr uint8_t   PREINIT_BUFFER[]    = { 0x1, 0x0, 0x0, 0x1, 0x40, 0x40, 0x0, 0x1, 0x40, 0x40 };

    assert(inputLength >= 0);
    assert(inputLength <= DATA_BUFFER_SIZE - HEADER_SIZE);

    uint8_t buffer[DATA_BUFFER_SIZE];
    memcpy(buffer, PREINIT_BUFFER, sizeof(PREINIT_BUFFER));
    
    buffer[INDEX_COUNTER] = commandCounter;
    buffer[INDEX_COMMAND] = (uint8_t)command;

    // Write command params
    memcpy(&buffer[HEADER_SIZE], input, inputLength);

    // Push command to device
    hid_write(device, buffer, HEADER_SIZE + inputLength);

    // Update command counter
    commandCounter = (commandCounter + 1) & (MAX_COUNTER_VALUE - 1);

    // Read response
    hid_read_timeout(device, data, sizeof(data), 50);
}

uint8_t* Joycon::ReadSPI(uint8_t addr1, uint8_t addr2, int length)
{
    // Small attempts make code run faster, but less exact result
    constexpr int MAX_ATTEMPTS = 10;
    constexpr int HEADER_SIZE = 20;
    constexpr int SPI_COMMAND = 0x10;

    uint8_t input[] = { addr2, addr1, 0x00, 0x00, (uint8_t)length };
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
    {
        Command(input, sizeof(input), SPI_COMMAND);
        if (!(data[15] != addr2 || data[16] != addr1))
        {
            break;
        }
    }

    // Return output body
    return data + HEADER_SIZE;
}

void Joycon::ReadStickCalibration(bool isLeft)
{
    // Constants
    constexpr int BUFFER_LENGTH = 9;

    // dump calibration data
    uint8_t* out = ReadSPI(0x80, isLeft ? 0x12 : 0x1d, BUFFER_LENGTH);

    // Find valid calibration
    bool found = false;
    for (int i = 0; i < BUFFER_LENGTH; ++i)
    {
        if (out[i] != 0xff)
        {
            // User calibration data found
            found = true;
            break;
        }
    }

    // Read other data of calibration
    if (!found)
    {               
        out = ReadSPI(0x60, isLeft ? 0x3d : 0x46, BUFFER_LENGTH);
    }
        
    // Merge data
    stickCalibration[isLeft ? 4 : 0] = ((out[7] << 8) & 0xf00) | out[6]; // X Min below center
    stickCalibration[isLeft ? 5 : 1] = ((out[8] << 4) | (out[7] >> 4));  // Y Min below center
    stickCalibration[isLeft ? 0 : 2] = ((out[1] << 8) & 0xf00) | out[0]; // X Max above center
    stickCalibration[isLeft ? 1 : 3] = ((out[2] << 4) | (out[1] >> 4));  // Y Max above center
    stickCalibration[isLeft ? 2 : 4] = ((out[4] << 8) & 0xf00 | out[3]); // X Center
    stickCalibration[isLeft ? 3 : 5] = ((out[5] << 4) | (out[4] >> 4));  // Y Center

    // Read deadzone
    out = ReadSPI(0x60, isLeft ? 0x86 : 0x98, 9);
    stickCalibration[6] = ((out[4] << 8) & 0xF00 | out[3]); // Deadzone
}

inline bool HasButton(uint8_t data, JoyconButton button)
{
    return !!(data & button);
}

inline void RightRegionButton(Joycon& joycon, uint8_t data, JoyconButton button, int xbutton)
{
    joycon.rightButtons |= HasButton(data, button) * xbutton;
}

inline void RightRegionTrigger(Joycon& joycon, uint8_t data, JoyconButton button)
{
    joycon.rightTrigger = HasButton(data, button) * 255;
}

inline float GetStickAxis(const uint16_t stickCalibration[], uint16_t raw, int axis)
{
    float result = (raw - stickCalibration[axis + 2]);
    if (abs(result) < stickCalibration[6]) result = 0;       // inside deadzone
    else if (result > 0) result /= stickCalibration[axis];   // axis is above center
    else result /= stickCalibration[axis + 4];               // axis is below center
    if (result > 1)  result = 1;
    if (result < -1) result = -1;
    result *= (XBOX_ANALOG_MAX);
    return result;
}

inline Vector2 GetStick(const uint16_t stickCalibration[], uint8_t a, uint8_t b, uint8_t c)
{
    uint16_t raw_x = (uint16_t)(a | ((b & 0xf) << 8));
    uint16_t raw_y = (uint16_t)((b >> 4) | (c << 4));
    float x = GetStickAxis(stickCalibration, raw_x, 0);
    float y = GetStickAxis(stickCalibration, raw_y, 1);
    return { x, y };
}

inline void ProcessRightStick(Joycon& joycon, uint8_t a, uint8_t b, uint8_t c)
{
    joycon.rightStick = GetStick(joycon.stickCalibration, a, b, c);
}

inline void ProcessRightButton(Joycon& joycon, JoyconRegion region, JoyconButton button)
{
    int command = JOYCON_HUMAN_COMMAND(region, button);
    if (command == JOYCON_HUMAN_COMMAND(JoyconRegion::RightAux, R_TRIGGER))
    {
        joycon.rightTrigger = 255;
    }
    else
    {
        joycon.rightButtons |= BUTTON_MAPPINGS[command];
    }
}

void Joycon::ProcessRightRegion(XboxController* xbox)
{
    // Reset state
    rightTrigger = 0;
    rightButtons = 0;

    uint8_t offset = 0;
    uint8_t shift = 0;
    uint8_t offset2 = 0;
    if (data[0] == 0x30 || data[0] == 0x21)
    {
        // 0x30 input reports order the button status data differently
        // this approach is ugly, but doesn't require changing the enum
        offset = 2;
        offset2 = 1;
        shift = 1;
        ProcessRightStick(*this, data[9], data[10], data[11]);
    }
    else
    {
        ProcessRightButton(*this, JoyconRegion::RightAnalog, (JoyconButton)data[3]);
    }

    RightRegionButton(*this, data[1 + offset] >> (shift * 3), R_BUT_A, XUSB_GAMEPAD_B);
    RightRegionButton(*this, data[1 + offset], R_BUT_B, XUSB_GAMEPAD_A);
    RightRegionButton(*this, data[1 + offset], R_BUT_X, XUSB_GAMEPAD_Y);
    RightRegionButton(*this, data[1 + offset] << (shift * 3), R_BUT_Y, XUSB_GAMEPAD_X);
    RightRegionButton(*this, data[1 + offset] >> shift, R_BUT_SL, XUSB_GAMEPAD_X);
    RightRegionButton(*this, data[1 + offset] << shift, R_BUT_SR, XUSB_GAMEPAD_Y);
    RightRegionButton(*this, data[2 + offset2], R_SHOULDER, XUSB_GAMEPAD_RIGHT_SHOULDER);
    RightRegionButton(*this, data[2 + offset], R_HOME, XUSB_GAMEPAD_START);
    RightRegionButton(*this, data[2 + offset], R_PLUS, XUSB_GAMEPAD_START);
    RightRegionButton(*this, data[2 + offset] << shift, R_STICK, XUSB_GAMEPAD_RIGHT_THUMB);
    RightRegionTrigger(*this, data[2 + offset2], R_TRIGGER);

    // Apply=
    xbox->report.bRightTrigger = (BYTE)rightTrigger;
    xbox->report.sThumbRX = (SHORT)rightStick.x;
    xbox->report.sThumbRY = (SHORT)rightStick.y;

    xbox->rightButtons = rightButtons;
    xbox->report.wButtons = (USHORT)(xbox->leftButtons | rightButtons);
}

__forceinline void LeftRegionButton(Joycon& joycon, unsigned char data, JoyconButton button, int xbutton)
{
    joycon.leftButtons |= HasButton(data, button) * xbutton;
}

__forceinline void LeftRegionTrigger(Joycon& joycon, unsigned char data, JoyconButton button)
{
    joycon.leftTrigger = HasButton(data, button) * 255;
}

inline void ProcessLeftStick(Joycon& joycon, uint8_t a, uint8_t b, uint8_t c)
{
    joycon.leftStick = GetStick(joycon.stickCalibration, a, b, c);
}

inline void ProcessLeftButton(Joycon& joycon, JoyconRegion region, JoyconButton button)
{
    int command = JOYCON_HUMAN_COMMAND(region, button);
    if (command == JOYCON_HUMAN_COMMAND(JoyconRegion::LeftAux, L_TRIGGER))
    {
        joycon.leftTrigger = 255;
    }
    else
    {
        joycon.leftButtons |= BUTTON_MAPPINGS[command];
    }
}

void Joycon::ProcessLeftRegion(XboxController* xbox)
{
    // Reset state
    leftTrigger = 0;
    leftButtons = 0;

    uint8_t offset = 0;
    uint8_t shift = 0;
    uint8_t offset2 = 0;
    if (data[0] == 0x30 || data[0] == 0x21)
    {
        // 0x30 input reports order the button status data differently
        // this approach is ugly, but doesn't require changing the enum
        offset = 2;
        offset2 = 3;
        shift = 1;
        ProcessLeftStick(*this, data[6], data[7], data[8]);
    }
    else
    {
        ProcessLeftButton(*this, JoyconRegion::LeftAnalog, (JoyconButton)data[3]);
    }

    LeftRegionButton(*this, data[1 + offset * 2] << shift, L_DPAD_UP, XUSB_GAMEPAD_DPAD_UP);
    LeftRegionButton(*this, data[1 + offset * 2] << shift, L_DPAD_DOWN, XUSB_GAMEPAD_DPAD_DOWN);
    LeftRegionButton(*this, data[1 + offset * 2] >> (shift * 3), L_DPAD_LEFT, XUSB_GAMEPAD_DPAD_LEFT);
    LeftRegionButton(*this, data[1 + offset * 2] << shift, L_DPAD_RIGHT, XUSB_GAMEPAD_DPAD_RIGHT);
    LeftRegionButton(*this, data[1 + offset * 2] >> shift, L_DPAD_SL, XUSB_GAMEPAD_X);
    LeftRegionButton(*this, data[1 + offset * 2] << shift, L_DPAD_SR, XUSB_GAMEPAD_A);
    LeftRegionButton(*this, data[2 + offset2], L_SHOULDER, XUSB_GAMEPAD_LEFT_SHOULDER);
    LeftRegionButton(*this, data[2 + offset], L_CAPTURE, XUSB_GAMEPAD_BACK);
    LeftRegionButton(*this, data[2 + offset], L_MINUS, XUSB_GAMEPAD_BACK);
    LeftRegionButton(*this, data[2 + offset] >> shift, L_STICK, XUSB_GAMEPAD_LEFT_THUMB);
    LeftRegionTrigger(*this, data[2 + offset2], L_TRIGGER);
        
    // Apply
    xbox->report.bLeftTrigger = (BYTE)leftTrigger;
    xbox->report.sThumbLX = (SHORT)leftStick.x;
    xbox->report.sThumbLY = (SHORT)leftStick.y;

    xbox->leftButtons = leftButtons;
    xbox->report.wButtons = (USHORT)(xbox->rightButtons | leftButtons);
}

