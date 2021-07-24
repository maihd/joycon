#include <Windows.h>

#include <ViGEm/Client.h>
#include <hid/hidapi.h>
#include <hid/hidapi_log.h>

#include <assert.h>
#include <string.h>
#include <signal.h>
#include <vector>
#include <regex>

#include "Joycon.h"
#include "JoyconUtils.h"

struct Application
{
    XboxController* xbox;
    HANDLE          mutex;
};

static bool kill_threads = false;

XboxController* initialize_xbox()
{
    XboxController* xbox = new XboxController();
    xbox->client = vigem_alloc();

    printf("initializing emulated Xbox 360 controller...\n");
    VIGEM_ERROR err = vigem_connect(xbox->client);
    if (err == VIGEM_ERROR_NONE)
    {
        printf(" => connected successfully\n");
    }
    else
    {
        //printf("connection error: %s\n", vigem_error_to_string(err));
        vigem_free(xbox->client);

        printf("press [ENTER] to exit\n");
        exit(1);
    }

    xbox->target = vigem_target_x360_alloc();
    vigem_target_add(xbox->client, xbox->target);
    XUSB_REPORT_INIT(&xbox->report);
    printf(" => added target Xbox 360 Controller\n\n");
    return xbox;
}

void disconnect_exit(XboxController* xbox)
{
    vigem_target_remove(xbox->client, xbox->target);
    vigem_target_free(xbox->target);
    vigem_disconnect(xbox->client);
    vigem_free(xbox->client);
    exit(0);
}

Joycon initialize_left_joycon(std::string* mac)
{
    hid_device* left_joycon = NULL;
    int counter = 1;

    struct hid_device_info* left_joycon_info = hid_enumerate(VENDOR_NINTENDO, PRODUCT_JOYCON_LEFT);
    if (left_joycon_info != nullptr) {
        if (mac != NULL) {
            do {
                hid_device* joycon = hid_open(VENDOR_NINTENDO, PRODUCT_JOYCON_LEFT, left_joycon_info->serial_number);

                std::string temp_mac = JoyconUtils::ReadMacAddressString(joycon);
                if (mac->compare(temp_mac) == 0) {
                    break;
                }
                left_joycon_info = left_joycon_info->next;
            } while (left_joycon_info != NULL);
        }
        else {
            while (left_joycon_info->next != NULL) {
                counter++;
                left_joycon_info = left_joycon_info->next;
            }
        }
    }

    if (left_joycon_info == NULL)
    {
        printf(" => could not find left Joy-Con\n");
        return {};
        /*
        vigem_free(client);
        std::cout << "press [ENTER] to exit" << std::endl;
        getchar();
        exit(1);
        */
    }

    printf(" => found left Joy-Con\n");

    left_joycon = hid_open(VENDOR_NINTENDO, PRODUCT_JOYCON_LEFT, left_joycon_info->serial_number);
    if (left_joycon != NULL)
    {
        printf(" => successfully connected to left Joy-Con\n");
    }
    else
    {
        printf(" => could not connect to left Joy-Con\n");
        return {};
        /*
        vigem_free(client);
        std::cout << "press [ENTER] to exit" << std::endl;
        getchar();
        exit(1);
        */
    }

    Joycon joycon = { left_joycon };
    joycon.Setup(counter, true);
    return joycon;
}

Joycon initialize_right_joycon(std::string* mac)
{
    hid_device* right_joycon = NULL;

    int counter = 1;

    struct hid_device_info* right_joycon_info = hid_enumerate(VENDOR_NINTENDO, PRODUCT_JOYCON_RIGHT);
    if (right_joycon_info != nullptr)
    {
        if (mac != NULL)
        {
            do {
                hid_device* joycon = hid_open(VENDOR_NINTENDO, PRODUCT_JOYCON_RIGHT, right_joycon_info->serial_number);
                std::string temp_mac = JoyconUtils::ReadMacAddressString(joycon);
                if (mac->compare(temp_mac) == 0)
                {
                    break;
                }
                right_joycon_info = right_joycon_info->next;
            } while (right_joycon_info != NULL);
        }
        else
        {
            while (right_joycon_info->next != NULL)
            {
                counter++;
                right_joycon_info = right_joycon_info->next;
            }
        }
    }

    if (right_joycon_info == NULL)
    {
        printf(" => could not find right Joy-Con\n");
        return {};

        /*
        vigem_free(client);
        std::cout << "press [ENTER] to exit" << std::endl;
        getchar();
        exit(1);*/
    }

    printf(" => found right Joy-Con\n");
    right_joycon = hid_open(VENDOR_NINTENDO, PRODUCT_JOYCON_RIGHT, right_joycon_info->serial_number);
    if (right_joycon != NULL)
    {
        printf(" => successfully connected to right Joy-Con\n");
    }
    else
    {
        printf(" => could not connect to right Joy-Con\n");
        return {};
        /*
        vigem_free(client);
        std::cout << "press [ENTER] to exit" << std::endl;
        getchar();
        exit(1);
        */
    }

    Joycon joycon = { right_joycon };
    joycon.Setup(counter, false);
    return joycon;
}

