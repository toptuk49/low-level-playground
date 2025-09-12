#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  pid_t pid = fork();

  if (pid == -1) {
    perror("Fork failed");
    return 1;
  }

  if (pid == 0) {
    char *program = "./sequential_min_max";
    char *args[] = {program, argv[1], argv[2], NULL};

    execvp(program, args);
    perror("execvp failed");
    return 1;
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      printf("Child process finished with exit code %d\n", WEXITSTATUS(status));
    } else {
      printf("Child process did not terminate normally\n");
    }
  }

  return 0;
}


