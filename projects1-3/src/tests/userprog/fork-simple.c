#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void) {
  int a = 1;
  pid_t pid = fork();
  if (pid < 0)
    fail("fork returned %d", pid);
  else if (pid == 0) {
    a--;
    msg("Child sees a as %d", a);
  } else {
    a++;
    wait(pid);
    msg("Parent sees a as %d", a);
  }
}
