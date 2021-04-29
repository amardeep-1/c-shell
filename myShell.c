/* AMARDEEP SINGH - SHELL - A1 */

/* Includes */
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

/* Prints the beginning part of the propmt */
void printCWD() {
    char cwd[1000];
    getcwd(cwd, sizeof(cwd));
    printf("%s> ", cwd);
}

/* Splits any char array with a delimeters that are sent */
void splitCommand(char **paramss, char *line, char *split) {
    char *token;
    token = strtok(line, split);
    paramss[0] = token;
    int count = 1;
    
    while (token != NULL) {
        token = strtok(NULL, split);
        if (token != NULL) {
            paramss[count] = token;
        }
        count++;
    }
    paramss[count-1] = NULL;    /* Adds null to the end for formatting  */
}

/* Prompt for when a command or file is not found  */
void commandNotFound (char **commands) {
    if (execvp(commands[0], commands) < 0) {
        if (commands[0][0] == '.' || commands[0][0] == '/'){
            printf("-myShell: %s: No such file or directory\n", commands[0]);
        } else {
            printf("-bash: %s: command not found\n", commands[0]);
        }        
    }
}

/* Prints from the history file */
void printHistory(FILE *historyFile, int lines) {
    size_t lengthHistory;
    char *historyLine = malloc(256 * sizeof(char));
    FILE *fptr;
    fptr = fopen(".CIS3110_history","r");
    FILE *fptrCpy;
    fptrCpy = fopen(".CIS3110_history","r");
    int lineCount = 0, i = 0;
 
    if (lines == -1) {
        while (getline(&historyLine, &lengthHistory, fptr) != -1) {    /* simple history command */
            lineCount ++;
            printf(" %d  %s", lineCount, historyLine); 
        }
    } else {
        while (getline(&historyLine, &lengthHistory, fptr) != -1) {
            lineCount ++;
        }
        while (getline(&historyLine, &lengthHistory, fptrCpy) != -1) {  /* history n command  */
            i ++;
            if (i > lineCount-lines && i <= lineCount) {
                printf(" %d  %s", i, historyLine); 
            }
        }
    }
    free(historyLine);
    fclose(fptr);
    fclose(fptrCpy);
}

/* handles single redirects */
void rdFile (char **commands, char *filename, int std) {
    int status;
    FILE *fp;
    if (std) {
        fp = freopen(filename, "w+", stdout);  /* if to handle both type of redirects */
    } else {
        fp = freopen(filename, "r", stdin);
    }
    commandNotFound(commands);
    status = execvp(commands[0], commands);
    exit(status);
    fclose(fp);
}

/* handles multiple redirects */
void rdFileMt (char **commands, char *infilename, char *outfilename, int std) {
    int status;
    FILE * infp;
    FILE * outfp;
    if (std) {
        infp = freopen(infilename, "w+", stdout);     /* if to handle both senarios of redirects */
        outfp = freopen(outfilename, "r", stdin);
    } else {
        outfp = freopen(outfilename, "r", stdin);
        infp = freopen(infilename, "w+", stdout);
    }
    commandNotFound(commands);
    status = execvp(commands[0], commands);
    exit(status);
    fclose(outfp);
    fclose(infp);
}

/* function that handles execution of command */
void executeCommand (char **commands) {
    int toFile = 0, fromFile = 0, count = 0, infile = 0, outfile = 0, paramsDone = 0, status;
    char *inFilename;
    char *outFilename;

    while (commands[count] != NULL) {
        if (strcmp(commands[count], ">") == 0) {             /* types of redirects */
            toFile = 1;
            infile = count;
        } else if (strcmp(commands[count], "<") == 0) {
            fromFile = 1;
            outfile = count;
        } 
        count++;
    }

    if (toFile || fromFile) {
        if (toFile && fromFile) {
            inFilename = commands[infile + 1];
            outFilename = commands[outfile + 1];

            if (infile > outfile) {  
                commands[infile] = NULL;
                rdFileMt(commands, inFilename, outFilename, 1);
            } else {
                commands[outfile] = NULL;
                rdFileMt(commands, inFilename, outFilename, 0);
            }
        } else if (toFile) {
            inFilename = commands[infile + 1];
            commands[infile] = NULL;
            rdFile (commands, inFilename, 1);
        } else if (fromFile) {
            outFilename = commands[outfile + 1];
            commands[outfile] = NULL;
            rdFile (commands, outFilename, 0);
        }

    } else {                                  /* regular commands */
        commandNotFound(commands);
        commands[count] = NULL;
        status = execvp(commands[0], commands);
        exit(status);
    }           
}

