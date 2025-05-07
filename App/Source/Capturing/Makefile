#/*******************************************************************************
#*
#* Copyright 2017,2022 NXP
#*
#* See the LICENSE file distributed for more details.
#* 
#*
#*******************************************************************************/


APPNAME			= capturing
DESTDIR			= ../../../Tools/CamCapture
SRCDIR			= ./src
COMMONDIR		= ../Common
PROJECTDIR 		= ../../Source

CXX			= $(CROSS_COMPILE)g++
DEL_FILE		= rm -rf
CP_FILE			= cp -rf    

ROOTFS_DIR 		= $(SDKTARGETSYSROOT)

TARGET_PATH_LIB 	= $(ROOTFS_DIR)/usr/lib
TARGET_PATH_INCLUDE 	= $(ROOTFS_DIR)/usr/include

GST_INCLUDE = -I$(ROOTFS_DIR)/usr/include/glib-2.0 \
			-I$(ROOTFS_DIR)/usr/lib/glib-2.0/include \
			-I$(ROOTFS_DIR)/usr/include/gstreamer-1.0 \
			-I$(ROOTFS_DIR)/usr/lib/gstreamer-1.0/include

CFLAGS		= -DLINUX -D$(DEVICE) -DCAMERAS -std=c++11 -Wall -O3 \
			-I. -I./inc -I$(COMMONDIR)/inc -I$(TARGET_PATH_INCLUDE) -I$(TARGET_PATH_INCLUDE)/libxml2 $(GST_INCLUDE)


LFLAGS		= -Wl,--library-path=$(TARGET_PATH_LIB),-rpath-link=$(TARGET_PATH_LIB) -lm -lc -lstdc++ \
			-lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio -lopencv_calib3d\
			-lGLESv2 -lEGL -lpthread -ldl -lassimp -lxml2  -L$(ROOTFS_DIR)/usr/lib \
			-lglib-2.0 -lgstreamer-1.0 -lgstvideo-1.0 -lgobject-2.0 -lgstgl-1.0




ifeq ($(EGL_FLAVOR),x11)   
CFLAGS		+= -DEGL_USE_X11
LFLAGS 		+=  -lX11

else ifeq ($(EGL_FLAVOR),wl)   
CFLAGS		+= -DEGL_API_WL
LFLAGS 		+=  -lwayland-client -lwayland-cursor -lwayland-egl

else ifeq ($(EGL_FLAVOR),xdg)   
CFLAGS		+= -DEGL_API_WL
LFLAGS 		+=  -lwayland-client -lwayland-cursor -lwayland-egl -L$(ROOTFS_DIR)/usr/lib/plugins/wayland-shell-integration/ -lxdg-shell

else
CFLAGS		+= -DEGL_API_FB
endif

OBJECTS			= $(COMMONDIR)/src/gl_shaders.o \
			  $(COMMONDIR)/src/settings.o \
			  $(COMMONDIR)/src/backend/$(EGL_FLAVOR)/display.o \
		  	  $(COMMONDIR)/src/view.o \
		  	  $(SRCDIR)/capturing.o \
			  $(COMMONDIR)/src/inputs/camera/$(DEVICE)/src_v4l2.o

first: all

all: $(APPNAME)

$(APPNAME) : $(OBJECTS)
	mkdir -p ${DESTDIR}
	@echo " LD " $@
	$(QUIET)$(CC) -g -o $(DESTDIR)/$(APPNAME) $(OBJECTS) $(LFLAGS)

%.o : %.c
	@echo " CC " $@
	$(QUIET)$(CC) $(CFLAGS) -MMD -c $< -o $@

%.o : %.cpp
	@echo " CXX " $@
	$(QUIET)$(CC) $(CFLAGS) -MMD -c $< -o $@

clean:
	$(DEL_FILE) $(SRCDIR)/$(OBJECTS) *.o *.d
	find $(PROJECTDIR) -name "*.d" -type f -delete
	$(DEL_FILE) $(DESTDIR)/$(OBJECTS) *.o *.d


distclean: clean
	$(DEL_FILE) $(DESTDIR)/$(APPNAME)

install: all

	
