#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/wait.h>

#define true 1
#define false 0
#define bool int

typedef int error_code;

#define ERROR (-1)
#define HAS_ERROR(code) ((code) < 0)
#define NULL_TERMINATOR '\0'

error_code readLine(char **out) {
    size_t size = 10;                       // size of the char array
    char *line = malloc(sizeof(char) * size);       // initialize a ten-char line
    if (line == NULL) return ERROR;   // if we can't, terminate because of a memory issue

    for (int at = 0; 1; at++) {
        if (at >= size) {
            size = 2 * size;
            char *tempPtr = realloc(line, size * sizeof(char));
            if (tempPtr == NULL) {
                free(line);
                return ERROR;
            }

            line = tempPtr;
        }

        int tempCh = getchar();

        if (tempCh == EOF) { // if the next char is EOF, terminate
            free(line);
            return ERROR;
        }

        char ch = (char) tempCh;

        if (ch == '\n') {        // if we get a newline
            line[at] = NULL_TERMINATOR;    // finish the line with return 0
            break;
        }


        line[at] = ch; // sets ch at the current index and increments the index
    }

    out[0] = line;
    return 0;
}

enum op {
    BIDON, NONE, OR, AND, ALSO
};

struct command {
    char **call;
    enum op operator;
    struct command *next;
    int count;
    bool also;
};

void freeStringArray(char **arr) {
    if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    }
    free(arr);
}

struct command *freeAndNext(struct command *current) {
    if (current == NULL) return current;

    struct command *next = current->next;
    freeStringArray(current->call);
    free(current);
    return next;
}

void freeCommands(struct command *head) {
    struct command *current = head;
    while (current != NULL) current = freeAndNext(current);
}

error_code nextCommands(char *line, struct command **out) {

    char **call = NULL;
    char *wordPtr = NULL;
    struct command *c = NULL;

    int currentCallSize = 0;
    struct command *head = NULL;
    struct command *current = NULL;

    int escaped = 0;

    for (int index = 0; line[index] != NULL_TERMINATOR; index++) {
        int nextSpace = index;
        while (1) {
            if (!escaped) {
                if (line[nextSpace] == ' ' || NULL_TERMINATOR == line[nextSpace]) {
                    break;
                }
            }
            if (line[nextSpace] == '(') escaped = 1;
            if (line[nextSpace] == ')') escaped = 0;
            nextSpace++;
        }

        wordPtr = malloc(sizeof(char) * (nextSpace - index + 1));
        if (wordPtr == NULL) goto error;

        int i = 0;
        for (; i < nextSpace - index; i++) wordPtr[i] = line[i + index];
        wordPtr[i] = NULL_TERMINATOR;

        enum op operator = BIDON;
        if (strcmp(wordPtr, "||") == 0) operator = OR;
        if (strcmp(wordPtr, "&&") == 0) operator = AND;
        if (strcmp(wordPtr, "&") == 0) operator = ALSO;
        if (operator != ALSO && line[nextSpace] == NULL_TERMINATOR) operator = NONE;

        if (operator == OR || operator == AND || operator == ALSO) free(wordPtr);

        if (operator == BIDON || operator == NONE) {
            if (call == NULL) {
                currentCallSize = 1;
                call = malloc(sizeof(char *) * 2);
                if (call == NULL) goto error;
                call[1] = NULL;
            } else {
                currentCallSize++;
                char **tempPtr = realloc(call, (currentCallSize + 1) * sizeof(char *));
                if (tempPtr == NULL) goto error;

                call = tempPtr;
                call[currentCallSize] = NULL;
            }

            call[currentCallSize - 1] = wordPtr;
        }

        if (operator != BIDON) {
            c = malloc(sizeof(struct command));
            if (c == NULL) goto error;

            if (head == NULL) head = c;
            else current->next = c;

            c->count = 1;
            if (call[0][strlen(call[0]) - 1] == ')') {
                char *command = call[0];

                unsigned long command_len = strlen(command); //rn et fn ici

                int paren_pos = 0;
                for (; command[paren_pos] != '('; paren_pos++);

                wordPtr = malloc(paren_pos * sizeof(char));
                if (wordPtr == NULL) goto error;

                for (int j = 0; j < paren_pos; j++) wordPtr[j] = command[j + 1];
                wordPtr[paren_pos - 1] = NULL_TERMINATOR;

                int nb = atoi(wordPtr); // NOLINT(cert-err34-c)
                c->count = ('r' == command[0]) ? nb : -nb;

                free(wordPtr);
                if (NULL == (wordPtr = malloc(sizeof(char) * (command_len - paren_pos - 1)))) {
                    goto error;
                }

                memcpy(wordPtr, &command[paren_pos + 1], command_len - paren_pos - 2);
                wordPtr[command_len - paren_pos - 2] = NULL_TERMINATOR;
                free(command);

                unsigned long resized_len = strlen(wordPtr);

                int space_nb = 0;
                for (int j = 0; j < resized_len; j++) {
                    space_nb += wordPtr[j] == ' ';
                }

                char **temp;
                if (NULL == (temp = realloc(call, sizeof(char *) * (space_nb + 2)))) {
                    goto error;
                } else {
                    call = temp;
                    call[space_nb + 1] = NULL;
                }

                int copy_index = 0;
                char *token = strtok(wordPtr, " ");
                while (token != NULL) {
                    char *arg = malloc(sizeof(char) * (strlen(token) + 1));
                    /* if (NULL == (arg )) {
                         goto error;
                     }*/

                    strcpy(arg, token);
                    call[copy_index] = arg;

                    token = strtok(NULL, " ");

                    copy_index++;
                }

                free(wordPtr);
            }

            c->call = call;

            c->next = NULL;
            c->operator = operator;
            c->also = false;
            if (operator == ALSO) {
                c->operator = NONE;
                head->also = true;
            }

            current = c;

            call = NULL;

        }

        if (line[nextSpace] == NULL_TERMINATOR) break;
        index = nextSpace;
    }

    out[0] = head;
    return 0;

    error:
    free(wordPtr);
    free(line);
    freeStringArray(call);
    freeAndNext(c);
    freeCommands(head);
    return ERROR;
}

