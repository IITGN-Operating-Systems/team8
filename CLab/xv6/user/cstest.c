#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void busy_wait(int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < 5000000; j++)
        { //
            asm volatile("nop");
        }
    }
}

int main(void)
{
    int parent_pid = getpid();
    printf("Starting Context Switch Test (Parent PID: %d)\n", parent_pid);
    sleep(10);

    int num_children = 3;

    for (int i = 0; i < num_children; i++)
    {
        int pid = fork();
        if (pid < 0)
        {
            printf("Fork failed!\n");
            exit(1);
        }

        if (pid == 0)
        {
            printf("Child %d (PID: %d) starting\n", i, getpid());

            for (int j = 0; j < 5; j++)
            {
                busy_wait(i + 1);
                printf("Child %d (PID: %d) completed iteration %d\n",
                       i, getpid(), j);
                sleep(5);
            }

            exit(0);
        }
        else
        {
            printf("Parent created Child %d (PID: %d)\n", i, pid);
        }
    }

    printf("Parent process (PID: %d) working...\n", parent_pid);
    for (int i = 0; i < 3; i++)
    {
        busy_wait(2);
        printf("Parent (PID: %d) completed iteration %d\n",
               parent_pid, i);
        sleep(2);
    }

    int status;
    for (int i = 0; i < num_children; i++)
    {
        wait(&status);
    }

    printf("Context Switch Test completed\n");
    exit(0);
}
