#pragma once

#ifndef IZOD_FW_VERSION
#define IZOD_FW_VERSION "0.1.0"
#endif

#define IZOD_FW_NAME     "iZod Mini"

static inline const char* izod_firmware_version() { return IZOD_FW_VERSION; }
static inline const char* izod_firmware_name() { return IZOD_FW_NAME; }
