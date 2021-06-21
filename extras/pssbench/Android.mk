LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/local/tmp
LOCAL_MODULE:= pssbench
LOCAL_LICENSE_KINDS:= SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS:= notice
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SRC_FILES += \
	main.cpp \

LOCAL_SHARED_LIBRARIES := \
	libutils \
	liblog \

include $(BUILD_EXECUTABLE)
