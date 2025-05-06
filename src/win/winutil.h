#include <type_traits>
#ifdef _WIN32

#ifndef WINUTIL_H
#define WINUTIL_H

#include <codecvt>
#include <handleapi.h>
#include <iostream>
#include <locale>
#include <memory>
#include <stdexcept>
#include <windows.h>

namespace winutil {

struct handle_deleter {
    void operator()(HANDLE handle) { ::CloseHandle(handle); }
};

typedef std::unique_ptr<void, handle_deleter> Handle;

inline std::wstring s2ws(const std::string &string) {
    if (string.empty()) {
        return L"";
    }

    const auto size_needed = MultiByteToWideChar(
        CP_UTF8, 0, string.data(), (int)string.size(), nullptr, 0);
    if (size_needed <= 0) {
        throw std::runtime_error("MultiByteToWideChar() failed: " +
                                 std::to_string(size_needed));
    }

    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, string.data(), (int)string.size(),
                        result.data(), size_needed);
    return result;
}

inline std::string ws2s(const std::wstring &wide_string) {
    if (wide_string.empty()) {
        return "";
    }

    const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, wide_string.data(),
                                                 (int)wide_string.size(),
                                                 nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) {
        throw std::runtime_error("WideCharToMultiByte() failed: " +
                                 std::to_string(size_needed));
    }

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_string.data(), (int)wide_string.size(),
                        result.data(), size_needed, nullptr, nullptr);
    return result;
}

// Returns the last Win32 error, in string format. Returns an empty string if
// there is no error.
inline std::string GetLastErrorAsString() {
    // Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return std::string(); // No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    // Ask Win32 to give us the string version of that message ID.
    // The parameters we pass in, tell Win32 to create the buffer that holds the
    // message for us (because we don't yet know how long the message string
    // will be).
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);

    // Copy the error message into a std::string.
    std::string message(messageBuffer, size);

    // Free the Win32's string's buffer.
    LocalFree(messageBuffer);

    return message;
}
} // namespace winutil

#endif
#endif
