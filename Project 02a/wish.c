// Import libraries
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

char* PATH [100]; // PATH array

// Function calls
void error();
void cd(char** args);
void path(char** args);
int split_line(char* line, char** token);
int execute(char** args, char** retArgs);
void redirect(char* redir, char* line);
int executeFile(char** args, char** retArgs);
int stringCompare(char** tokens);
int detection(char** temp);
int read_line(char** args, FILE* fp);

// Error function used for reporting issues
void error() {
    char error_message[30] = "An error has occurred\n"; // Error message
    write(STDERR_FILENO, error_message, strlen(error_message)); // Write error message to stderr
}

// Change directory function used for changing directory
void cd(char** args) {
    // Variable initialization
    int status = chdir(args[1]); 

    if(status == -1) { // If error
        error();
    }
}

// Path function used for setting the PATH
void path(char** args) {
    // Variable initialization
    int i = 0;

    while(1) { // Loop through args
        if((PATH[i] = args[i + 1]) == NULL) { // If args[i + 1] is NULL, break
            break;
        }
        i++;
    }
}

// Split line function used for splitting the line into tokens
int split_line(char* line, char** token) {
    // Variable initialization
    int i = 0;
    token[i] = strtok(line, " \t\r\n\a"); // Tokenize the line

    if(token[i] == NULL) { // If token is NULL, return 0
        return -1;
    }
    i++;
    while(1) { // Loop through line
        token[i] = strtok(NULL, " \t\r\n\a"); // Tokenize the line
        if(token[i] == NULL) { // If token is NULL, break
            break;
        }
        i++;
    }

    return i;
}

// Execute function used for executing commands
int execute(char** args, char** retArgs) {
    // Variable initializations
    int status = 0, pid = 0, i = 0, j = 0;
    char* match = malloc(100 * sizeof(char*));

    if(args[0] == NULL) { // If args[0] is NULL, return 0
        return 1;
    }
    while(1) { // Loop through PATH
        if(PATH[i] == NULL) { // If PATH[i] is NULL, break
            break;
        }
        char* temp = malloc(sizeof(char) * 100); // Temporary memory allocation
        if(!strcpy(temp, PATH[i])) { // If error
            error();
            return 1;
        }
        // Add elements to directory
        strcat(temp, "/"); 
        strcat(temp, "\0");
        strcat(temp, args[0]);
        if(access(temp, X_OK) == 0) { // If file is executable
            strcpy(match, temp); // Copy temp to match
            j = 1;
            free(temp); // Free temp
            break;
        }
        free(temp); // Free temporary memory
        i++;
    }
    if(j != 1) { // If file is not executable
        error();
        return 1;
    }
    pid = fork(); // Fork process
    if(pid == 0) { // If child process
        if(retArgs) { // If retArgs is not NULL
            int fp = open(retArgs[0], O_CREAT|O_WRONLY|O_TRUNC, 0644);
            if(fp > 0) { // If file_out is greater than 0
                dup2(fp, STDOUT_FILENO); // Duplicate file_out to stdout
                fflush(stdout); // Flush stdout
            }
        }
        execv(match, args); // Execute file
    } else if( pid < 0) { // If error
        error();
        return 1;
    } else { // If parent process
        waitpid(pid, &status, 0);
    }

    return 0;
}

// Redirect function used for redirecting output
void redirect(char* redir, char* line) {
    // Variable initializations
    char* args[100], * redirArgs[100];
    redir[0] = '\0';
    redir++;

    if (split_line(line, args) == -1) { // If error
        error();
        return;
    }
    if (split_line(redir, redirArgs) != 1) { // If error
        error();
        return;
    }
    execute(args, redirArgs); // Execute command
}

