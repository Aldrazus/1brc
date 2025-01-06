target_sources(
    ${PROJECT_NAME}
    PRIVATE
    "calculate_average.cpp"
    "file_view.cpp"
    "file_view.h"
    "hash_map.cpp"
    "hash_map.h"
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    .
)
