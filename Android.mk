LOCAL_PATH:= $(call my-dir)

ifneq ($(TARGET_SIMULATOR),true)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
		framebuffer.c \
		gifdecode.c \
		device.c \
		input.c \
		charge.c
 
LOCAL_MODULE := charge
LOCAL_CFLAGS := -DCHARGE_ENABLE_SCREEN
LOCAL_C_INCLUDES += external/giflib
LOCAL_STATIC_LIBRARIES += libgif
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/usr/share/charge/battery.gif
ALL_PREBUILT += $(file)
$(file) : $(LOCAL_PATH)/data/battery.gif | $(ACP)
	$(transform-prebuilt-to-target)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif

