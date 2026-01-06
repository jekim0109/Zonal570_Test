cmake_minimum_required(VERSION 3.10)
project(fms_core LANGUAGES C CXX)

# Example CMakeLists for building the core native module.
# Edit SOURCES to point to the actual core .cpp files you want to build.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

## Try to auto-discover core sources under common module folders
file(GLOB_RECURSE CORE_SOURCES
  "${CMAKE_SOURCE_DIR}/FMS/*.cpp"
  "${CMAKE_SOURCE_DIR}/AFC/*.cpp"
  "${CMAKE_SOURCE_DIR}/common/**/*.cpp"
  "${CMAKE_SOURCE_DIR}/*.cpp"
)

# Patterns/indicators used to detect Windows-specific sources to exclude
set(WINDOWS_FILENAME_PATTERNS "(^|/|\\\\)stdafx\\.cpp$" "WinMain\\.cpp$" "\\.rc$" )
set(WINDOWS_CONTENT_INDICATORS
  "#include <windows.h>"
  "winmain("
  "afx"
  "hwnd"
  "createwindow"
  "regopenkey"
  "regcreatekey"
  "regsetvalue"
  "coinitialize"
  "cocreateinstance"
  "createservice"
  "deviceiocontrol"
  "setupdi"
  "susi"
  "susi_imc"
  "pragma comment"
  "dshow"
  "directshow"
  "cominitialize"
)

# Directories or path fragments that usually indicate Windows-only or test projects
set(WINDOWS_DIR_PATTERNS
  "/testproject/"
  "/test485/"
  "/test/"
  "/tests/"
  "/test_"
  "/_build/"
  "/build_cmake/"
  "/build_cmake_filtered/"
  "/debug/"
  "/release/"
  "/x64/"
  "/fmsui/"
  "/ui_page/"
  "/wizard/"
  "/wizard2/"
)

# Exclude large Windows-only modules that are not part of portable core
list(APPEND WINDOWS_DIR_PATTERNS
  "/hwmanager/"
  "/fmswsvc/"
  "/serversetdlg/"
  "/test485/"
  "/test/"
  "/turnstile/"
  "/pip/"
  "/processmanager/"
  "/fmsui/"
  "/wizard/"
  "/wizard2/"
  "/testproject/"
)

set(FILTERED_SOURCES)
foreach(src IN LISTS CORE_SOURCES)
  set(skip FALSE)
  # skip files that live in obvious test/build/Windows-only directories
  string(TOLOWER "${src}" _src_l)
  foreach(dirpat IN LISTS WINDOWS_DIR_PATTERNS)
    string(TOLOWER "${dirpat}" _dirpat_l)
    string(FIND "${_src_l}" "${_dirpat_l}" _pos)
    if(NOT _pos EQUAL -1)
      set(skip TRUE)
      break()
    endif()
  endforeach()

  # skip generated/compiler-id files and other CMake internals
  if(NOT skip)
    string(FIND "${_src_l}" "/cmakefiles/" _cmkpos)
    if(NOT _cmkpos EQUAL -1)
      set(skip TRUE)
    endif()
  endif()

  # skip very large files to avoid reading binary blobs
  if(NOT skip)
    file(SIZE "${src}" _size)
    if(_size GREATER 200000)
      set(skip TRUE)
      message(STATUS "Skipping large file from source scan: ${src} (${_size} bytes)")
    endif()
  endif()
  get_filename_component(fname ${src} NAME)
  string(TOLOWER "${fname}" fname_l)
  foreach(pat IN LISTS WINDOWS_FILENAME_PATTERNS)
    if(fname_l MATCHES "${pat}")
      set(skip TRUE)
      break()
    endif()
  endforeach()

  if(NOT skip)
    # read file content and search for indicators (case-insensitive)
    file(READ "${src}" __content_raw)
    string(TOLOWER "${__content_raw}" __content)
    foreach(ind IN LISTS WINDOWS_CONTENT_INDICATORS)
      string(TOLOWER "${ind}" ind_l)
      string(FIND "${__content}" "${ind_l}" pos)
      if(NOT pos EQUAL -1)
        set(skip TRUE)
        break()
      endif()
    endforeach()
  endif()

  if(NOT skip)
    list(APPEND FILTERED_SOURCES ${src})
  endif()
endforeach()

if(FILTERED_SOURCES)
  # Ensure tinyxml parser implementation is included (sometimes filtered by heuristics)
  if(EXISTS "${CMAKE_SOURCE_DIR}/common/tinyxml/tinyxmlparser.cpp")
    list(FIND FILTERED_SOURCES "${CMAKE_SOURCE_DIR}/common/tinyxml/tinyxmlparser.cpp" _tix_idx)
    if(_tix_idx EQUAL -1)
      list(APPEND FILTERED_SOURCES "${CMAKE_SOURCE_DIR}/common/tinyxml/tinyxmlparser.cpp")
    endif()
  endif()
  add_library(fms_core SHARED ${FILTERED_SOURCES})
  target_include_directories(fms_core PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/FMS
    ${CMAKE_SOURCE_DIR}/AFC
  )
  find_library(M_LIB m)
  if(M_LIB)
    target_link_libraries(fms_core PRIVATE ${M_LIB})
  endif()
  message(STATUS "fms_core target created with ${FILTERED_SOURCES}")
else()
  message(WARNING "No core sources remain after Windows-specific filtering; set SOURCES manually if needed.")
  add_library(fms_core INTERFACE)
endif()

# --- optional tests -------------------------------------------------------
option(BUILD_NATIVE_TESTS "Build native tests" ON)
if(BUILD_NATIVE_TESTS)
  enable_testing()
  # Add simple shared-memory unit test if present
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_hwshared.cpp")
    add_executable(test_hwshared ${CMAKE_SOURCE_DIR}/tests/test_hwshared.cpp)
    target_include_directories(test_hwshared PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/common/include)
    # Link HWLog implementation into the test executable so HWLOG resolves
    if(EXISTS "${CMAKE_SOURCE_DIR}/common/HWManager/HWLog_posix_stub.cpp")
      target_sources(test_hwshared PRIVATE ${CMAKE_SOURCE_DIR}/common/HWManager/HWLog_posix_stub.cpp)
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/common/HWManager/HWLog.cpp")
      target_sources(test_hwshared PRIVATE ${CMAKE_SOURCE_DIR}/common/HWManager/HWLog.cpp)
    endif()
    add_test(NAME hwshared COMMAND test_hwshared)
    message(STATUS "Added hwshared test target")
  endif()
endif()
