
//const size_t rand_word_num = rand() % 10 + 1; // random number beetween 1 - 10 for number words in one line

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h>     // for atoi
#include <string.h>     // for strlen

int main(int argc, char* argv[]) {

  // number of words
  size_t N = 8333333LL ;  // BSD unix: page_size = 4096 Byte //50MB file
  //size_t N = 1666666600LL ;// 9GiB file
  if( argc == 2)
    N  = (size_t) atoi(argv[1]);

  // File for output
  char *output = "/data/testdata/rand_words-many.dat";
  if( argc == 3){
    N  = (size_t) atoi(argv[1]);
    output = argv[2];
  };

  FILE *fpt;
  fpt = fopen(output, "w");

  // number of letters per word
  size_t lenWord = 5L;   // 26^5 = 11.881.376

  printf("letters per word  = %lu \n", lenWord           );
  printf("number of words   = %lu \n", N                 );


  srand( 3 ); // seed the random number generator

  for( size_t i = 1; i <= lenWord * N + 1 ; i++) {
    // for size_t (= unsigned long) calculate modulo via  A % B = A – B * (A / B)
    size_t nRandom = (size_t) rand() ;
    fprintf(fpt, "%c",  (char)(97  + nRandom%26 ) );
    size_t nr = i - lenWord * (i / lenWord);
    if( (int)nr == 0L)
	fprintf(fpt, "%c", ' ' );
  };

  fclose(fpt);

  return 0;
}
