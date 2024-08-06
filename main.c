#include "train_handler.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <MAP1|MAP2>\n", argv[0]);
    exit(1);
  }

  run(argv[1]);

  return 0;
}
