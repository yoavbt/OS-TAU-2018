#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

// Queue of Directories implemented as a linked list
typedef struct dir_node
{
    DIR *dir;
    struct dir_node *next;
    char path[256];
} dir_node;

// The Global Directories Queue
dir_node *entry = NULL;

//Number of sleeping threads - meant for detecting the end of scanning
int number_of_sleeping_threads = 0;
// Number of threads
int num_of_threads = 0;
// Number of total files that found
int total_files = 0;
// The Term to search
char *term = 0;
// Array of threads
pthread_t *thread_ids;
// The queue Lock for consistency
pthread_mutex_t queue_mutex;
// The condition threshold
pthread_cond_t queue_threshold;

/******** Queue helping functions ***/
// Add dir to the queue
int add_dir_to_queue(DIR *dir, char *path)
{
    // If the queue is empty
    if (!entry)
    {
        dir_node *curr;
        curr = (dir_node *)malloc(sizeof(dir_node));
        curr->dir = dir;
        strcpy(curr->path, path);
        curr->next = NULL;
        entry = curr;
        return 0;
    }
    // If the queue is not empty
    dir_node *next;
    dir_node *curr;
    next = (dir_node *)malloc(sizeof(dir_node));
    if (!thread_ids)
    {
        perror("Malloc failed.\n");
        return 1;
    }
    next->dir = dir;
    strcpy(next->path, path);
    curr = entry;
    while (curr->next)
    {
        curr = curr->next;
    }
    curr->next = next;
    next->next = NULL;
    return 0;
}

// Poping a directory from the Queue
dir_node *pull_dir_from_queue()
{
    if (!entry)
    {
        return entry;
    }
    dir_node *temp = entry;
    entry = entry->next;
    return temp;
}
// Free queue Resources
void free_queue(dir_node *enrty)
{
    while (enrty != NULL)
    {
        free_queue(entry->next);
        free(enrty);
    }
}

// The handler for the SIGINT signal
static void hdl(int sig, siginfo_t *siginfo, void *context)
{
    for (int i = 0; i < num_of_threads; i++)
    {
        pthread_cancel(thread_ids[i]);
    }
    printf("Search stopped, found %d files.\n", total_files);
    if (!thread_ids){
        free(thread_ids);
    }
    if(!term){
        free(term);
    }
    exit(0);
}

// Check whether a file is a folder or not
int is_folder(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

/****** Thread function ****/

void *thread_func(void *thread_param)
{
    while (1)
    {
        int rc;
        // Locking the pull
        rc = pthread_mutex_lock(&queue_mutex);
        if (0 != rc)
        {
            printf("ERROR in pthread_mutex_lock(): "
                   "%s\n",
                   strerror(rc));
            pthread_exit(NULL);
        }
        dir_node *curr_node = pull_dir_from_queue(entry);
        rc = pthread_mutex_unlock(&queue_mutex);
        if (0 != rc)
        {
            printf("ERROR in pthread_mutex_unlock(): "
                   "%s\n",
                   strerror(rc));
            exit(1);
        }
        //make thread sleep if queue empty
        while (!curr_node)
        {
            rc = pthread_mutex_lock(&queue_mutex);
            // Error is not critical - application can work
            if (0 != rc)
            {
                printf("ERROR in pthread_mutex_lock(): "
                       "%s\n",
                       strerror(rc));
                pthread_exit(NULL);
            }
            number_of_sleeping_threads += 1;
            pthread_cond_wait(&queue_threshold, &queue_mutex);
            curr_node = pull_dir_from_queue(entry);
            number_of_sleeping_threads += -1;
            rc = pthread_mutex_unlock(&queue_mutex);
            // Error is crucial since we took a folder from the queue
            if (0 != rc)
            {
                printf("ERROR in pthread_mutex_unlock(): "
                       "%s\n",
                       strerror(rc));
                exit(1);
            }
        }
        struct dirent *dp;
        while ((dp = readdir(curr_node->dir)) != NULL)
        {
            char new_path[256];
            strcpy(new_path, curr_node->path);
            strcat(strcat(new_path, "/"), dp->d_name);
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            {
                //Ignore ".." and "."
            }
            else
            {
                if (is_folder(new_path))
                {
                    // push folder to the queue and wake up a thread
                    pthread_mutex_lock(&queue_mutex);
                    DIR *dir;
                    dir = opendir(new_path);
                    if ((add_dir_to_queue(dir, new_path)) == 1)
                    {
                        perror("Malloc failed.\n");
                        exit(1);
                    }
                    pthread_cond_signal(&queue_threshold);
                    pthread_mutex_unlock(&queue_mutex);
                }
                else
                {
                    // search for file name a print the path
                    if (strstr(dp->d_name, term) != NULL)
                    {
                        printf("%s\n", new_path);
                        __sync_fetch_and_add(&total_files, 1);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    struct sigaction act;

    memset(&act, '\0', sizeof(act));

    /* Use the sa_sigaction field because the handles has two additional parameters */
    act.sa_sigaction = &hdl;

    /* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        fprintf(stderr, "sigaction had failed.\n");
        return 1;
    }

    //Initialize mutex and condition variable objects
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_threshold, NULL);
    // put the root directory inside the queue
    term = (char *)malloc(sizeof(char) * strlen(argv[2]));
    if (!term)
    {
        perror("Malloc failed.\n");
        return 1;
    }
    strcpy(term, argv[2]);
    DIR *dir;
    dir = opendir(argv[1]);
    if ((add_dir_to_queue(dir, argv[1])) == 1)
    {
        perror("Malloc failed.\n");
        // free resources
        free(term);
        free_queue(entry);
        return 1;
    }
    // Create the threads
    num_of_threads = atoi(argv[3]);
    thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * num_of_threads);
    if (!thread_ids)
    {
        perror("Malloc failed.\n");
        return 1;
    }
    for (int i = 0; i < num_of_threads; i++)
    {
        int rc = pthread_create(&thread_ids[i], NULL, thread_func, NULL);
        if (rc)
        {
            fprintf(stderr, "ERROR in pthread_create\n");
            // free resources
            free(thread_ids);
            free(term);
            return 1;
        }
    }
    // The condition for end of searching
    while (number_of_sleeping_threads != num_of_threads)
    {
    }
    // free resources
    free(thread_ids);
    free(term);
    free_queue(entry);
    printf("Done searching, found %d files\n", total_files);
    return 0;
}
