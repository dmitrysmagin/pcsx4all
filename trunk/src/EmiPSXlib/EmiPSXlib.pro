#-------------------------------------------------
#
# Project created by QtCreator 2013-01-30T23:36:49
#
#-------------------------------------------------
# Separate project just to build asm sources
#-------------------------------------------------
#
# Copyright (C) 2013 AndreBotelho(andrebotelhomail@gmail.com)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.
#-------------------------------------------------------------------

#QT       += core gui opengl multimedia declarative

TARGET = EmiPSXlib
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    ../gpu/gpu_unai/gpu.cpp

INCLUDEPATH += ../zlib \
            ../gte/gte_new \
            relocator \
            ../spu/spu_franxis \
            ../gpu/gpu_unai \
            ../port/sdl \
            ../gpu/gpu-gles \
            ../gpu/gpu-gles/gles/gles2 \
            ../gpu/gpu_dfxvideo \
            ../ \
            ../zlib \
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


QMAKE_CXXFLAGS += -mcpu=arm1136jf-s -O1 -fthread-jumps \
    -falign-functions -falign-jumps -falign-loops -falign-labels\
    -fcaller-saves -fcrossjumping -fcse-follow-jumps \
    -fcse-skip-blocks -fdelete-null-pointer-checks -fexpensive-optimizations \
    -fgcse -fgcse-lm -finline-small-functions -findirect-inlining \
    -foptimize-sibling-calls -fpeephole2  -freorder-blocks \
    -freorder-functions -frerun-cse-after-loop -fsched-interblock \
    -fsched-spec -fsched-stalled-insns=0 -fsched-stalled-insns-dep \
    -fstrict-aliasing -fstrict-overflow -ftree-switch-conversion \
    -ftree-pre -ftree-vrp -mfloat-abi=softfp -mfpu=vfp -ffast-math \
    -fomit-frame-pointer -fstrict-aliasing -mstructure-size-boundary=32\
    -fweb -frename-registers -falign-functions=32 -falign-loops \
    -falign-labels -falign-jumps -finline -finline-functions \
    -fno-common -fno-builtin -fipa-pta

MMP_RULES += "DEBUGGABLE"
MMP_RULES += "ALWAYS_BUILD_AS_ARM"

LIBS += -llibGLESv1_CM

OTHER_FILES += \
    ../gpu/gpu_unai/gpu_arm.s \
    data/emuuip.qml \
    data/emuui.qml \
    relocator/relocator_glue.s \
    ../recompiler/arm/run.s \
    ../recompiler/arm/arm.S \
    ../gpu/newGPU/ARM_asm.S \
    ../recompiler3/arm/arm.S
