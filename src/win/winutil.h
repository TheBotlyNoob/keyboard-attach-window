#include <handleapi.h>
#ifdef _WIN32

#ifndef WINUTIL_H
#define WINUTIL_H

#include <codecvt>
#include <locale>
#include <stdexcept>
#include <windows.h>

namespace winutil {
class Handle {
  private:
    HANDLE m_handle;

  public:
    Handle(HANDLE handle) : m_handle(handle) {
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
            throw std::invalid_argument(
                "Handle constructor recieved invalid handle");
        }
    }

    ~Handle() // I. destructor
    {
        CloseHandle(m_handle);
    }

    Handle(const Handle &other) // II. copy constructor
        : Handle(other.m_handle) {}

    Handle &operator=(const Handle &other) // III. copy assignment
    {
        CloseHandle(m_handle);
        m_handle = other.m_handle;
        return *this;
    }

    HANDLE handle() const // accessor
    {
        return m_handle;
    }
};

inline std::wstring s2ws(const std::string &str) {
    using convert_typeX = std::codecvt_utf16<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

inline std::string ws2s(const std::wstring &wstr) {
    using convert_typeX = std::codecvt_utf16<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}
} // namespace winutil

#endif
#endif
