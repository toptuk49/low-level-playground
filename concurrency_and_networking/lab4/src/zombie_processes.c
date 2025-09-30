#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHILD_PROCESS 0
#define WAIT_FOR_COMPLETE 0

int main(void) {
  pid_t child_processes[10];
  int process_index;
  for (process_index = 9; process_index >= 0; process_index--) {
    child_processes[process_index] = fork();

    if (child_processes[process_index] == CHILD_PROCESS) {
      printf("Имитация деятельности процесса %d\n", process_index);
      sleep(process_index + 1);
      exit(0);
    }
  }

  for (process_index = 9; process_index >= 0; process_index--) {
    printf("Родитель ждет завершения процесса %d\n", process_index);
    waitpid(child_processes[process_index], NULL, WAIT_FOR_COMPLETE);
  }

  printf("\nРодитель в первую очередь ждал завершения самого долгого процесса, когда остальные уже завершились.\n");

  return 0;
}
