 /* Copyright (C) 2020 Øyvind Kolås <pippin@gimp.org>
 */

#if CTX_FORMATTER||CTX_AUDIO

/* returns the maximum string length including terminating \0 */
int ctx_a85enc_len (int input_length);
int ctx_a85enc (const void *srcp, char *dst, int count);

#if CTX_PARSER

int ctx_a85dec (const char *src, char *dst, int count);
int ctx_a85len (const char *src, int count);
#endif

#endif
