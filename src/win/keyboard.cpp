#ifdef _WIN32

#include <handleapi.h>
#include <iostream>
#include <stdint.h>
#include <vector>

#include <windows.h>

extern "C" {
#include <hidsdi.h>
}

#include "keyboard.h"
#include "win/winutil.h"

Keyboard::Keyboard(winutil::Handle device) : Keyboard(Keyboard(device.get())) {}

Keyboard::Keyboard(HANDLE inputdev) : name() {
    uint32_t pcbSize = 0;

    GetRawInputDeviceInfoW(inputdev, RIDI_DEVICENAME, nullptr, &pcbSize);

    if (pcbSize == 0) {
        std::cout << winutil::GetLastErrorAsString() << std::hex << " "
                  << inputdev << std::endl;
        throw std::runtime_error("GetRawInputDeviceInfoW failed to get size");
    }

    std::vector<wchar_t> devName(pcbSize);

    GetRawInputDeviceInfoW(inputdev, RIDI_DEVICENAME, devName.data(), &pcbSize);
    HANDLE dev =
        CreateFileW(devName.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, 0, NULL);

    m_device.reset(dev);
}

std::string Keyboard::getName() {
    if (name != "") {
        return name;
    }

    std::cout << "redoing name";

    uint8_t tries = 5;
    std::vector<wchar_t> productStr(1 << tries);

    while (!HidD_GetProductString(m_device.get(), productStr.data(),
                                  productStr.size())) {
        if (GetLastError() == ERROR_INVALID_HANDLE) {
            throw std::runtime_error(
                "HidD_GetProductString failed: invalid handle");
        }

        if (tries > 10) {
            break;
        }

        tries++;

        productStr.resize(1 << tries);
    };

    std::string str = winutil::ws2s(std::wstring(productStr.data()));

    name = str;

    return name;
}

std::vector<Keyboard> KeyboardDevices::getList() const {
    uint32_t numDevices = 0;
    if (GetRawInputDeviceList(nullptr, &numDevices,
                              sizeof(RAWINPUTDEVICELIST)) != 0) {
        std::cout << "GetRawInputDeviceList errored: " << std::hex
                  << GetLastError() << std::dec << std::endl;
    };

    std::vector<RAWINPUTDEVICELIST> devices(numDevices);

    if (GetRawInputDeviceList(devices.data(), &numDevices,
                              sizeof(RAWINPUTDEVICELIST)) != 0) {
    };

    // TODO: this doesn't seem very efficient

    std::vector<Keyboard> keyboards;
    keyboards.reserve(numDevices);

    for (tagRAWINPUTDEVICELIST &dev : devices) {
        Keyboard k(dev.hDevice);
        keyboards.emplace_back(std::move(k));
    }

    return keyboards;
}

#endif