// Execute File function used for executing commands from a given file
int executeFile(char** args, char** retArgs) {
    // Variable initializations
    int pid = 0, status = 0, i = 0, j = 0, k = 0;
    char* match = malloc(100 * sizeof(char));

    if(args[0] == NULL) { // If args[0] is NULL, return 0
        return 1;
    }
    while(1) { // Loop through PATH
        if(PATH[i] == NULL) { // If PATH[i] is NULL, break
            break;
        }
        char* temp = malloc(sizeof(char) * 100); // Temporary memory allocation
        if(!strcpy(temp, PATH[i])) { // If error
            error();
            return 1;
        }
        // Add elements to directory
        strcat(temp, "/");
        strcat(temp, "\0");
        strcat(temp, args[0]);
        if(access(temp, X_OK) == 0) { // If file is executable
            strcpy(match, temp);
            j = 1;
            free(temp);
            break;
        }
        free(temp); // Free temporary memory
        i++;
    }
    if(!j) { // If file is not executable
        error();
        return 1;
    }
    pid = fork(); // Fork process
    if(pid == 0) { // If child process
        if(retArgs) { // If retArgs is not NULL
            int fp = open(retArgs[0],O_CREAT|O_WRONLY|O_TRUNC, 0644);
            if(fp > 0) { // If fp_out is greater than 0
                dup2(fp, STDOUT_FILENO); // Duplicate fp_out to stdout
                fflush(stdout); // Flush stdout
            }
        }
        k = execv(match, args); // Execute file
    } else if( pid < 0) { // If error
        error();
        return 1;
    } else { // If parent process
        wait(&status); // Wait for child process
        k = WEXITSTATUS(status); // Get exit status
        return k;
    }

    return 1;
}

// String Compare function used for comparing strings
int stringCompare(char** token) {
    // Variable initialization
    char* command[10];
    int output = 0, token_length = 0, i = 0;

    for(i = 0; i < 10; i++) { // Loop through tokens
        command[i] = '\0';
    }
    while(1) { // Loop through tokens
        if(token[token_length] == NULL) { // If tokens[tokens_length] is NULL, break
            break;
        }
        token_length++;
    }
    int result = atoi(token[token_length - 1]); // Convert string to integer
    for(i = 0; i < token_length - 2; i++) { // Loop through tokens
        command[i] = token[i];
    }
    output = executeFile(command, NULL); // Run command
    if(strcmp(token[token_length - 2], "==") == 0) { // If equals
        if(output == result) { // If output is equal to result
            return 0;
        }
    }
    if(strcmp(token[token_length - 2], "!=") == 0) { // If not equals
        if(output != result) { // If not equals to result
            return 0;
        }
    }

    return 1;
}

