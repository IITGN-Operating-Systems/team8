#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void print_test(char *test_name)
{
    printf("=== %s ===\n", test_name);
}

void test_initial_priority()
{
    print_test("Testing Initial Priority");
    int pid = getpid();
    int prio = getPriority();
    printf("Process %d initial priority: %d\n", pid, prio);
    if (prio == 0)
        printf("PASS: Initial priority is 0\n");
    else
        printf("FAIL: Initial priority should be 0, got %d\n", prio);
}

void test_self_priority_set()
{
    print_test("Testing Self Priority Set");
    int pid = getpid();
    int ret = setPriority(pid, 5);
    if (ret == -1)
        printf("PASS: Process cannot set its own priority\n");
    else
        printf("FAIL: Process shouldn't be able to set its own priority\n");
}

void test_invalid_priority()
{
    print_test("Testing Invalid Priority Values");
    int pid = fork();

    if (pid < 0)
    {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        // Child process
        sleep(1);
        exit(0);
    }
    else
    {
        // Parent process
        int ret1 = setPriority(pid, -1);
        int ret2 = setPriority(pid, 21);

        if (ret1 == -1)
            printf("PASS: Rejected negative priority\n");
        else
            printf("FAIL: Accepted negative priority\n");

        if (ret2 == -1)
            printf("PASS: Rejected priority > 20\n");
        else
            printf("FAIL: Accepted priority > 20\n");

        wait(0);
    }
}

void test_parent_child_priority()
{
    print_test("Testing Parent-Child Priority Setting");
    int pid = fork();

    if (pid < 0)
    {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        // Child process
        sleep(2); // Give parent time to set priority
        int child_prio = getPriority();
        printf("Child priority after parent set: %d\n", child_prio);
        if (child_prio == 10)
            printf("PASS: Child priority correctly set to 10\n");
        else
            printf("FAIL: Child priority should be 10, got %d\n", child_prio);
        exit(0);
    }
    else
    {
        // Parent process
        sleep(1); // Ensure child has started
        printf("Setting child process %d priority to 10\n", pid);
        int ret = setPriority(pid, 10);
        if (ret == 0)
            printf("PASS: Successfully set child priority\n");
        else
            printf("FAIL: Failed to set child priority\n");
        wait(0);
    }
}

void test_multiple_children(void)
{
    printf("=== Testing Multiple Children Priority Setting ===\n");

    int pid1 = fork();
    if (pid1 < 0)
    {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid1 == 0)
    {
        sleep(1); // Child 1: wait a bit before checking priority (to ensure parent has set priority)
        int p = getPriority();
        printf("Child %d priority: %d\n", getpid(), p);
        if (p == 5)
        {
            printf("PASS: Child %d has correct priority 5\n", getpid());
        }
        else
        {
            printf("FAIL: Child %d should have priority 5, got %d\n", getpid(), p);
        }
        exit(0);
    }

    int pid2 = fork();
    if (pid2 < 0)
    {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid2 == 0)
    {
        sleep(2); // Child 2: wait longer before checking priority (to ensure parent has set priority)
        int p = getPriority();
        printf("Child %d priority: %d\n", getpid(), p);
        if (p == 10)
        {
            printf("PASS: Child %d has correct priority 10\n", getpid());
        }
        else
        {
            printf("FAIL: Child %d should have priority 10, got %d\n", getpid(), p);
        }
        exit(0);
    }

    int pid3 = fork();
    if (pid3 < 0)
    {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid3 == 0)
    {
        sleep(3);
        int p = getPriority();
        printf("Child %d priority: %d\n", getpid(), p);
        if (p == 15)
        {
            printf("PASS: Child %d has correct priority 15\n", getpid());
        }
        else
        {
            printf("FAIL: Child %d should have priority 15, got %d\n", getpid(), p);
        }
        exit(0);
    }

    // Parent process
    printf("Setting child process %d priority to 5\n", pid1);
    if (setPriority(pid1, 5) >= 0)
    {
        printf("PASS: Set priority 5 for child %d\n", pid1);
    }

    printf("Setting child process %d priority to 10\n", pid2);
    if (setPriority(pid2, 10) >= 0)
    {
        printf("PASS: Set priority 10 for child %d\n", pid2);
    }

    printf("Setting child process %d priority to 15\n", pid3);
    if (setPriority(pid3, 15) >= 0)
    {
        printf("PASS: Set priority 15 for child %d\n", pid3);
    }

    for (int i = 0; i < 3; i++)
    {
        wait(0);
    }
}

int run_tests(void)
{
    test_initial_priority();
    printf("\n");
    test_self_priority_set();
    printf("\n");
    test_invalid_priority();
    printf("\n");
    test_parent_child_priority();
    printf("\n");
    test_multiple_children();
    printf("\n");
    return 0;
}

int main(int argc, char *argv[])
{
    printf("\nStarting Priority System Call Tests\n\n");

    if (run_tests() == 0)
        printf("All tests passed\n");
    else
        printf("Some tests failed\n");

    exit(0);
}
