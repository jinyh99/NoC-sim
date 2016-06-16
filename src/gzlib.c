#include <stdio.h>
#include "gzstream.h"
#include "gzlib.h"

using namespace std;

extern "C"
FILE* igzstream_open(const char* filename)
{
  igzstream *igz = new igzstream(filename, ios::in | ios::binary);

  if ( igz != NULL ) {
    return (FILE *)igz;
  }
  else {
    return NULL;
  }
}

extern "C"
void igzstream_read(FILE* in, char* buf, long size)
{
  igzstream *igz = (igzstream *) in;
  
  if ( in != NULL ) {
    igz->read(buf, size);
  }
}

extern "C"
void igzstream_close(FILE* in)
{
  if (in) {
    igzstream *igz = (igzstream *) in;
    delete igz;
  }
}

extern "C"
int igzstream_feof(FILE *in)
{
  igzstream *igz = (igzstream *) in;
  
  return igz->eof();
}

extern "C"
int igzstream_fail(FILE *in)
{
  igzstream *igz = (igzstream *) in;
  
  return igz->fail();
}
