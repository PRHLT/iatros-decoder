#include <string.h>

int die() {
  char *err = (char *)0x0;
  strcpy(err, "gonner");
  return 0;
}

void setup_sigsegv();
int main() {
  setup_sigsegv();
  return die();
}