int callCommand(struct command *current) {
    if (current->count <= 0) return 1;

    int exitCode = -1;     // the exit code of the child
    pid_t pid = 0;
    for (int i = 0; i < current->count; i++) {
        pid = fork();

        if (pid < 0) {        // forking failed
            return pid;
        } else if (pid == 0) {
            // -----------------------------------------------------
            //                    CHILD PROCESS
            // -----------------------------------------------------
            char *cmd_name = current->call[0];
            execvp(cmd_name, current->call);    // execvp searches for command[0] in PATH, and then calls command

            printf("bash: %s: command not found\n", cmd_name);    // if we reach this, exec couldn't find the command

            exit(49);    // and then we exit with 2, signaling an error. This also frees ressources
        }
    }

    // PID is correct
    waitpid(pid, &exitCode, 0); // Wait for the child process to exit.
    int x = WIFEXITED(exitCode);
    if (!x) return 0;

    x = WEXITSTATUS(exitCode);
    if (x != 0) return 0;
    return 1;
}

error_code callCommands(struct command *current) {
    if (current == NULL || current->call == NULL) return 0;

    int ret = callCommand(current);
    if (ret == -1) return ERROR;

    enum op operator = current->operator;
    struct command *next = current->next;

    switch (operator) {
        default:
        case NONE:
        case BIDON:
            return 0;
        case AND:
            if (ret) return callCommands(next);
            return 0;
        case OR:
            if (ret) {
                while (next != NULL && (next->operator == OR || next->operator == NONE)) next = next->next;
                if (next != NULL && next->operator == AND) next = next->next;
            }

            return callCommands(next);
    }
}

error_code shell() {

    char *line;
    struct command *head;
    while (1) {
        if (HAS_ERROR(readLine(&line))) goto top;
        if (strlen(line) == 0) continue;

        if (HAS_ERROR(nextCommands(line, &head))) goto bot;
        free(line);

        if (head->also) {
            pid_t pid = fork();
            if (pid == -1) {        // forking failed
                freeCommands(head);
                return ERROR;
            } else if (pid == 0) {    // child
                if (HAS_ERROR(callCommands(head))) goto toptop;
                exit(0);    // and then we exit with 2, signaling an error
            }

        } else callCommands(head);

        freeCommands(head);
    }

    toptop:
    freeCommands(head);
    bot:
    free(line);
    top:
    printf("An error has occured");
    exit(-1);
}

//BEGIN COPY ༽つ۞﹏۞༼つ
int main() {
	shell();
}
//END COPY ༽つ۞﹏۞༼つ