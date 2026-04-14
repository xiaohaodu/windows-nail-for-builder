enable_testing()
add_subdirectory(third_party/googletest-1.14.0)
include_directories(third_party/googletest-1.14.0/googletest/include)
add_subdirectory(third_party/LOG)
include_directories(third_party/LOG)

# 跨平台支持
if(APPLE)
    # macOS 框架
    find_library(CORE_GRAPHICS CoreGraphics)
    find_library(CARBON Carbon)
    find_library(APPLICATION_SERVICES ApplicationServices)
    
    if(CORE_GRAPHICS AND CARBON AND APPLICATION_SERVICES)
        set(PLATFORM_LIBS ${CORE_GRAPHICS} ${CARBON} ${APPLICATION_SERVICES})
    else()
        message(FATAL_ERROR "Failed to find required macOS frameworks")
    endif()
elseif(WIN32)
    # Windows 库
    set(PLATFORM_LIBS user32 gdi32)
endif()