/* function that handles execution of piped command */
int pipeFunc(char *line) {
    char *paramss[50];
    splitCommand(paramss, line, "|");
    char *firstCommand[25];
    splitCommand(firstCommand, paramss[0], " \0");
    char *secondCommand[25];
    splitCommand(secondCommand, paramss[1], " \0");    /* splitting into 2 commands */
 
    int fd[2];
    if (pipe(fd) < 0) {     /* error checking for pipe */
        perror("pipe");
    }
    pid_t childpid = fork();
    int status;

    if (childpid >= 0) {
        if (childpid == 0) {         /* executes first commands and coverts to input of next one */
            dup2(fd[1], 1);
            close(fd[0]);
            close(fd[1]);
            executeCommand (firstCommand);
        }
    
        pid_t grandchildpid = fork();
        if (grandchildpid >= 0) {
            if (grandchildpid == 0) {
                dup2(fd[0], 0);
                close(fd[0]);            /* second command executes with input */
                close(fd[1]);
                executeCommand (secondCommand);
            }
        } else {
            perror("fork");
            exit(-1);
        }
        close(fd[0]);              /* close inputs and start waits */
        close(fd[1]);
        waitpid(childpid,NULL,0);
        waitpid(grandchildpid,NULL,0);
    } else {
        perror("fork");
        exit(-1);
    }
}

/*this function sets up the path variable*/
void setEnvVar (char *pathString, char *type, char *varType, char* split) {  
    char *pathss[1000];
    char pathLine[4000];
    strcpy(pathLine,"");

    splitCommand(pathss,pathString,split);
    if (pathss != NULL) {
        int i = 0;
        while (pathss[i] != NULL) {
            if (i != 0 && strcmp(split, ":") == 0) {
                strcat(pathLine, ":");
            }
            if (strcmp(split, "/") == 0 && strcmp(pathss[i], varType) != 0) {
                strcat(pathLine, "/");
            }
            if (strcmp(pathss[i], "$HOME") == 0) {
                strcat(pathLine, getenv("HOME"));
            } else if (strcmp(pathss[i], "$PATH") == 0) {
                strcat(pathLine, getenv("PATH"));
            } else if (strcmp(pathss[i], "$HISTFILE") == 0) {
                strcat(pathLine, getenv("HISTFILE"));
            } else {
                strcat(pathLine, pathss[i]);
            }
            i++;
        }
        setenv(type, pathLine, 1);
    } else {
        printf("-bash: Could not export %s", varType);
    }
}


