LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := eslocation
LOCAL_CFLAGS    := -Werror -DES_ANDROID
LOCAL_SRC_FILES := \
../../src/ESDeviceLocationManager_android.cpp \
../../src/ESDeviceLocationManager.cpp \
../../src/ESLocation.cpp \
../../src/ESGeoNames.cpp \
../../src/ESLocationTimeHelper.cpp \

# Leave a blank line before this one
LOCAL_LDLIBS    := -llog
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../src ../../deps/esutil/src ../../deps/estime/src

include $(BUILD_STATIC_LIBRARY)
