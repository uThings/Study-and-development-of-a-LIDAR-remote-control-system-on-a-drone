#include "compression_lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

long int copy_file_in_buffer (const char filename[], Bytef **buf)
{
    FILE *f;
    long int len;
    if ((f= fopen(filename, "rb")) == NULL) {
        return -1;
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);

    *buf = malloc(len);
    if(*buf == NULL) {
        return -2;
    }

    if (len != 0 && fread(*buf, len, 1, f) != 1) {
        return -3;
    }
    
    fclose(f);
    return len;
}

int compressor (const char filename[], long int *in_len, Bytef **out, long int *out_len)
{
    Bytef *in = NULL;
    int ret;

    if ((*in_len = copy_file_in_buffer(filename, &in)) < 0) {
        free(in);
        return -1;
    }

    *out_len = compressBound(*in_len);
    *out = malloc(*out_len);
    if (*out == NULL) {
        free(in);
        return -2;
    }

    ret = compress(*out, out_len, in, *in_len);
    if (ret != Z_OK) {
        ret = -3;
    }
    else {
        ret = 0;
    }

    free(in);
    return ret;
}

int copy_buffer_in_file (const char filename[], Bytef buf[], int len)
{
    FILE *f;
    if ((f = fopen(filename, "wb")) == NULL) {
        return -1;
    }

    if (len != 0 && fwrite(buf, len, 1, f) != 1) {
        return -2;
    }

    fclose(f);
    return 0;
}

int decompressor (const char filename[], Bytef in[], long int in_len, long int *out_len)
{
    Bytef *out = NULL;
    int ret;

    out = malloc(*out_len);
    if (out == NULL) {
        free(out);
        return -1;
    }

    ret = uncompress(out, out_len, in, in_len);
    if (ret != Z_OK) {
        free(out);
        return -2;
    }

    if (copy_buffer_in_file(filename, out, *out_len) < 0) {
        free(out);
        return -3;
    }

    free(out);
    return 0;
}