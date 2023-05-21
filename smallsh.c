#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h> //for FORK
#include <sys/wait.h> // for children processes checks per the module
#include <signal.h> //FOR SIGNALS
#include <sys/types.h> //for children
#include <errno.h> //for errors
#include <fcntl.h> // for file read write
#include <ctype.h> // for intmax_t NO
#include <sys/stat.h>
#include <stdint.h>

char *procPid = NULL;
char *dollarquestion = "0";
char *dollarmark = "";

pid_t spawnPid = -5;
int child_flag = 0;

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);

void handle_SIGINT(int signo){
}

int main (void){
    struct sigaction SIGINT_action = {0};
    struct sigaction SIGTSTP_action = {0};
    SIGINT_action.sa_handler = handle_SIGINT;
    SIGINT_action.sa_flags = 0;
    sigfillset(&SIGINT_action.sa_mask);
    SIGTSTP_action.sa_handler = SIG_IGN;
    

    procPid = malloc(8);
    sprintf(procPid, "%d", getpid());
    char *user_input = NULL;
    size_t user_input_size = 2048;

    int exit_command = 0;

for_loop:
    for(;;){
        char *read_file = NULL;
        char *write_file = NULL;
        char *directory = NULL;

        int index = 0;

        int bg_flag_bool = 0;
        int write_flag = 0;
        int read_flag = 0;

        int childStatus;
        pid_t start_spawnPid;
        int count=0;
        char *word_array[512]; 
        for (int l = 0; l < (sizeof(word_array)/sizeof(word_array[0])); l++){
          
          if (word_array[l] != NULL){
            word_array[l] = NULL;
          }

        }

        //non blocking waitpid before the prompt, in the script
        while((start_spawnPid = waitpid(0, &childStatus, WUNTRACED | WNOHANG)) > 0){
            if(WIFEXITED(childStatus)){
                fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t) start_spawnPid, WEXITSTATUS(childStatus));
            }
            if(WIFSIGNALED(childStatus)){
                fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) start_spawnPid, WTERMSIG(childStatus));
            }
            if(WIFSTOPPED(childStatus)){
                kill(start_spawnPid, SIGCONT);
                fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) start_spawnPid);
            }
        }       
        
        char *psenv = getenv("PS1");
        if (psenv == NULL){
            fprintf(stderr, "");

        }
        if (psenv != NULL){
            fprintf(stderr, "%s", getenv("PS1"));
        }
        sigaction(SIGINT, &SIGINT_action, NULL);
        int linenum = getline(&user_input, &user_input_size, stdin);
        sigaction(SIGINT, &SIGTSTP_action, NULL);
        
        if (linenum == -1){
          clearerr(stdin);
          exit(childStatus);
        } 
        if (feof(stdin) != 0){
          clearerr(stdin);
          exit(childStatus);
        }

        char *ifs_nonsense = getenv("IFS");

        if (ifs_nonsense == NULL){
            ifs_nonsense = " \t\n";
        }

        char *token = strtok(user_input, ifs_nonsense);

        while (token != NULL){

            word_array[count] = strdup(token);

            token = strtok(NULL, ifs_nonsense);

            count++;
        }


        char *home_replacement = getenv("HOME");

        if (home_replacement == NULL){
            home_replacement = "";
        }
      
        //expansion
        for(int i = 0; i < count; i++){

            if (strncmp(word_array[i], "~/", 2) == 0){
                word_array[i] = str_gsub(&word_array[i], "~", home_replacement);
                //goto while_loop;

            }
            word_array[i] = str_gsub(&word_array[i], "$$", procPid);
            word_array[i] = str_gsub(&word_array[i], "$?", dollarquestion);               
            word_array[i] = str_gsub(&word_array[i], "$!", dollarmark);
        }

        //PARSING
        index = (count -1);
        for(int m = 0; m < count; m++){

            if (word_array[0]){
                if (strcmp(word_array[index], "&") == 0) {
                    bg_flag_bool = 1;
                    free(word_array[index]);
                    word_array[index] = NULL;
                    count--;
                    index = index - 1;
                } // end &
            }
            if (strcmp(word_array[m], "#") == 0){
                word_array[m] = NULL;
                count--;
                index = index-1;
            }
        }//&LOOP
        if (word_array[index-1]){
            if (strcmp(word_array[index-1], "<") == 0){

                write_flag = 0;
                read_flag = 1;
                read_file = word_array[index];
                word_array[index-1] = NULL;

                index = index-2;
                count = count-1;
            }
        }
        if (word_array[index-1]){
            if (strcmp(word_array[index-1], ">") == 0){

                write_flag = 1;
                write_file = word_array[index];
                word_array[index-1] = NULL;

                index = index-2;
                count = count-1;

            }
        }
        if (word_array[index-1]){
            if (strcmp(word_array[index-1], "<") == 0){

                read_flag = 1;
                read_file = word_array[index];
                word_array[index-1] = NULL;
                index = index-2;
                count = count-1;

            }
        }

        //EXECUTION
        if(count > 0) {
            if (strcmp(word_array[0], "exit") == 0) {
                //fprintf(stderr, "EXIT ZONE \n");
                exit_command = 1;
                if (count > 1) {
                    //fprintf(stderr, "not null counter > 1 \n");
                    exit_command = atoi(word_array[1]);

                }

                if (count > 2) {
                    //printf("not null counter > 2 \n");
                    fprintf(stderr, "Too much junk, try without this and beyond: %s\n", word_array[2]);

                    exit(1);
                }

                //null the command array for both values
                exit(exit_command);
            } //end exit command

        }

            spawnPid = fork();


            switch (spawnPid){
                case -1:
                    //fail
                    fprintf(stderr, "fork fail\n");
                    exit(spawnPid);
                    break; // end fork == -1

                case 0:
                    sigaction(SIGINT, &SIGINT_action, NULL);
                    printf("CASE 0");
                    errno = 0;
                    if (read_file != NULL || write_file != NULL) {
                        if (read_flag == 1) {
                            // if file < 0 error open file
                            int file = open(read_file, O_RDONLY);
                            if (file < 0){
                                printf("ERROR OPEN FILE\n");
                                exit(1);
                            }
                            // if dup2 < 0 error
                            int result = dup2(file, 0);
                            if (result == -1 ){
                                printf("ERROR READ FILE\n");
                                exit(2);
                            }
                            //readfile row_file
                        }//end read
                        if (write_flag == 1) {
                            int file = open(write_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                            if (file == -1) {
                                printf("ERROR OPEN FILE\n");
                                exit(1);
                            }

                            int result = dup2(file, 1);
                            if (result == -1) {
                                printf("ERROR WRITE FILE\n");
                                exit(2);
                            }
                            //write file row_file

                        }//end write
                    }
                    if (strstr(word_array[0], "/")){
                      execv(word_array[0], word_array);
                    }
                    else{
                    execvp(word_array[0], word_array);
                        fprintf(stderr, "FAILURE EXECVP");
                        exit(EXIT_FAILURE);
                        //break;
                      }
                      break;
                    
                default:
                    //printf("DEFAULT");
                    //child_flag = 1;
                    //spawnPid = fork();
                    //printf("default\n");
                    //printf("303 %jd\n", (intmax_t)spawnPid);
                    //fork result is a child process id adn this is a parent
                    if (bg_flag_bool == 1) {
                        //printf("BG PROC");
                        //waitpid(spawnPid, &childStatus, WUNTRACED | WNOHANG);
                        dollarmark = malloc(8);
                        sprintf(dollarmark, "%d", spawnPid);

                    } else { // bg flag bool ==0
                        spawnPid = waitpid(spawnPid, &childStatus, 0);
                        if (WIFEXITED(childStatus)){
                            dollarquestion = malloc(8);
                            sprintf(dollarquestion, "%d", WEXITSTATUS(childStatus));
                        }
                       if (WIFSIGNALED(childStatus)){
                           dollarquestion = malloc(8);
                           sprintf(dollarquestion, "%d", (128 + WTERMSIG(childStatus)));
                        }
                        if (WIFSTOPPED(childStatus)) {
                            fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) spawnPid);
                            dollarmark = malloc(8);
                            sprintf(dollarmark, "%d", spawnPid);
                        }
                    }// end of else bg flag bool 0
                   
                    break;
              }
            }//currently end of switch //end word array null check
            //printf("We try again\n");

        

    }

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub)
{
    char *str = *haystack;
    size_t haystack_len = strlen(str);
    size_t const needle_len = strlen(needle), sub_len = strlen(sub);

    for (; (str = strstr(str, needle));) {
        ptrdiff_t off = str - *haystack;
        if (sub_len > needle_len) {
            str = realloc( *haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1));
            if (!str) goto exit;
            *haystack = str;
            str = *haystack + off;
        }
        memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
        memcpy(str, sub, sub_len);
        haystack_len = haystack_len + sub_len - needle_len;
        str += sub_len;
      
        if (strcmp(needle,"~/")==0){
            break;
        }
    }
    str = *haystack;
    if (sub_len < needle_len) {
        str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
        if (!str) goto exit;
        *haystack = str;
    }

    exit:
    return str;
}


