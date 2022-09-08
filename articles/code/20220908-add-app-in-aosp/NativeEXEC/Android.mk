LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := Hello

LOCAL_MODULE_HOST_OS := linux

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/src

LOCAL_SRC_FILES := $(call \
     all-c-files-under,src)

LOCAL_PRODUCT_MODULE := true
include $(BUILD_EXECUTABLE)