_Function_class_(EVT_VIGEM_X360_NOTIFICATION)
void CALLBACK OnJoyconNofitication(
    PVIGEM_CLIENT Client,
    PVIGEM_TARGET Target,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    UCHAR LedNumber,
    LPVOID UserData
)
{
    Joycon* joycon = (Joycon*)&UserData;
    printf("require shake joycon...\n");
}

int WINAPI LeftJoyconThread(__in Application* app)
{
    printf(" => left Joy-Con thread started\n");
    WaitForSingleObject(app->mutex, INFINITE);
    Joycon left_joycon = initialize_left_joycon(nullptr);
    ReleaseMutex(app->mutex);

    if (!left_joycon.IsValid())
    {
        printf(" => Failed to initialize left joycon\n");
        return 0;
    }

    auto xbox = app->xbox;
    while (!kill_threads)
    {
        left_joycon.Update();
        left_joycon.ProcessLeftRegion(xbox);

        WaitForSingleObject(app->mutex, INFINITE);
        vigem_target_x360_update(xbox->client, xbox->target, xbox->report);
        ReleaseMutex(app->mutex);
    }

    left_joycon.Cleanup();
    return 0;
}

int WINAPI RightJoyconThread(__in Application* app)
{
    printf(" => right Joy-Con thread started\n");
    WaitForSingleObject(app->mutex, INFINITE);
    Joycon right_joycon = initialize_right_joycon(nullptr);
    ReleaseMutex(app->mutex);

    if (!right_joycon.IsValid())
    {
        printf(" => Failed to initialize right joycon\n");
        return 0;
    }

    auto xbox = app->xbox;
    while (!kill_threads)
    {
        right_joycon.Update();
        right_joycon.ProcessRightRegion(xbox);

        WaitForSingleObject(app->mutex, INFINITE);
        vigem_target_x360_update(xbox->client, xbox->target, xbox->report);
        ReleaseMutex(app->mutex);
    }

    right_joycon.Cleanup();
    return 0;
}

void terminate(XboxController* xbox, HANDLE leftThread, HANDLE rightThread)
{
    kill_threads = true;
    Sleep(10);
    printf("disconnecting and exiting...\n");
    disconnect_exit(xbox);
}

void exit_handler(int signum) {
    terminate();
    exit(signum);
}

void ShowDeviceInfo(const char* prefix, const unsigned short product_id, wchar_t* hidserial)
{
    hid_device* joycon = hid_open(VENDOR_NINTENDO, product_id, hidserial);
    if (joycon != nullptr)
    {
        printf("- %s, %s, %s, %s, %s\n",
            prefix,
            JoyconUtils::ReadSerialNumber(joycon, 0x6001, 0xF),
            JoyconUtils::ReadMacAddressString(joycon),
            JoyconUtils::GetColorName(JoyconUtils::ReadColor(joycon)),
            JoyconUtils::ReadBatteryStatusString(joycon)
        );
    }
}

void ShowList(const char* name)
{
    printf("Available devices:\n");

    int left_counter = 0;
    struct hid_device_info* left_joycon_info = hid_enumerate(VENDOR_NINTENDO, PRODUCT_JOYCON_LEFT);
    while (left_joycon_info != nullptr)
    {
        ShowDeviceInfo("Left Joycon", PRODUCT_JOYCON_LEFT, left_joycon_info->serial_number);

        left_counter++;
        left_joycon_info = left_joycon_info->next;
    }

    int right_counter = 0;
    struct hid_device_info* right_joycon_info = hid_enumerate(VENDOR_NINTENDO, PRODUCT_JOYCON_RIGHT);
    while (right_joycon_info != nullptr)
    {
        ShowDeviceInfo("Right Joycon", PRODUCT_JOYCON_RIGHT, right_joycon_info->serial_number);

        right_counter++;
        right_joycon_info = right_joycon_info->next;
    }

    if (left_counter == 0 && right_counter == 0)
    {
        printf("  No joycons available, sync with OS first\n");
    }
}

void ShowUsage(const char* name)
{
    printf(
        "Configure joy cons\n\n"
        "Joycon [/?] [/L] [/V] [/P[[:]Args]]\n\n"
        "Options:\n"
        "/L	List existing joy cons\n"
        "/V	Show Joycon version\n"
        "/P	Pair by MAC\n"
        "Args	Pair formed by -, Pairs separated by ,\n"
        "  	11:22:33:44:55:66-11:22:33:44:55:66,11:22:33:44:55:66-11:22:33:44:55:66\n"
        "/?	Show this help\n\n"
    );
}

void ShowVersion()
{
    printf("Joycon v0.1.0\n");
}

bool IsValidMacString(std::string* mac)
{
    return std::regex_match(*mac, std::regex("[a-fA-F0-9][a-fA-F0-9]:[a-fA-F0-9][a-fA-F0-9]:[a-fA-F0-9][a-fA-F0-9]:[a-fA-F0-9][a-fA-F0-9]:[a-fA-F0-9][a-fA-F0-9]:[a-fA-F0-9][a-fA-F0-9]"));
}

