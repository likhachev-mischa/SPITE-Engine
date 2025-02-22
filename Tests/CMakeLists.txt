cmake_minimum_required(VERSION 3.14)
project(Tests)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB TEST_SOURCES "source/*.cpp")
file(GLOB TEST_HEADERS "source/*.hpp")
set(SOURCES ${TEST_SOURCES} ${TEST_HEADERS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${TEST_HEADERS})

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/d144031940543e15423a25ae5a8a74141044862f.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
	Tests
  ${SOURCES})

target_include_directories(Tests PRIVATE "../vcpkg_installed/x64-windows/include/")
target_include_directories(Tests PRIVATE "../source/")
target_link_directories(
	Tests
PRIVATE
"../x64/Debug/"
)

target_link_directories(
	Tests
	PRIVATE
"../vcpkg_installed/x64-windows/debug/lib/"
)


target_link_libraries(
	Tests
  GTest::gtest_main
	"SPITE.lib"
	"EASTL.lib"
)




include(GoogleTest)
gtest_discover_tests(Tests)