// Detection function used for detecting commands
int detection(char** temp) {
    // Variable initialization
    int size = 2, redir = 0, i = 0, j = 0, status = 0;
    char spacer[256] = "";

    for (i = 0; i < size; i++) { // Loop through tokens
        while (temp[j]) { // Loop through tokens
            if (strcmp(temp[j], ">") == 0) { // If greater than
                redir = j;
                break;
            }
            j++;
        }
        // Add elements to directory
        strcat(spacer, PATH[i]); 
        strcat(spacer, "/");
        strcat(spacer, temp[0]);
        if(access(spacer, X_OK) == 0) { // If file is executable
            int pid = fork();
            if (pid == 0) { // If child process
                if (redir) { // If redir is not 0
                    int fp = open(temp[redir + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open file
                    dup2(fp, fileno(stdout)); // Duplicate file to stdout         
                    close(fp); // Close file
                    temp[redir] = NULL; // Set temp[redir] to NULL
                }
                execv(spacer, temp); // Execute file
            }
            else if (pid == -1) { // If error
                error();
                return 1;
            }
            else{
                waitpid(pid,&status,0); // Wait for child process
                return 1;
            }
        }
    }
    
    return 0;
}

// Read Line function used for reading input 
int read_line(char** args, FILE* fp) {
    // Variable initialization
    char* line = malloc(100 * sizeof(char)), * forge = malloc(100 * sizeof(char)), * redir = NULL;
    size_t size = 0;
    
    if(getline(&line, &size, fp) == -1) { // If error   
        free(line);  
        return 1;
    }
    strcpy(forge, line); // Copy line to forge
    int argc = split_line(line, args); // Split line
    if(argc == -1) { // If error
        free(line);
        return -1;
    }
    if(strcmp(args[0], "cd") == 0) { // If cd
        cd(args);
        free(line);
        return 0;
    }
    if (strcmp(args[0], "exit") == 0) { // If exit
        if (argc != 1) { // If argc is not 1
            free(line);
            error();
        }
        exit(0);
    }
    if (strcmp(args[0], "path") == 0 ) { // If path
        path(args);
        return 0;
    }
    if(strcmp(args[0], "if") == 0) { // If if
        // Variable initialization
        int i = 0, j = 0, k = 0, l = 0, m = 0, n = 0, o = 0, p = 0, q = 0, r = 0, s = 0, t = 0, u = 0, v = 0;
        char* if_then[8], * then_fi[50], * compare [10], * final[3];

        if(strcmp(args[argc - 1], "fi") != 0) { // If fi
            error();
            exit(0);
        }
        while(args[i] != NULL && i < argc - 1) { // Loop through tokens
            if(strcmp(args[i], "fi_") == 0) { // If fi_
                error();
                return 0;
            }
            i++;
        }
        while(args[j] != NULL && j < argc - 1) { // Loop through tokens
            if(strcmp(args[j], "then_") == 0) { // If then_
                error();
                return 0;
            }
            j++;
        }
        while(args[k] != NULL && k < argc - 1) { // Loop through tokens
            if(strcmp(args[k], "&|") == 0) { // If &|
                error();
                return 0;
            }
            k++;
        }
        for(l = 0; l < 8; l++) { // Loop through tokens
            if_then[l] = '\0';
        }
        for(m = 1; m < argc - 1; m++) { // Loop through tokens
            if(strcmp(args[m], "then") == 0) { // If then
                break;
            }
            if_then[m - 1] = args[m]; // Add elements to if_then
        }
        if(stringCompare(if_then) != 0) { // If not equal
            exit(0);
        } else { // Else
            for(n = 0; n < 50; n++) { // Loop through tokens
                then_fi[n] = '\0';
            }
            int after_then = m + 1; // Set after_then to m + 1
            if(strcmp(args[m], "then") != 0) { // If then
                error();
                return -1;
            }
            if(strcmp(args[after_then], "if") == 0) { // If if
                for(o = after_then; o < argc - 1; o++) { // Loop through tokens
                    then_fi[o - after_then] = args[o]; // Add elements to then_fi
                    p++;
                }
                
                for(q = 0; q < 10; q++) { // Loop through tokens
                    compare[q] = '\0';
                }
                // Add elements to compare
                compare[0] = args[after_then + 1];
                compare[1] = args[after_then + 2];
                compare[2] = args[after_then + 3];
                compare[3] = args[after_then + 4];
                for(r = 0; r < 3; r++) { // Loop through tokens
                    final[r] = '\0';
                }
                final[0] = args[after_then + 6]; // Add elements to final
                if(stringCompare(compare) == 0) { // If equal
                    executeFile(final, NULL);
                    return 0;
                } else { // Else
                    return 1;
                }
            } else if(strcmp(args[after_then], "ls") == 0) { // If ls
                for(t = after_then; t < argc - 1; t++) { // Loop through tokens
                    then_fi[t - after_then] = args[t]; // Add elements to then_fi
                    u++;
                }
                detection(then_fi); // Detect file
                return 0;
            } else if(strcmp(args[after_then], "cd") == 0) { // If cd
                for(s = after_then; s < argc - 1; s++) { // Loop through tokens
                    then_fi[s - after_then] = args[s]; // Add elements to then_fi
                }
                cd(then_fi);
                return 0;
            } else { // Else
                for(v = m + 1; v < argc - 1; v++) { // Loop through tokens
                    if(strcmp(args[m], "fi") == 0) { // If fi
                        break;
                    }
                    then_fi[v - m -1] = args[v]; // Add elements to then_fi
                }
                executeFile(then_fi, NULL); // Execute file
                return 0;
            }
        } 
    }
    if ((redir = strchr(forge, '>'))) { // If >
        redirect(redir, forge); // Redirect
        free(line);
        return 0;
    }   

    return -1;
}

// Main function
int main(int argc, char** argv) {
    // Variable initializations
    PATH[0] = "/bin";
    FILE* file;
    char** args = malloc(100 * sizeof(char));
    int status;

    while(1) { // Loop forever
        if(argc == 1) { // If argc is 1
            fprintf(stdout, "wish> ");
            status = read_line(args, stdin);
            if(status == 0) { // If status is 0
                continue;
            }
            if(status == 1) { // If status is 1
                return 0;
            }
            if(execute(args, NULL) == 1) { // If execute is 1
                continue;
            }
            free(args); // Free args
        } else if(argc == 2) { // If argc is 2
            file = fopen(argv[1], "r");
            if(file == NULL) { // If file is NULL
                error();
                exit(1);
            }
            while(1) { // Loop forever
                status = read_line(args, file); // Call read_line
                if(status == 0) { // If status is 0
                    continue;
                }
                if(status == 1) { // If status is 1
                    return 0;
                }
                if(execute(args, NULL) == 1) { // If execute is 1
                    continue;
                }
                free(args); // Free args
            }
        } else { // Else
            error();
            exit(1);
        }
    }

    return 0;
}