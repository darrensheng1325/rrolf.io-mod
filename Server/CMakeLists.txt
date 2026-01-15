# Copyright (C) 2024  Paul Johnson

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.

# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.16)

# Check if WASM_BUILD is set from parent (for single player mode)
# Also check if SERVER_WASM_BUILD is set
# This must be done BEFORE project() to set compiler correctly
if((DEFINED SERVER_WASM_BUILD AND SERVER_WASM_BUILD) OR (DEFINED WASM_BUILD AND WASM_BUILD))
    if(NOT DEFINED WASM_BUILD)
        set(WASM_BUILD 1)
    endif()
    # Set compiler and system before project() call
    set(CMAKE_SYSTEM_NAME Generic)
    # Try common emcc locations, fallback to just "emcc" (must be in PATH)
    if(EXISTS "/opt/homebrew/bin/emcc")
        set(CMAKE_C_COMPILER "/opt/homebrew/bin/emcc" CACHE FILEPATH "C compiler" FORCE)
    elseif(EXISTS "/usr/local/bin/emcc")
        set(CMAKE_C_COMPILER "/usr/local/bin/emcc" CACHE FILEPATH "C compiler" FORCE)
    else()
        # Assume emcc is in PATH - user must have it installed
        set(CMAKE_C_COMPILER "emcc" CACHE FILEPATH "C compiler" FORCE)
    endif()
    # Set C++ compiler to same as C compiler (emcc can handle C++ too)
    # This prevents CMake from trying to detect a system C++ compiler
    set(CMAKE_CXX_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "C++ compiler" FORCE)
    # Skip compiler tests entirely - we know emcc works
    set(CMAKE_C_COMPILER_WORKS TRUE CACHE BOOL "C compiler works" FORCE)
    set(CMAKE_CXX_COMPILER_WORKS TRUE CACHE BOOL "C++ compiler works" FORCE)
    # Use STATIC_LIBRARY target type for try_compile to avoid linking issues
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "Try compile target type" FORCE)
endif()

# Only enable C language (server is C-only, no C++)
# This should prevent CMake from trying to compile C++ test programs
project(rrolf-server LANGUAGES C)
include_directories(..)

# Find libwebsockets, OpenSSL, and other dependencies
if(NOT WASM_BUILD)
find_package(PkgConfig REQUIRED)
set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl@3)
find_package(OpenSSL REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
endif()

set(SRCS
    MobAi/Aggressive.c
    MobAi/Helpers.c
    MobAi/Neutral.c
    MobAi/Passive.c
    System/Ai.c
    System/Camera.c
    System/Centipede.c
    System/CollisionDetection.c
    System/CollisionResolution.c
    System/Drops.c
    System/Health.c
    System/PetalBehavior.c
    System/Velocity.c
    System/Web.c
    Main.c
    EntityAllocation.c
    EntityDetection.c
    Client.c
    Logs.c
    Server.c
    Simulation.c
    SpatialHash.c
    Squad.c
    UpdateProtocol.c
    Waves.c
    ../Shared/Component/Ai.c
    ../Shared/Component/Arena.c
    ../Shared/Component/Centipede.c
    ../Shared/Component/Drop.c
    ../Shared/Component/Flower.c
    ../Shared/Component/Health.c
    ../Shared/Component/Mob.c
    ../Shared/Component/Petal.c
    ../Shared/Component/Physical.c
    ../Shared/Component/PlayerInfo.c
    ../Shared/Component/Relations.c
    ../Shared/Component/Web.c
    ../Shared/Api.c
    ../Shared/Binary.c
    ../Shared/Bitset.c
    ../Shared/cJSON.c
    ../Shared/Crypto.c
    ../Shared/pb.c
    ../Shared/SimulationCommon.c
    ../Shared/StaticData.c
    ../Shared/Utilities.c
    ../Shared/Vector.c
)

if(NOT WASM_BUILD)
    set(CMAKE_C_COMPILER "clang")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRR_SERVER=1")

# release by default
if(DEBUG_BUILD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -gdwarf-4")
    if (WINDOWS)
    else()
#        add_compile_options(-fsanitize=undefined,address)
 #       add_link_options(-fsanitize=undefined,address)
    endif()
elseif(CALLGRIND_BUILD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG -gdwarf-4 -march=native")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG -Ofast")
    # link time optimiations
    if(WINDOWS)
    else()
        # set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

if(RIVET_BUILD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRIVET_BUILD")
    set(SRCS ${SRCS} ../Shared/Rivet.c)
endif()

if(WASM_BUILD)
    # System and compiler already set before project() call
    # Add Emscripten-specific flags
    # Export shared memory functions for direct calling from client
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --closure=0 -DWASM_BUILD -s INITIAL_MEMORY=33554432 -s NO_EXIT_RUNTIME=1 -s EXPORTED_FUNCTIONS=_malloc,_free,_rr_shared_memory_init,_rr_server_shared_init,_rr_server_shared_tick,_rr_server_shared_get_client_message,_rr_server_shared_send_message -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\",\"UTF8ToString\",\"stringToUTF8\",\"HEAPU8\",\"HEAP32\",\"HEAPF64\",\"HEAPU32\"]' -s ALLOW_MEMORY_GROWTH=1 -s USE_PTHREADS=0 -s SHARED_MEMORY=1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRR_WORKER_MODE=1")
    set(SRCS ${SRCS} SharedMemory.c WorkerStorage.c)
endif()

add_executable(rrolf-server ${SRCS})

if(WASM_BUILD)
    # Set output name for WASM build
    set_target_properties(rrolf-server PROPERTIES
        OUTPUT_NAME "rrolf-server-worker"
        SUFFIX ".js"
    )
    # WASM build doesn't need libwebsockets or other native libraries
else()
if (WINDOWS)
    target_link_directories(rrolf-server PRIVATE /c/libwebsockets/unix-build/bin)
    target_link_libraries(rrolf-server ws2_32 wsock32)
else()
    target_include_directories(rrolf-server PRIVATE ${LIBWEBSOCKETS_INCLUDE_DIRS})
    target_link_directories(rrolf-server PRIVATE ${LIBWEBSOCKETS_LIBRARY_DIRS})
    target_link_libraries(rrolf-server pthread ${LIBWEBSOCKETS_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto m)
endif()

if (NUSE_CURL)
else()
    target_link_libraries(rrolf-server curl)
    endif()
endif()
