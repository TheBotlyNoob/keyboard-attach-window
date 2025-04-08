#ifdef _WIN32

#include <iostream>
#include <stdint.h>
#include <vector>

#include <windows.h>

extern "C" {
#include <hidsdi.h>
}

#include "keyboard.h"
#include "win/winutil.h"

Keyboard::Keyboard(winutil::Handle dev) : mDevHandle(nullptr) {}

std::string Keyboard::GetName() {
    uint32_t pcbSize = 0;

    GetRawInputDeviceInfoW(mDevice.handle(), RIDI_DEVICENAME, nullptr,
                           &pcbSize);

    if (pcbSize == 0) {
        return std::string();
    }

    std::vector<wchar_t> devName;
    devName.resize(pcbSize);

    if (GetRawInputDeviceInfoW(mDevice.handle(), RIDI_DEVICENAME,
                               devName.data(), &pcbSize) < 0) {
        std::cout << "GetRawInputDeviceInfo returned value < 0" << std::endl;
        return std::string();
    }

    uint8_t tries = 5;
    std::vector<wchar_t> productStr;
    productStr.resize(1 << tries);

    winutil::Handle dev =
        CreateFileW(devName.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, 0, NULL);

    while (!HidD_GetProductString(dev.handle(), productStr.data(),
                                  productStr.size())) {
        if (tries > 10) {
            break;
        }

        tries++;

        productStr.resize(1 << tries);
    };

    std::wstring str = std::wstring(productStr.data());

    return winutil::ws2s(str);
}

std::vector<Keyboard> KeyboardDevices::GetList() {
    uint32_t numDevices = 0;
    if (GetRawInputDeviceList(nullptr, &numDevices,
                              sizeof(RAWINPUTDEVICELIST)) != 0) {
        std::cout << "GetRawInputDeviceList errored: " << std::hex
                  << GetLastError() << std::dec << std::endl;
    };

    std::vector<RAWINPUTDEVICELIST> devices;
    devices.resize(numDevices);

    GetRawInputDeviceList(devices.data(), &numDevices,
                          sizeof(RAWINPUTDEVICELIST));

    // TODO: this doesn't seem very efficient

    std::vector<Keyboard> keyboards;

    for (int i = 0; i < numDevices; i++) {
        keyboards.emplace_back(new Keyboard(devices[i].hDevice));
    }

    return keyboards;
}

#endif
