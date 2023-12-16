
#ifndef _MAGIC_H
#define _MAGIC_H

void
magic_add (const char *mime_type,
           const char *ext,
           char *magic_data,
           int magic_len,
           int is_text);

bool magic_has_mime(const char *mime_type);

const char *magic_detect_sector512 (const char *path, const char *sector512);

const char *magic_detect_path (const char *location);

int magic_main(int argc, char **argv);
#endif