bool CheckJoyconTypeByMac(std::string* mac, unsigned short type)
{
    struct hid_device_info* joycon_info = hid_enumerate(VENDOR_NINTENDO, type);
    do {
        if (joycon_info == NULL)
            break;
        hid_device* joycon = hid_open(VENDOR_NINTENDO, type, joycon_info->serial_number);

        std::string temp_mac = JoyconUtils::ReadMacAddressString(joycon);
        if (mac->compare(temp_mac) == 0) {
            return true;
        }
        joycon_info = joycon_info->next;
    } while (joycon_info != NULL);
    return false;
}

int PairDuoJoycon(std::string* l_mac, std::string* r_mac)
{
    DWORD left_thread_id;
    DWORD right_thread_id;

    XboxController* xbox = initialize_xbox();

    HANDLE mutex = CreateMutex(nullptr, false, nullptr);
    if (mutex == 0 || mutex == INVALID_HANDLE_VALUE)
    {
        printf("\nfailed to created mutex...\n");
        return -1;
    }

    Application* app = new Application();
    app->xbox = xbox;
    app->mutex = mutex;

    printf("\ninitializing threads...\n");

    // Left joycon's input should handle first, to make movement faster
    HANDLE leftThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)LeftJoyconThread, app, 0, &left_thread_id);

    // Right joycon's input should handle first, to make attack faster
    HANDLE rightThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)RightJoyconThread, app, 0, &right_thread_id);

    Sleep(500);
    printf("\n");

    if (leftThread != 0 && leftThread != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(leftThread, INFINITE);
        //TerminateThread(leftThread, 0);
    }

    if (rightThread != 0 && rightThread != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(rightThread, INFINITE);
        //TerminateThread(rightThread, 0);
    }

    terminate(xbox, leftThread, rightThread);
    
    // Release usage memory
    CloseHandle(mutex);
    delete xbox;
    delete app;

    return 0;
}

int pairMacs(std::string* l_mac, std::string* r_mac)
{
    if (!IsValidMacString(l_mac))
    {
        printf("Invalid mac %s\n\n", l_mac->c_str());
        return 1;
    }
    else if (!IsValidMacString(r_mac))
    {
        printf("Invalid mac %s\n\n", r_mac->c_str());
        return 1;
    }
    else if (!CheckJoyconTypeByMac(l_mac, PRODUCT_JOYCON_LEFT))
    {
        printf("Left joy con not found with mac %s\n\n", l_mac->c_str());
        return 1;
    }
    else if (!CheckJoyconTypeByMac(r_mac, PRODUCT_JOYCON_RIGHT))
    {
        printf("Right joy con not found with mac %s\n\n", r_mac->c_str());
        return 1;
    }
    else
    {
        PairDuoJoycon(l_mac, r_mac);
    }
    return 0;
}

int pairTuple(const std::string& pair)
{
    printf("Pair tuple: %s\n", pair.c_str());
    if (pair.empty())
    {
        printf("Add pair arguments\n");
        return 1;
    }
    else if (pair.find("-"))
    {
        //std::istringstream f(pair);
        std::string s;
        std::vector<std::string> strings;
        //while (getline(f, s, '-')) {
        //	strings.push_back(s);
        //}
        if (strings.size() != 2)
        {
            printf("Wrong number of macs: %s\n", pair.c_str());
            return 1;
        }
        return pairMacs(&strings[0], &strings[1]);
    }
    else
    {
        printf("Unknow pair tuple args: %s\n", pair.c_str());
        return 1;
    }

    return 0;
}

int pair_joycons(const std::string& pairList)
{
    printf("Pair list args: %s\n", pairList.c_str());
    if (pairList.empty())
    {
        printf("Add pair arguments\n");
        return 1;
    }
    else if (pairList.find(","))
    {
        //std::istringstream f(pairList);
        //std::string s;
        //while (getline(f, s, ',')) {
        //	int result = pairTuple(s);
        //	if (result != 0) {
        //		std::cout << "Error pairing tuple" << s << std::endl;
        //		return result;
        //	}
        //}
    }
    else
    {
        return pairTuple(pairList);
    }
    return 0;
}

int main(int argc, const char* argv[])
{
    signal(SIGINT, exit_handler);

    const char* name = argv[0];

#if 0
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help")
        {
            ShowUsage(name);
            return 0;
        }
        else if (arg == "--version")
        {
            ShowVersion();
            return 0;
        }
        else if (arg == "--left")
        {
            ShowList(name);
            return 0;
        }
        else if (arg.rfind("--pair", 0) == 0)
        {
            std::string pairargs = argv[i + 1];
            pair_joycons(pairargs);
            printf("press [ENTER] to exit\n");
            //terminate(xbox, left_thread, right_thread);
            return 0;
        }
        else
        {
            printf("Unknow option %s\n", arg.c_str());
            return 1;
        }
    }
#endif

    // Print current availables devices
    ShowList(name);

    // Without option, pairs the last connected joycons
    return PairDuoJoycon(NULL, NULL);
}