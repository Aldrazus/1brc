#pragma once

#include <string_view>
#include <cstdint>

#define NOMINMAX  // lol ok
#include <Windows.h>
#include <memoryapi.h>

class FileView {
    public:
        FileView(const char* filename);

        ~FileView();

        std::string_view file_view;
        uint64_t size;

    private:
        HANDLE file_;
        HANDLE mapping_;
        LPVOID map_view_;

};
