
//const size_t rand_word_num = rand() % 10 + 1; // random number beetween 1 - 10 for number words in one line

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h>     // for atoi
#include <unistd.h>     // for long sysconf(_SC_PAGESIZE)

int main(int argc, char* argv[]) {

  static const char wordList[][10] = {"aaa ", "A_A ", "bbb ", "B_B " , "ccc ", "C_C "};

  // number of words
  size_t N = 12500000L ;  // BSD unix: page_size = 4096 Byte 50 mb file
  //size_t N = 2500000000L; //9.4GB file

  if( argc == 2)
     N  = (size_t) atoi(argv[1]);

  // File for output
  char *output = "/data/testdata/rand_words-2.dat";
  if( argc == 3){
    N  = (size_t) atoi(argv[1]);
    output = argv[2];
  };


  size_t i, j;
  size_t n1 = 0L, n2 = 0L;
  FILE *fpt;

  fpt = fopen(output, "w");

  srand( 3 ); // seed the random number generator
  for(i = 0; i < N; i++) {
      size_t rand_word = rand() % 2 ; // random number beetween 0 and 1
      // if (i >= 1024) rand_word = rand_word + 2;
      // if (i >= 2048) rand_word = rand_word + 2;
      fprintf(fpt, "%s", wordList[rand_word ] );
      if( rand_word == 0 ) n1++;
      else                 n2++;
  };

  fclose(fpt);

  printf("Number of words: \n %s = %lu \n %s = %lu\n",  wordList[0], n1, wordList[1], n2);

  return 0;
}
