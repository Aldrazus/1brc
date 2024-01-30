target_sources(
    "calculate_average"
    PRIVATE
    "calculate_average.cpp"
    "file_view.cpp"
    "file_view.h"
    "hash_map.cpp"
    "hash_map.h"
)

target_include_directories(
    "calculate_average"
    PRIVATE
    .
)
