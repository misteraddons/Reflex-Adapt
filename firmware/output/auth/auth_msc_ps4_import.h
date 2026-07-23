#pragma once

#include <stddef.h>
#include <stdint.h>

struct AuthMscPs4File {
  uint16_t first_cluster;
  uint32_t file_size;
  uint32_t meta_fingerprint;
  char name[20];
};

struct AuthMscPs4ImportCallbacks {
  bool (*read_named_file)(const char short_name[11], AuthMscPs4File* out_file, void* context);
  bool (*read_last_file)(AuthMscPs4File* out_file, void* context);
  bool (*read_file_data)(const AuthMscPs4File* file, uint8_t* buffer, size_t capacity, void* context);
  void (*set_result_text)(const char* name, const char* text, void* context);
  void (*set_result_with_name)(const char* format, const char* name, uint32_t size, void* context);
};

void auth_msc_ps4_import_reset();
void auth_msc_ps4_import_process(const AuthMscPs4ImportCallbacks& callbacks, void* context);
