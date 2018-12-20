#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

/****
 **** Help functions for working with arglist 
****/

// Check for an Ampersand sign in a command - return 1 for true and 0 for false
int isContainAmper(int len, char **args)
{
    for (int i = 0; i < len; i++)
    {
        if (strcmp(args[i], "&") == 0)
            return 1;
    }
    return 0;
}

// Check for an "|" sign in a command - return the index of the "|" sign if true and -1 for false
int isContainPipe(int len, char **args)
{
    for (int i = 0; i < len; i++)
    {
        if (strcmp(args[i], "|") == 0)
            return i;
    }
    return -1;
}

/**
 * Different mode functions - three modes are available: 1.Regular Mode (foreground process)
 *                                                       2.Background Mode -"&"- (background process)
 *                                                       3.Pipe Mode -"|"- (2 foreground process and connecting output of
 *                                                                      A to input of process B)
*/

int activePipeMode(int count, char **arglist, int isPipe, struct sigaction new_action2, struct sigaction new_action)
{
    arglist[isPipe] = NULL; // Separting the two commands
    int pd[2];
    int p = pipe(pd);
    int wstatus;
    if (p < 0)
    {
        perror("Failed Piping");
        exit(1);
    }
    pid_t cpid = fork();
    if (cpid == -1)
    {
        perror("Failed forking");
        exit(EXIT_FAILURE);
    }
    // Son Code
    if (cpid == 0)
    {
        if (0 != sigaction(SIGINT, &new_action2, &new_action))
            {
                printf("Signal handle registration "
                       "failed. %s\n",
                       strerror(errno));
                exit(EXIT_FAILURE);;
            }
        dup2(pd[1], 1); // the first son process execute the first command - read from stdin and write to buffer
        if (execvp(arglist[0], arglist) < 0)
        {
            perror("failed to execute the Command");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // wait for the first process to finish its execution
        do
        {
            pid_t w = waitpid(cpid, &wstatus, WNOHANG);
            if (w == -1)
            {
            // perror("waitpid"); Ignored due to the forum note - http://moodle.tau.ac.il/mod/forum/discuss.php?d=17262
                return 1;
            }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        // close the writing buffer entry
        close(pd[1]);
        pid_t cpid = fork(); // the second son process execute the second command - read from buffer and write to stdout
        if (cpid == 0)
        {

            if (0 != sigaction(SIGINT, &new_action2, &new_action))
            {
                printf("Signal handle registration "
                       "failed. %s\n",
                       strerror(errno));
                exit(EXIT_FAILURE);;
            }
            dup2(pd[0], 0);
            if (execvp(arglist[isPipe + 1], &arglist[isPipe + 1]) < 0)
            {
                perror("failed to execute the Command");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // wait for the second process to finish its execution
            do
            {
                pid_t w = waitpid(cpid, &wstatus, 0);
                if (w == -1)
                {
            // perror("waitpid"); Ignored due to the forum note - http://moodle.tau.ac.il/mod/forum/discuss.php?d=17262
                    return 1;
                }
            } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
            close(pd[0]);
            return 1;
        }
    }
    return 1; // just for compilation purpose
}

int activeAmperMode(int count, char **arglist)
{
    arglist[count - 1] = NULL;
    pid_t cpid = fork();
    if (cpid == -1)
    {
        perror("Failed forking");
        exit(EXIT_FAILURE);
    }
    // Son Code
    if (cpid == 0)
    {
        if (execvp(arglist[0], arglist) < 0)
        { /* execute the command  */
            perror("failed to execute the Command");
            exit(EXIT_FAILURE);
        }
    }
    // Father Code
    else
    {
        return 1;
    }
    return 1; // just for compilation purpose
}

int activeNormalMode(char **arglist, struct sigaction new_action2, struct sigaction new_action)
{
    int wstatus;
    pid_t cpid = fork();
    if (cpid == -1)
    {
        perror("Failed forking");
        exit(EXIT_FAILURE);
    }
    // Son Code
    if (cpid == 0)
    {

        if (0 != sigaction(SIGINT, &new_action2, &new_action))
        {
            printf("Signal handle registration "
                   "failed. %s\n",
                   strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (execvp(arglist[0], arglist) < 0)
        { /* execute the command  */
            perror("failed to execute the Command");
            exit(EXIT_FAILURE);
        }
    }
    // Father Code
    else
    {
        pid_t w = waitpid(cpid, &wstatus, 0);
        if (w == -1)
        {
            // perror("waitpid"); Ignored due to the forum note - http://moodle.tau.ac.il/mod/forum/discuss.php?d=17262
            return 1;
        }
        return 1;
    }

    return 1; // just for compilation purpose
}
// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should cotinue, 0 otherwise

int process_arglist(int count, char **arglist)
{
    // Defining the structure of the handlers
    struct sigaction ignore_sigint; // ignore the sigint for myshell and background process
    ignore_sigint.sa_handler = SIG_IGN;
    struct sigaction handle_sigint; // handle sigint (terminate) for Foreground process in noram and pipe mode
    handle_sigint.sa_handler = SIG_DFL;
    // Register the sigint for the myshell
    if (0 != sigaction(SIGINT, &ignore_sigint, NULL))
    {
        printf("Signal handle registration "
               "failed. %s\n",
               strerror(errno));
        return -1;
    }
    // Ignore the SIGCHLD signal to avoid zombies - there is a con to it - The waitpid will throw an EICHLD error
    // that will be ignored - as Adam answered in the forum.
    struct sigaction sigchld_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT};
    if (0 != sigaction(SIGCHLD, &sigchld_action, NULL))
    {
        printf("Signal handle registration "
               "failed. %s\n",
               strerror(errno));
        return -1;
    }

    // Understanding the command mode
    int isAmper = isContainAmper(count, arglist); // 1 or 0 for Amper sing
    int isPipe = isContainPipe(count, arglist);   // the index of "|" or -1

    // Foreground child mode - no pipe or background processes
    if ((isPipe == -1) && (!isAmper))
    {
        return activeNormalMode(arglist, handle_sigint, ignore_sigint);
    }

    // Background child mode - no pipe process (By definition of the exercise)
    else if (isAmper)
    {
        return activeAmperMode(count, arglist);
    }

    // Pipe Mode - No background processes (By definition of the exercise)
    else
    {
        return activePipeMode(count, arglist, isPipe, handle_sigint, ignore_sigint);
    }
}

// set up the handler for the sigint for the function getline (read syscall)
int prepare(void)
{
    struct sigaction kernal_action;
    memset(&kernal_action, 0, sizeof(kernal_action));
    kernal_action.sa_handler = SIG_IGN; // ignore the sigint signal
    kernal_action.sa_flags = SA_RESTART;
    return sigaction(SIGINT, &kernal_action, NULL); // 0 upon success, -1 upon failure
};

// No need to free memset structure and therefore nothing to finalize
int finalize(void)
{
    return 0;
};
