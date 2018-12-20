#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>

#define MALLOC_FAIL "Malloc failure - exit with error\n"
#define SYS_FAIL "SYSCALL FAIL: %s"


// This is a function for replacing an old String pattern in a new String pattern
// with the helped of Geekout website
int replaceWord(const char *s, const char *oldW,
                  const char *newW) {
    char *result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);

    // Counting the number of times old word occur in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;
            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }

    // Making new string of enough length
    result = (char *) malloc((size_t) (i + cnt * (newWlen - oldWlen) + 1));
    if ( result == NULL ){
        printf(MALLOC_FAIL);
        return 0;
    }
    i = 0;

    while (*s) {
        // compare the substring with the result
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else{
            result[i++] = *s++;
        }
    }
    result[i] = '\0';
    printf("%s", result);
    free(result);
    return 1;
}

// This function concate the path to the file
char *calculatePath(char *env1 , char *env2){
    char *result = (char*) malloc(3000);
    if (result == NULL){
        return NULL;
    }
    strcpy(result , env1);
    strcat(result , "/");
    strcat(result , env2);
    return result;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Not enough parameters!\n");
        return 1;
    }
    char *firstStr = argv[1];
    char *secondStr = argv[2];
    char *env1 = getenv("HW1DIR");
    char *env2 = getenv("HW1TF");
    if (env1 == NULL || env2 == NULL) {
        printf("One of the environment variable is not defined\n");
        return 1;
    }
    char *address = calculatePath(env1, env2);
    if (address == NULL){
        printf(MALLOC_FAIL);
        return 1;
    }
    struct stat fileStat;
    if (stat(address, &fileStat) < 0) {
        printf(SYS_FAIL, "STAT\n");
        return 1;
    }
    int fileSize = (int) fileStat.st_size;
    int fd = open(address, O_RDONLY);
    if (fd < 0) {
        printf(SYS_FAIL , "OPEN\n");
        free(address);
        return 1;
    }
    // This part read the entire file
    char buf[fileSize + 1];
    char tempbuf[fileSize + 1];
    int tempInd = 0;
    ssize_t len = fileSize;
    int realFileSize = 0;
    while (len > 0) {
        ssize_t curr = read(fd, tempbuf, (unsigned int) fileSize);
        if (curr == -1){
            printf(SYS_FAIL , "READ\n");
            free(address);
        }
        realFileSize = curr + realFileSize;
        if (curr == 0) {
            break;
        }
        len = len - curr;
        lseek(fd, len, curr);
        for (int i = tempInd; i < realFileSize; i++) {
            buf[i] = tempbuf[i];
        }
        tempInd = curr + tempInd;
    }
    buf[realFileSize] = '\0';
    int i = replaceWord(buf, firstStr, secondStr);
    if (i == 0) {
        printf(MALLOC_FAIL);
        free(address);
        close(fd);
        return 1;
    }
    free(address);
    close(fd);
    return 0;
}



