#ifndef GZLIB_H
#define GZLIB_H

#ifdef __cplusplus
extern "C" {
#endif
  
FILE* igzstream_open(const char* filename);
void igzstream_read(FILE* in, char* buf, long size);
void igzstream_close(FILE* in);
int igzstream_feof(FILE *in);
int igzstream_fail(FILE *in);
  
#ifdef __cplusplus
}
#endif
  
#endif
