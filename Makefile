include ../Build/Common/CommonDefs.mak

BIN_DIR = ../Bin

INC_DIRS = /usr/local/Cellar/openni/1.5.7.10/include =/usr/local/include/ni =/usr/include/ni =/usr/local/include

SRC_FILES = ./*.cpp

EXE_NAME = kamehameha

ifeq "$(GLUT_SUPPORTED)" "1"
	ifeq ("$(OSTYPE)","Darwin")
		LDFLAGS += -framework OpenGL -framework GLUT
	else
		USED_LIBS += glut GL
	endif
else
	ifeq "$(GLES_SUPPORTED)" "1"
		DEFINES += USE_GLES
		USED_LIBS += GLES_CM IMGegl srv_um
	else
		DUMMY:=$(error No GLUT or GLES!)
	endif
endif

USED_LIBS += OpenNI portaudio sndfile

LIB_DIRS += /usr/local/Lib
LIB_DIRS += /usr/local/Cellar/openni/1.5.7.10/lib

include ../Build/Common/CommonCppMakefile

