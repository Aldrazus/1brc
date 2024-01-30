#include "file_view.h"

#include <string_view>
#include <cstdint>

#define NOMINMAX  // lol ok
#include <Windows.h>
#include <memoryapi.h>

FileView::FileView(const char* filename) {
    file_ = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    mapping_ =
        CreateFileMapping(file_, NULL, PAGE_READONLY, 0, 0, "mapped file");

    LARGE_INTEGER file_size;
    GetFileSizeEx(file_, &file_size);

    const char* mapping = reinterpret_cast<const char*>(
            MapViewOfFile(mapping_,
                FILE_MAP_READ,  // TODO: experiment with large pages
                0, 0, file_size.QuadPart));

    file_view = std::string_view{mapping, static_cast<uint64_t>(file_size.QuadPart)};

    if (file_view[file_view.length() - 1] == '\n') {
        file_view = file_view.substr(0, file_view.length() - 1);
    }
}

FileView::~FileView() {
    UnmapViewOfFile(map_view_);
    CloseHandle(mapping_);
    CloseHandle(file_);
}
