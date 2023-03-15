#ifndef COMPRESSION_LIB
#define COMPRESSION_LIB

#include <zlib.h>

long int copy_file_in_buffer (const char filename[], Bytef **buf);
int compressor (const char filename[], long int *in_len, Bytef **out, long int *out_len);
int copy_buffer_in_file (const char filename[], Bytef buf[], int len);
int decompressor (const char filename[], Bytef in[], long int in_len, long int *out_len);

#endif
