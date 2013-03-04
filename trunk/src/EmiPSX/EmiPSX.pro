#-------------------------------------------------
#
# Project created by QtCreator 2013-01-30T23:36:49
#
#-------------------------------------------------


QT       += core gui opengl multimedia declarative

TARGET = EmiPSX
TEMPLATE = app

HEADERS  += mainwindow.h \
    port.h \
    virtualkey.h \
    emuinterface.h \
    ../gpu/gpu-gles/gpuTexture.h \
    ../gpu/gpu-gles/gpuStdafx.h \
    ../gpu/gpu-gles/gpuPrim.h \
    ../gpu/gpu-gles/gpuPlugin.h \
    ../gpu/gpu-gles/gpuFps.h \
    ../gpu/gpu-gles/gpuExternals.h \
    ../gpu/gpu-gles/gpuDraw.h \
    ../gpu/gpu_unai/gpu_raster_sprite.h \
    ../gpu/gpu_unai/gpu_raster_polygon.h \
    ../gpu/gpu_unai/gpu_raster_line.h \
    ../gpu/gpu_unai/gpu_raster_image.h \
    ../gpu/gpu_unai/gpu_inner_light.h \
    ../gpu/gpu_unai/gpu_inner_blend_arm7.h \
    ../gpu/gpu_unai/gpu_inner_blend_arm5.h \
    ../gpu/gpu_unai/gpu_inner_blend.h \
    ../gpu/gpu_unai/gpu_inner.h \
    ../gpu/gpu_unai/gpu_fixedpoint.h \
    ../gpu/gpu_unai/gpu_command.h \
    ../gpu/gpu_unai/gpu_blit.h \
    ../gpu/gpu_unai/gpu_arm.h \
    ../gpu/gpu_unai/gpu.h \
    relocator/symbian_memory_handler.h \
    relocator/relocutils.h \
    ../gpu/gpu_dfxvideo/gpu_soft.h \
    ../gpu/gpu_dfxvideo/gpu_prim.h \
    ../gpu/gpu_dfxvideo/gpu_fps.h \
    ../gpu/gpu_dfxvideo/gpu_blit.h \
    ../gpu/gpu_dfxvideo/gpu.h \
    relocator/symbols.h \
    glwidget.h \
    ../gpu/newGPU/raster.h \
    ../gpu/newGPU/profiller.h \
    ../gpu/newGPU/op_Texture.h \
    ../gpu/newGPU/op_Light.h \
    ../gpu/newGPU/op_Blend.h \
    ../gpu/newGPU/newGPU.h \
    ../gpu/newGPU/inner_Sprite.h \
    ../gpu/newGPU/inner_Poly.h \
    ../gpu/newGPU/inner_Pixel.h \
    ../gpu/newGPU/inner_Blit.h \
    ../gpu/newGPU/fixed.h \
    ../gpu/newGPU/gpuAPI.h

FORMS    += mainwindow.ui

CONFIG += mobility
MOBILITY += systeminfo

symbian {
    TARGET.UID3 = 0xA0016204
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x1000000 0x1800000
    MMP_RULES += "DEBUGGABLE"
    MMP_RULES += "ALWAYS_BUILD_AS_ARM"
    INCLUDEPATH += /epoc32/include/mmf/server
    LIBS += -lmmfdevsound #required for CMMFDevSound
}

INCLUDEPATH += ../zlib \
            ../gte/gte_new \
            relocator \
            ../spu/spu_franxis \
            ../gpu/gpu_unai \
            ../port/sdl \
            ../gpu/gpu-gles \
            ../gpu/gpu-gles/gles/gles2 \
            ../gpu/gpu_dfxvideo \
            ../
SOURCES +=\
    virtualkey.cpp \
    emuinterface.cpp \
    CImageProvider.cpp \
    mainwindow.cpp \
    main.cpp \
    relocator/symbian_memory_handler.cpp \
    relocator/relocator.cpp \
    ../spu/spu_franxis/spu.cpp \
    ../recompiler/arm/recompiler.cpp \
    ../sio.cpp \
    ../r3000a.cpp \
    ../psxmem.cpp \
    ../psxhw.cpp \
    ../psxhle.cpp \
    ../psxdma.cpp \
    ../psxcounters.cpp \
    ../psxbios.cpp \
    ../profiler.cpp \
    ../plugins.cpp \
    ../pad.cpp \
    ../misc.cpp \
    ../mdec.cpp \
    ../decode_xa.cpp \
    ../debug.cpp \
    ../cdrom.cpp \
    ../cdriso.cpp \
    ../interpreter/interpreter_new/psxinterpreter.cpp \
    ../gte/gte_new/gte.cpp