int main(int argc, char *argv[]) {
 
    pid_t childpid;
    int status = 0, bgs = 0, ret = 0, exitShell = 0;
    char *commands[100];
    size_t length;
    char **backCommands = malloc(100 * sizeof(char*));
    int *backPids = malloc(100 * sizeof(int*));

    char homdir[1000], curdir[1000], startdir[1000], histdir[1000], pathdir[1000];

    getcwd(homdir, sizeof(homdir));
    strcpy(startdir, homdir);
    strcpy(histdir, homdir);

    FILE *profileFile;
    if (profileFile = fopen(".CIS3110_profile", "r")) {
        char *profileLine = malloc(1000 * sizeof(char*));                 /*reads profile*/
        int count = 0;
        size_t profileLinelength;
        while (getline(&profileLine, &profileLinelength, profileFile) != -1) {
            strtok(profileLine, "\n");
            char *commandss[100];
            char *pathss[100];
            splitCommand(commandss,profileLine," ");
            if (commandss[1] != NULL) {
                splitCommand(pathss,commandss[1],"=");
                if (pathss[1] != NULL) {
                    if (strcmp(pathss[0], "HOME") == 0) {
                        setEnvVar(pathss[1], "HOME", "$HOME", "/");
                        strcpy(homdir, getenv("HOME"));

                    } else if (strcmp(pathss[0], "PATH") == 0) {
                        setEnvVar(pathss[1], "PATH", "$PATH", ":");                 /*I am unsure about this one and I feel like it can mess everything up but ta said it works*/
                        strcpy (pathdir, getenv("PATH"));

                    } else if (strcmp(pathss[0], "HISTFILE") == 0) {
                        setEnvVar(pathss[1], "HISTFILE", "$HISTFILE", "/");
                        strcpy(histdir, getenv("HISTFILE"));
                    }       
                }
            }  
        }
        free(profileLine);
    } else {
       profileFile = fopen(".CIS3110_profile", "w+");              
    }
    fclose(profileFile);

    chdir(histdir);  
    setenv("HISTFILE", histdir, 1);  
    FILE *historyFile;
    historyFile = fopen(".CIS3110_history","a+");             /*set up history file*/
    chdir(startdir); 

    char *line = malloc(256 * sizeof(char));
    char *linecpy = malloc(256 * sizeof(char));    
 
    while (!exitShell) {
        
        printCWD();
        ret = getline(&line, &length, stdin);
        strtok(line, "\n");
        strcpy(linecpy, line);

        while (waitpid(-1, NULL, WNOHANG) > 0){
            strtok(backCommands[bgs-1], "&");
            printf("[%d]+ DONE                 %s\n",bgs,backCommands[bgs-1]);      /*checks for backgrounded process here*/
            bgs--;
        }

        if (strcmp (line, "\n") == 0) {
            strcpy(line, " ");         
        }
        splitCommand(commands,line," ");
 
        if (commands[0] != NULL) {
            fprintf(historyFile,"%s\n",linecpy);
 
            int isPipe = 0, inBack = 0, count = 0, isEcho = 0, isExp = 0;
 
            while (commands[count] != NULL) {
                if (strcmp(commands[count], "exit") == 0 && count == 0) {                  /*finds special cases*/
                    exitShell = 1;
                    return 0;
                } else if ((strcmp(linecpy, "echo $HOME") == 0 ||  strcmp(linecpy, "echo $PATH") == 0 || strcmp(linecpy, "echo $HISTFILE") == 0)&& count == 0) {
                    isEcho = 1;
                } else if (strcmp(commands[count], "export") == 0 && count == 0) {
                    isExp = 1;
                } else if (strcmp(commands[count], "&") == 0) {
                    inBack = 1;
                } else if (strcmp(commands[count], "|") == 0) {
                    isPipe = 1;
                }  
                count++;          
            } 
 
            if (inBack) {                            /*fix formatting for command with background*/
                commands[count - 1] = NULL;
                count = count - 1;
            }

            if (isExp) {
                char *pathss[100];
                if (commands[1] != NULL) {
                    splitCommand(pathss,commands[1],"=");
                    if (pathss[1] != NULL) {
                        if (strcmp(pathss[0], "HOME") == 0) {
                            setEnvVar(pathss[1], "HOME", "$HOME", "/");
                            strcpy(homdir, getenv("HOME"));

                        } else if (strcmp(pathss[0], "PATH") == 0) {
                            setEnvVar(pathss[1], "PATH", "$PATH",":");                 /*I am unsure about this one and I feel like it can mess everything up but ta said it works*/
                            strcpy (pathdir, getenv("PATH"));

                        } else if (strcmp(pathss[0], "HISTFILE") == 0) {
                            getcwd(curdir, sizeof(curdir));
                            chdir(histdir);
                            fclose(historyFile);
                            setEnvVar(pathss[1], "HISTFILE", "$HISTFILE", "/");
                            strcpy(histdir, getenv("HISTFILE"));
                            chdir(histdir);
                            historyFile = fopen(".CIS3110_history","a+");
                            chdir(curdir);
                        }       
                    }
                }  
            }  else if (isEcho) {
                if (strcmp(commands[1],"$HOME") == 0) {                    /*echo command checks for paths*/
                    printf("%s\n", homdir);
                } else if (strcmp(commands[1],"$PATH") == 0) {
                    printf("%s\n", getenv("PATH"));
                } else if (strcmp(commands[1],"$HISTFILE") == 0) {
                    printf("%s\n", histdir);
                }
            }  else if (isPipe) {
                pipeFunc(linecpy);
 
            } else if (strcmp(commands[0],"history") == 0) {             /*history command closes history file to work on*/
                getcwd(curdir, sizeof(curdir));
                chdir(histdir);
                fclose(historyFile);            /*ensures it is in correct dir*/
            
                if (commands[1] == NULL) {
                    printHistory(historyFile, -1);
                } else if (commands[2] != NULL) {
                    printf("-bash: history: too many arguments\n");
                } else {
                    if (strcmp (commands[1], "-c") != 0) {
                        printHistory(historyFile, atoi(commands[1]));
                    } else {
                        FILE *deleteH;
                        deleteH = fopen(".CIS3110_history","w");
                        fclose(deleteH);
                    }
                } 
                historyFile = fopen(".CIS3110_history","a+");
                chdir(curdir);
 
            } else if (strcmp(commands[0],"cd") == 0) {                    /*cd command*/
 
                if (commands[1] == NULL || strcmp (commands[1], "~") == 0) {             /*makes it can go to home*/
                    chdir(homdir);
                }else if (commands[2] != NULL) {
                    printf("-bash: cd: too many arguments\n");
                } else if (chdir(commands[1]) != 0) {
                    printf("-bash: cd: %s: No such file or directory\n", commands[1]);
                } 
            } else {
                childpid = fork();
                if (childpid >= 0) {
                    if (childpid == 0) {
                        
                        executeCommand (commands);

                    } else {
                        if (inBack){
                            backCommands[bgs] = malloc((100) * sizeof(char));    /*backgrounding handled here*/
                            strcpy(backCommands[bgs], linecpy);
                            backPids[bgs] = childpid;
                            bgs++;
                            printf("[%d] %d\n",bgs, childpid);
                        } else {
                            childpid = waitpid(childpid, &status, 0);
                        }
                    }

                } else {
                    perror("fork");
                    exit(-1);
                }
            }
        }
    }

    free(linecpy);                /*freeing malloced line*/
    free(line);
    fclose(historyFile);
    fclose(profileFile);
    return 0;
}