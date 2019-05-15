#include "streamvbyte.h"
#include "streamvbyte_zigzag.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

#include <string.h>

//#define INT16

#ifdef INT16
#define DTYPE int16_t
#else
#define DTYPE uint32_t
#endif

void punt(long long n, char *s) {
  int i = 127;
  int sign = 0;
  if (n < 0) {
    sign = 1;
    n = -n;
  }
  s[i--] = '\0'; // null terminated
  int digits = 0;
  do {
    s[i--] = n % 10 + '0';
    digits++;
    n /= 10;
    if (((digits % 3) == 0) && (n != 0))
      s[i--] = ',';

  } while (n);
  if (sign)
    s[i--] = '-';
  memmove(s, s + i + 1, 127 - i);
}

int main() {
  int M = 1000;
  int N = 10000000;
  int NTrials = 25;
  struct rusage before;
  struct rusage after;
  float t, v;
  char s[128];
  char s1[128];
  char s2[128];

  uint8_t *compressedbuffer = malloc(sizeof(uint8_t) * N * 5);

  uint32_t *datain = malloc(sizeof(uint32_t) * N);
  uint32_t *recovdata = malloc(sizeof(uint32_t) * N);

  int16_t *datain_16 = malloc(sizeof(int16_t) * N);
  int16_t *recovdata_16 = malloc(sizeof(int16_t) * N);

  int k;
  size_t compsize = 0;
  size_t compsize2;

  for (int r = 0; r < N; ++r) {
    datain[r] = rand() >> (31 & rand());
    datain_16[r] = rand() >> (15 & rand());
  }

  for (int size = M; size <= N; size*=10) {

    getrusage(RUSAGE_SELF, &before);

    #ifdef INT16
    zigzag_delta_encode((int32_t *)datain_16, datain, size, 0);
    #endif

    for (int i = 0; i < NTrials; i++)
      compsize = streamvbyte_encode(datain, size, compressedbuffer);

    getrusage(RUSAGE_SELF, &after);

    t = (after.ru_utime.tv_usec - before.ru_utime.tv_usec) / 1000000.0;
    punt((long long)round(size * NTrials / t), s);

    v = (size * NTrials * sizeof(DTYPE))/ t /1e9;
    printf("encoding time = %f s,   %s uints/sec %7.3f GB/s\n", t, s, v);


    getrusage(RUSAGE_SELF, &before);
    for (int i = 0; i < NTrials; i++) {
      compsize2 = streamvbyte_decode(compressedbuffer, recovdata, size);
    }
    #ifdef INT16
    zigzag_delta_decode(recovdata, (int32_t*)recovdata_16, size, 0);
    #endif

    getrusage(RUSAGE_SELF, &after);

    t = (after.ru_utime.tv_usec - before.ru_utime.tv_usec) / 1000000.0;
    punt((long long)round(size * NTrials / t), s);
    v = (size * NTrials * sizeof(DTYPE))/ t /1e9;
    printf("decoding time = %f s,   %s uints/sec %7.3f GB/s\n", t, s, v);
    if (compsize != compsize2) {
      printf("compsize=%zu compsize2 = %zu\n", compsize, compsize2);
    }

    for (k = 0; k < size && datain[k] == recovdata[k]; k++)
      ;

    if (k < size)
      printf("mismatch at %d before=%d after=%d\n", k, datain[k], recovdata[k]);

  }

  free(compressedbuffer);
  free(datain);
  free(recovdata);
  free(datain_16);
  free(recovdata_16);

  assert(k >= N);
  punt(N * sizeof(DTYPE), s1);
  punt(compsize, s2);
  printf("Compressed %s bytes down to %s bytes.\n", s1, s2);
  return 0;
}
