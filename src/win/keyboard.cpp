#include <stdexcept>
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

Keyboard::Keyboard(winutil::Handle inputdev) {
    uint32_t pcbSize = 0;

    GetRawInputDeviceInfoW(inputdev.get(), RIDI_DEVICENAME, nullptr, &pcbSize);

    if (pcbSize == 0) {
        std::cout << winutil::GetLastErrorAsString() << std::hex << " "
                  << inputdev.get() << std::endl;
        throw std::runtime_error("GetRawInputDeviceInfoW failed to get size");
    }

    std::vector<wchar_t> devName(pcbSize);

    GetRawInputDeviceInfoW(inputdev.get(), RIDI_DEVICENAME, devName.data(),
                           &pcbSize);
    HANDLE dev =
        CreateFileW(devName.data(), NULL, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL);

    m_device.reset(dev);

    const uint32_t productStrMaxSize = 4093;
    wchar_t productStr[productStrMaxSize];

    // TODO: fallback to deviceNode if this fails
    if (!HidD_GetProductString(m_device.get(), &productStr,
                               productStrMaxSize)) {
        throw std::runtime_error("HidD_GetProductString failed");
    }

    name = winutil::ws2s(std::wstring(productStr));

    _HIDD_ATTRIBUTES attrs;
    if (!HidD_GetAttributes(m_device.get(), &attrs)) {
        throw std::runtime_error("HidD_GetAttributes failed");
    }

    attributes = KeyboardAttributes{attrs.VendorID, attrs.ProductID,
                                    attrs.VersionNumber};
}

const std::string Keyboard::getName() const { return name; }

std::vector<Keyboard> KeyboardDevices::getList() {
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

    std::vector<Keyboard> keyboards;
    keyboards.reserve(numDevices);

    for (tagRAWINPUTDEVICELIST &dev : devices) {
        keyboards.emplace_back(Keyboard(winutil::Handle(dev.hDevice)));
    }

    return keyboards;
}

#endif
