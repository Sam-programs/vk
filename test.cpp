#include <stdlib.h>//exit
#include <stdio.h>//printf
#include "test.h"//Test Macro
#include <unistd.h>//sleep

void run_tests(void){
}

__attribute__((constructor))
void testentry(int argc,char **argv){
  printline("Testing",COL_CYN,"");
  run_tests();
  //this won't get refactored as it's a special line
  printf(BOLD "[" COL_CYN "Done" COL_CLR "] ");
  printf(COL_GRN "%d" COL_CLR " Tested | ",testcount);
  printf(COL_GRN "%d" COL_CLR " Passed | ",testcount - failed);
  printf(COL_RED "%d" COL_CLR " Failed   ",failed);
  printf(COL_BOLD_CLR);
  printf("\n");
  exit(0);
}