HEADERS += \
    ../sjisfont.h \
    ../sio.h \
    ../r3000a.h \
    ../psxmem.h \
    ../psxhw.h \
    ../psxhle.h \
    ../psxdma.h \
    ../psxcounters.h \
    ../psxcommon.h \
    ../psxbios.h \
    ../psemu_plugin_defs.h \
    ../profiler.h \
    ../plugins.h \
    ../misc.h \
    ../mdec.h \
    ../decode_xa.h \
    ../debug.h \
    ../cdrom.h \
    ../gpu/gpu_unai/gpu_raster_sprite.h \
    ../gpu/gpu_unai/gpu_raster_polygon.h \
    ../gpu/gpu_unai/gpu_raster_line.h \
    ../gpu/gpu_unai/gpu_raster_image.h \
    ../gpu/gpu_unai/gpu_inner_light.h \
    ../gpu/gpu_unai/gpu_inner_blend.h \
    ../gpu/gpu_unai/gpu_inner.h \
    ../gpu/gpu_unai/gpu_fixedpoint.h \
    ../gpu/gpu_unai/gpu_command.h \
    ../gpu/gpu_unai/gpu_blit.h \
    ../gpu/gpu_unai/gpu.h \
    ../spu/spu_franxis/spu_xa.h \
    ../spu/spu_franxis/spu_regs.h \
    ../spu/spu_franxis/spu_dma.h \
    ../spu/spu_franxis/spu_adsr.h \
    ../spu/spu_franxis/spu.h \
    ../gte/gte_new/gte_divide.h \
    ../gte/gte_new/gte.h \
    ../zlib/zutil.h \
    ../zlib/zlib.h \
    ../zlib/zconf.h \
    ../zlib/trees.h \
    ../zlib/infutil.h \
    ../zlib/inftrees.h \
    ../zlib/inffixed.h \
    ../zlib/inffast.h \
    ../zlib/infcodes.h \
    ../zlib/infblock.h \
    ../zlib/deflate.h \
    ../port/sdl/port.h \
    ../spu/spu_franxis/spu_xa.h \
    ../spu/spu_franxis/spu_regs.h \
    ../spu/spu_franxis/spu_dma.h \
    ../spu/spu_franxis/spu_adsr.h \
    ../spu/spu_franxis/spu.h \
    ../gte/gte_pcsx/gte_divide.h \
    ../gte/gte_pcsx/gte.h \
    ../spu/spu_franxis/spu_xa.h \
    ../spu/spu_franxis/spu_regs.h \
    ../spu/spu_franxis/spu_dma.h \
    ../spu/spu_franxis/spu_adsr.h \
    ../spu/spu_franxis/spu.h \
    ../recompiler/arm/risc_armv4le.h \
    ../recompiler/arm/rec_misc.h \
    ../recompiler/arm/rec_mem.h \
    ../recompiler/arm/rec_gte.h \
    ../recompiler/arm/rec_branch.h \
    ../recompiler/arm/rec_alu.h \
    ../recompiler/arm/opcodes.h \
    ../recompiler/arm/disarm.h \
    ../recompiler/arm/arm.h

INCLUDEPATH += ../zlib \
            ../gte/gte_new \
            relocator \
            ../spu/spu_franxis \
            ../spu/spu_dfxsound \
            ../gpu/gpu_unai \
            ../port/sdl \
            ../gpu/gpu-gles \
            ../gpu/gpu-gles/gles/gles2 \
            ../gpu/gpu_dfxvideo \
            ../gpu/newGPU/ \
            ../

DEFINES += XA_HACK BIOS_FILE=\"scph1001.bin\" MCD1_FILE=\"mcd001.mcr\" \
    MCD2_FILE=\"mcd002.mcr\" INLINE="__inline"  gpu_unai spu_franxis \
    interpreter_new gte_new MAEMO_CHANGES TIME_IN_MSEC PSXREC  ARM_ARCH \
     DYNAREC STD_PSXREC WITH_HLE

QMAKE_CXXFLAGS += -mcpu=arm1136jf-s -O3 -mfloat-abi=softfp -mfpu=vfp \
-ffast-math -fomit-frame-pointer -fstrict-aliasing -mstructure-size-boundary=32\
-fexpensive-optimizations -fweb -frename-registers -falign-functions=32 \
-falign-loops -falign-labels -falign-jumps -finline -finline-functions \
-fno-common -fno-builtin

RESOURCES += \
    EmiPSX.qrc

#-llibGLESv1_CM
LIBS +=  -lEmiPSXlib.lib

OTHER_FILES += \
    ../gpu/gpu_unai/gpu_arm.s \
    data/emuuip.qml \
    data/emuui.qml \
    relocator/relocator_glue.s \
    ../recompiler/arm/run.s \
    ../recompiler/arm/arm.S \
    ../gpu/newGPU/ARM_asm.S \
    ../recompiler3/arm/arm.S
