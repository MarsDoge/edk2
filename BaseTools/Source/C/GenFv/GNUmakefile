## @file
# GNU/Linux makefile for 'GenFv' module build.
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
MAKEROOT ?= ..

APPNAME = GenFv

OBJECTS = GenFv.o GenFvInternalLib.o

include $(MAKEROOT)/Makefiles/app.makefile

LIBS = -lCommon
ifeq ($(CYGWIN), CYGWIN)
  LIBS += -L/lib/e2fsprogs -luuid
endif

ifeq ($(LINUX), Linux)
ifndef CROSS_LIB_UUID
  LIBS += -luuid
else
  LIBS += -L$(CROSS_LIB_UUID)
  BUILD_CFLAGS += -D__CROSS_LIB_UUID__ -I $(CROSS_LIB_UUID_INC)
endif
endif

