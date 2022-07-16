/* This library will be loaded to memory */

#include <stdio.h>

int sample_function(void);

int sample_function(void) {
  fprintf(stderr,"===> Test module from SO file loaded and executed!!!\n");
  return 123;
}
