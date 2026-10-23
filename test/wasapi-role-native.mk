# Opt-in Windows COM contract test, built with the existing MinGW toolchain.
ifndef WASAPI_ROLE_OUT
$(error Set WASAPI_ROLE_OUT to a fresh owned output directory)
endif
WASAPI_ROLE_CC ?= x86_64-w64-mingw32-gcc
WASAPI_ROLE_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
.PHONY: wasapi-role-native
wasapi-role-native: $(WASAPI_ROLE_OUT)/test_wasapi_role.exe
$(WASAPI_ROLE_OUT):
	mkdir -p "$@"
$(WASAPI_ROLE_OUT)/test_wasapi_role.exe: $(WASAPI_ROLE_ROOT)/test/modules/audio_output/wasapi_role.c $(WASAPI_ROLE_ROOT)/modules/audio_output/wasapi_role.h | $(WASAPI_ROLE_OUT)
	$(WASAPI_ROLE_CC) -std=c11 -Wall -Wextra -Werror -I"$(WASAPI_ROLE_ROOT)/modules/audio_output" "$<" -static-libgcc -o "$@"
