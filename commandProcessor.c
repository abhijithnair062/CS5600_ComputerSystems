//
// Created by cs5600 on 10/4/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
/*Function declarations*/
int processCd(int noOfTokens, char* token);
int processPwd(int noOfTokens);
int  processExit(int noOfTokens, char* token);
int processExternalCommand(int noOfTokens, char** tokens);


int parseLine(int noOfTokens, char** tokens){
    int status = 0;
    if(tokens[0] == NULL) {
        return 0;
    }else if(!tokens[noOfTokens-1] || strcmp(tokens[noOfTokens - 1], ">") == 0|| strcmp(tokens[noOfTokens - 1], "<") == 0){
        return 1;
    }else if(strcmp(tokens[0],"cd") == 0) {
        status = processCd(noOfTokens,tokens[1]);
    }else if(strcmp(tokens[0],"pwd") == 0){
        status = processPwd(noOfTokens);
    }else if (strcmp(tokens[0], "exit") == 0) {
        status = processExit(noOfTokens,tokens[1]);
    }else{
        status = processExternalCommand(noOfTokens, tokens);
    }
    return status;
}

int processCd(int noOfTokens, char* token){
    if(noOfTokens > 2){
        fprintf(stderr,"cd: wrong number of arguments\n");
        fflush(stderr);
        return 1;
    }else{
        char *directory;
        if(token == NULL){
            directory = getenv("HOME");
        }else{
            directory = token;
        }
        if(chdir(directory) != 0){
            fprintf(stderr,"cd: %s\n",strerror(errno));
            return 1;
        }
    }
    return 0;
}
int processPwd(int noOfTokens){
    if(noOfTokens > 1){
        fprintf(stderr, "pwd : Invalid number of arguments!\n");
        fflush(stderr);
        return 1;
    }else{
        char buffer[PATH_MAX];
        char *pwd = getcwd(buffer,sizeof(buffer)); //We can assume getcwd always passes
        printf("%s\n",pwd);
    }
    return 0;
}
int processExit(int noOfTokens, char* token){
    if(noOfTokens > 2) {
        fprintf(stderr, "exit: too many arguments\n");
        fflush(stderr);
        return 1;
    }else if(noOfTokens == 2){
        int status = atoi(token);
        if(status < 0){
            status = 0;
        }
        exit(status);
    }else{
        exit(0);
    }
}
int processExternalCommand(int noOfTokens, char** tokens) {
    int exit_status = 0;
    int startIndex[9];
    int endIndex[9];
    int noOfCmds = 0;
    bool cmdFound = false;
    int i = 0 ; //start array index
    int j = 0; //end array index
    int k = 0; //token array index
    bool checkFlag = 0;
    //Filter out invalid prompts
    for(int i= 1; i< noOfTokens; i++){
        //No need to chck the first index
        if(!tokens[i] || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "<") == 0){
            checkFlag = true;
        }
        if(checkFlag){
            if(!tokens[i-1] || strcmp(tokens[i-1], ">") == 0 || strcmp(tokens[i-1], "<") == 0){
                return 1;
            }else{
                checkFlag = false;
            }
        }
    }
    startIndex[i] = 0;

    //Below code finds start and end index for all commands
    while(k < noOfTokens){
        if(tokens[k]){
            if(cmdFound){
                k = k +1;
                continue;
            }else{
                noOfCmds++;
                if(noOfCmds > 9){
                    //Exceeding the limit of 8 pipeline commands
                    return 1;
                }
                if(i != 0){
                    startIndex[i] = k;
                }
                i++;
                cmdFound = true;
            }
        }else{
            cmdFound = false;
            endIndex[j] = k;
            j++;
    /***********************/
        }
        k = k+ 1;
    }
    if(tokens[k-1]){
        //Only if the second last index is valid
        endIndex[j] = k;
    }
        int pipefd[2];
        pid_t cpid[9];
        int prevRdFd = 0;
        for (int i = 0; i < noOfCmds; i++) {
            if (i < noOfCmds - 1) {
                if(pipe(pipefd) < 0){
                    return 1;
                }
            }
            //Moving here to debug
            int buffSize = endIndex[i]-startIndex[i] + 1;
            char *buf[buffSize];
            for(int j = 0, k = startIndex[i]; j <= (endIndex[i]-startIndex[i]); j++, k++){
                buf[j] = tokens[k];
            }
            buf[buffSize-1] = NULL;
            cpid[i] = fork();
            if(cpid[i] == -1){
                perror("Fork failed");
                return 1;
            }
            /*****************************FILE REDIRECTION CODE*****************************************/
            /*
             * Here goes the logic for file redirection
             * Buf contains redirection symbol
             * Update buf
             * Get the file name
             */
            int redirectionIndex = 0;
            int secondEndIndex = buffSize - 1;
            bool redirectionFlag = false;
            char *op;
            for(int m = 0; m < buffSize; m++){
                if(buf[m]){
                    if(redirectionFlag && (strcmp(buf[m], ">") == 0 || strcmp(buf[m], "<") == 0)){
                        secondEndIndex = m;
                        break;
                    }
                    if(strcmp(buf[m], ">") == 0 || strcmp(buf[m], "<") == 0){
                        redirectionIndex = m;
                        redirectionFlag = true;
                        op = buf[m];
                    }
                }
            }
            char *buf1[redirectionIndex+1]; //+1 for null
            char *filename[secondEndIndex-redirectionIndex]; // Assuming that this already handles null at the end
            if(redirectionFlag){
                int beforeIndex = 0;
                int afterIndex = 0;
                int travIndex = 0;
                while(buf[travIndex] && travIndex != secondEndIndex){
                    if(strcmp(buf[travIndex], ">") == 0 ||strcmp(buf[travIndex], "<") == 0){
                        travIndex++;
                        while(buf[travIndex] != NULL && travIndex != secondEndIndex){
                            filename[afterIndex++] = buf[travIndex];
                            travIndex++;
                        }
                    }else {
                        buf1[beforeIndex++] = buf[travIndex];
                        travIndex++;
                    }
                }
                buf1[beforeIndex] = NULL;
                filename[secondEndIndex-redirectionIndex-1] = NULL;
            }
            /*************************************************/
            if (cpid[i] == 0) {
                //Child
                signal(SIGINT, SIG_DFL);
                if (i < noOfCmds - 1) {
                    //For all commands except last, as there is not need to configure it
                    if(redirectionFlag){
                        if(strcmp(op, ">") == 0){
                            if(close(1) == -1) return 1;
                            if(open(filename[0], O_CREAT | O_WRONLY | O_TRUNC, 0666) == -1) return -1;
                        }
                        if(strcmp(op, "<") == 0){
                            if(close(0) == -1) return 1;
                            if(open(filename[0], O_RDONLY, 0666) == -1) return -1;
                        }
                    }
                    dup2(pipefd[1], 1);
                    close(pipefd[1]);
                    close(pipefd[0]);
                }
                if(i == noOfCmds - 1 && redirectionFlag){
                    if(strcmp(op, ">") == 0){
                        if(close(1) == -1) return 1;
                        if(open(filename[0], O_CREAT | O_WRONLY | O_TRUNC, 0666) == -1) return -1;
                    }
                    if(strcmp(op, "<") == 0){
                        if(close(0) == -1) return 1;
                        if(open(filename[0], O_RDONLY, 0666) == -1) return -1;
                    }
                }
                if (prevRdFd != 0) {
                    dup2(prevRdFd, 0);
                    close(prevRdFd);
                }
                //Redirection with single command
                if(redirectionFlag){
                    execvp(buf1[0], buf1);
                }else{
                    execvp(buf[0], buf);
                }
                fprintf(stderr, "%s: %s\n", buf[0], strerror(errno));
                exit(EXIT_FAILURE);
            } else {
                if (prevRdFd != 0) {
                    close(prevRdFd);
                    prevRdFd = 0;
                }
                if (i < noOfCmds - 1) {
                    close(pipefd[1]);
                    prevRdFd = pipefd[0];
                }
                int childStatus;
                do {
                    assert(cpid[i] > 0);
                    waitpid(cpid[i], &childStatus, WUNTRACED);
                } while (!WIFEXITED(childStatus) && !WIFSIGNALED(childStatus));
                exit_status = WEXITSTATUS(childStatus);
            }
        }
    return exit_status;
}