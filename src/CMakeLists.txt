option(USE_GPULIB "Use gpulib from pcsx rearmed" ON)
option(USE_BGR15 "Hardware BGR15 convert (Only for MIPS targets)" ON)

set(PORT sdl)
set(GPU gpu_unai)
set(SPU spu_pcsxrearmed)

find_package(SDL REQUIRED)
find_package(ZLIB REQUIRED)

set(SRC_FILES
    r3000a.cpp misc.cpp plugins.cpp psxmem.cpp psxhw.cpp
    psxcounters.cpp psxdma.cpp psxbios.cpp psxhle.cpp psxevents.cpp
    psxcommon.cpp
    plugin_lib/plugin_lib.cpp plugin_lib/pl_sshot.cpp plugin_lib/perfmon.cpp
    psxinterpreter.cpp
    mdec.cpp decode_xa.cpp
    cdriso.cpp cdrom.cpp ppf.cpp cheat.cpp
    sio.cpp pad.cpp
    gte.cpp
    external_lib/ioapi.c external_lib/unzip.c
    spu/${SPU}/spu.c
    port/${PORT}/port.cpp
    port/${PORT}/frontend.cpp
    )

if(USE_GPULIB)
    set(GPULIB_FLAG USE_GPULIB)
    set(SRC_FILES ${SRC_FILES}
        gpu/${GPU}/gpulib_if.cpp
        gpu/gpulib/gpu.cpp gpu/gpulib/vout_port.cpp
        )
endif()

if("${SPU}" STREQUAL "spu_pcsxrearmed")
    set(SOUND_DRIVERS sdl)

    set(SRC_FILES ${SRC_FILES}
        spu/spu_pcsxrearmed/dma.c spu/spu_pcsxrearmed/freeze.c
        spu/spu_pcsxrearmed/out.c spu/spu_pcsxrearmed/nullsnd.c
        spu/spu_pcsxrearmed/registers.c)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        set(SRC_FILES ${SRC_FILES} spu/spu_pcsxrearmed/arm_utils.c)
    endif()
    if(HAVE_C64_TOOLS)
        set(SPU_FLAGS ${SPU_FLAGS} C64X_DSP)
        set(SRC_FILES ${SRC_FILES} spu/spu_pcsxrearmed/spu_c64x.c)
    endif()
    if("${SOUND_DRIVERS}" MATCHES "oss")
        set(SPU_FLAGS ${SPU_FLAGS} HAVE_OSS)
        set(SRC_FILES ${SRC_FILES} spu/spu_pcsxrearmed/oss.c)
    endif()
    if("${SOUND_DRIVERS}" MATCHES "alsa")
        set(SPU_FLAGS ${SPU_FLAGS} HAVE_ALSA)
        set(SRC_FILES ${SRC_FILES} spu/spu_pcsxrearmed/alsa.c)
    endif()
    if("${SOUND_DRIVERS}" MATCHES "sdl")
        set(SPU_FLAGS ${SPU_FLAGS} HAVE_SDL)
        set(SRC_FILES ${SRC_FILES} spu/spu_pcsxrearmed/sdl.c)
    endif()
    if("${SOUND_DRIVERS}" MATCHES "pulseaudio")
        set(SPU_FLAGS ${SPU_FLAGS} HAVE_PULSE)
        set(SRC_FILES ${SRC_FILES} spu/spu_pcsxrearmed/pulseaudio.c)
    endif()
    if("${SOUND_DRIVERS}" MATCHES "libretro")
        set(SPU_FLAGS ${SPU_FLAGS} HAVE_LIBRETRO)
    endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_executable(${PROJECT_NAME} ${SRC_FILES})

string(TOUPPER "${GPU}" GPU_FLAG)
string(TOUPPER "${SPU}" SPU_FLAG)
set(GPU_FLAGS ${GPU_FLAGS} ${GPU_FLAG} ${GPULIB_FLAG})
set(SPU_FLAGS ${SPU_FLAGS} ${SPU_FLAG})

if(MINGW)
    set(EXTRA_FLAGS __USE_MINGW_ANSI_STDIO=1)
endif()
if(USE_BGR15)
    set(EXTRA_FLAGS ${EXTRA_FLAGS} USE_BGR15)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE XA_HACK
    "INLINE=static __inline__" "asm=__asm__ __volatile__"
    ${GPU_FLAGS} ${SPU_FLAGS} ${EXTRA_FLAGS})
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-format-truncation)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS}
    . spu/${SPU} gpu/${GPU} port/${PORT} plugin_lib external_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL_LIBRARY} ${ZLIB_LIBRARIES})
