#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <prhlt/trace.h>


int start_monitoring(void) {
  while (true) {
    TRACE(1, "MONITOR: Starting monitored process ...\n");
    pid_t pid = fork();

    CHECK_SYS_ERROR(pid != (pid_t)-1, "MONITOR: Error restarting monitored process\n");

    if (pid == (pid_t)0) {
      return pid;
    }
    else {
      TRACE(1, "MONITOR: Waiting for monitored process (pid = %d) ...\n", pid);
      int status;
      errno = 0;
      pid = wait(&status);
      CHECK_SYS_ERROR(pid != (pid_t)-1, "MONITOR: Error waiting monitored process to end\n");
      // child ended correctly so exit the program
      if (!WIFSIGNALED(status)) {
        TRACE(1, "MONITOR: Monitored process exited successfully with return code %d\n", WEXITSTATUS(status));
        exit(WEXITSTATUS(status));
      }
      // child crashed. Restarting the program
      TRACE(1, "MONITOR: Monitored process terminated unexpectedly with signal %d. Restarting process ...\n", WTERMSIG(status));
    }
  }

  // this should not happen
  exit(EXIT_FAILURE);

}
