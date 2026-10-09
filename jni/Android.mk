LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := pmod
LOCAL_SRC_FILES := src/main.cpp \
                   src/memory.cpp \
                   src/hook.cpp \
                   src/esp.cpp \
                   src/aim.cpp \
                   src/utils.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/includes
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2
LOCAL_CFLAGS := -DNDEBUG -w -s
include $(BUILD_SHARED_LIBRARY)
