#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#define MAXLINE 80
#define MAXARGC 80

void executePrompt(char** argv, int Notamper, int in, int out, char* in_file, char* out_file);
int builtInPrompt(char** argv);
void signalChild(int s);

struct Jobs
{
    int jid;
    char max[80];
    char stats[20];
    int pid;
} maxJobs[5];

int forgpid = 0;
int sum = 0;

int Fgpid(){
    int j;
    for(j = 0; j < sum; ++j)
    {
        if(strcmp(maxJobs[j].stats, "Foreground.") == 0)
        {
            return maxJobs[j].pid;
        }
    }
    return 0;
}
void signalint(int s){
    kill(-forgpid, SIGINT);
    return;
}

void signaltstp(int s)
{
    kill(-forgpid, SIGTSTP);
    return;
}

void signalChild(int s)
{
    int stats;
    pid_t pid;
    int j;
    int k;
    int signalnum = -1;

    while((pid = waitpid(signalnum, &stats, WNOHANG|WUNTRACED)) > 0) {
        if(WIFSIGNALED(stats))
        {
            for(j = 0; j < sum; ++j)
            {
                if(maxJobs[j].pid == pid)
                {
                    for(k = j; k < sum - 1; ++k)
                    {
                        maxJobs[k].pid = maxJobs[k+1].pid;
                        strcpy(maxJobs[k].stats, maxJobs[k+1].stats);
                        strcpy(maxJobs[k].max, maxJobs[k+1].max);
                    }
                    sum--;
                }
            }
        }
        else if(WIFEXITED(stats))
        {
            for(j = 0; j < sum; ++j)
            {
                if(maxJobs[j].pid == pid)
                {
                    int k;
                    for(k = j; k < sum - 1; ++k)
                    {
                        maxJobs[k].pid = maxJobs[k+1].pid;
                        strcpy(maxJobs[k].stats, maxJobs[k+1].stats);
                        strcpy(maxJobs[k].max, maxJobs[k+1].max);
                    }
                    sum--;
                }
            }
        }
        else if(WIFSTOPPED(stats)) {
            for(j = 0; j < sum; ++j)
            {
                if(maxJobs[j].pid == pid) strcpy(maxJobs[j].stats, "Stopped.");
            }
        }
    }
    return;
}


int builtInPrompt(char** argv) {
    int kPid = 0;
    int j;
    int k;
    int ij;
    if(strcmp(*argv, "quit") == 0) exit(0);
    else if(strcmp(*argv, "kill") == 0)
    {
        if(argv[1][0] == '%')
        {
            ij = argv[1][1]-'0';
            kPid = maxJobs[ij-1].pid;
        }
        else
            kPid = atoi(argv[1]);

        kill(kPid, SIGCONT);

        if(kill(kPid, SIGINT)==0){
            for(j = 0; j < sum; ++j)
            {
                if(kPid == maxJobs[j].pid)
                {
                    for(k = j; k < sum - 1; ++k)
                    {
                        maxJobs[k].pid = maxJobs[k+1].pid;
                        strcpy(maxJobs[k].stats, maxJobs[k+1].stats);
                        strcpy(maxJobs[k].max, maxJobs[k+1].max);
                    }
                    sum--;
                }
            }
        }
        return 1;
    }
    else if(strcmp(*argv, "jobs") == 0)
    {
        for(j = 0; j < sum; ++j) printf("[%d] (%d) %s %s\n", maxJobs[j].jid, maxJobs[j].pid, maxJobs[j].stats, maxJobs[j].max);
        return 1;
    }
    else if(strcmp(*argv, "fg")==0)
    {
        int foregroundpid = 0;
        if(argv[1][0] == '%')
        {
            ij = argv[1][1]-'0';
            foregroundpid = maxJobs[ij-1].pid;
        }
        else
        {
            foregroundpid = atoi(argv[1]);
            for(j = 0; j < sum; ++j)
            {
                if(foregroundpid == maxJobs[j].pid) ij = maxJobs[j].jid;
            }
        }
        if((strcmp(maxJobs[ij-1].stats, "Stopped.")==0) || (strcmp(maxJobs[ij-1].stats, "Running.")==0))
        {
            if(kill(foregroundpid, SIGCONT)==0)
            {
                for(j = 0; j < sum; ++j)
                {
                    if(foregroundpid == maxJobs[j].pid) strcpy(maxJobs[j].stats, "Foreground.");
                }
                while(foregroundpid == Fgpid()) sleep(1);
            }   
        }
        return 1;
    }
    else if(strcmp(*argv, "bg")==0)
    {
        int backgroundpid = 0;
        if(argv[1][0] == '%')
        {
            ij = argv[1][1]-'0';
            backgroundpid = maxJobs[ij-1].pid;
        }
        else
        {
            backgroundpid = atoi(argv[1]);
            for(j = 0; j < sum; ++j)
            {
                if(backgroundpid == maxJobs[j].pid) ij = maxJobs[j].pid;
            }
        }

        if(strcmp(maxJobs[ij-1].stats, "Stopped.")==0)
        {
            if(kill(backgroundpid, SIGCONT)<0) perror("Killed SIGCONT");
            strcpy(maxJobs[ij-1].stats, "Running.");
        }
        return 1;
    }
    return 0;
}

void executePrompt(char** argv, int Notamper, int in, int out, char* in_file, char* out_file) {
    if(builtInPrompt(argv)) return;
    int inFileID;
    int outFileID;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;

    pid_t pid = fork();
    if(pid == 0)
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        if(in) {
            int redir = open(in_file, O_RDONLY, mode);
            if(in < 0) {
                printf("File not found.");
                exit(0);
            }
            dup2(redir, STDIN_FILENO);
            close(redir);
            in = 0;

        }
        if(out){
            int outredir = open(out_file, O_CREAT|O_WRONLY|O_TRUNC, mode);
            if(outredir < 0) {
                printf("File not found.");
                exit(0);
            }
            dup2(outredir, STDOUT_FILENO);
            close(outredir);
            out = 0;
        }
        execv(*argv, argv);
        execvp(*argv, argv);
        exit(0);
    }
    else
    {
        if(sum == 5)
        {
            printf("More jobs than allowed.\n");
            return;
        }
        maxJobs[sum].jid = sum + 1;
        maxJobs[sum].pid = pid;
        strcpy(maxJobs[sum].stats, "Running.");
        strcpy(maxJobs[sum].max, *argv);
        sum++;

        if(Notamper)
        {
            forgpid = pid;
            strcpy(maxJobs[sum-1].stats, "Foreground.");
            while(pid == Fgpid()) sleep(1);
        }
        else {
            strcat(maxJobs[sum-1].max, "&");
            setpgid(pid, 0);
        }
        //wait(NULL);
    }
}

int ampersand(char **argv){
    int j;
    for(j = 1; argv[j] != NULL; j++);

    if(argv[j-1][0] == '&')
    {

        //free(argv[j-1]);
        argv[j-1] = NULL;
        return 1;
    }
    else
        return 0;
}

int in_redir(char **argv, char **in_file)
{
    int j;
    int k;

    for (j = 0; argv[j] != NULL; j++)
    {
        if(argv[j][0] == '<')
        {
            //free(argv[j]);
            if(argv[j+1] != NULL)
                *in_file = argv[j+1];
            else
                return -1;
            
            for(k = j; argv[k-1] != NULL; k++)
                argv[k] = argv[k+2];
            
            return 1;
        }
    }
    return 0;
}

int out_redir(char **argv, char **out_file)
{
    int j;
    int k;

    for (j = 0; argv[j] != NULL; j++)
    {
        if(argv[j][0] == '>')
        {
            //free(argv[j]);
            if(argv[j+1] != NULL)
                *out_file = argv[j+1];
            else
                return -1;
            
            for(k = j; argv[k-1] != NULL; k++) {
                argv[k] = argv[k+2];
            }
            
            return 1;
        }
    }
    return 0;
}

int main(void){
    char buffer[MAXLINE];
    int in;
    int out;
    char *in_file;
    char *out_file;
    char* argv[MAXARGC];
    int argc = 0;
    int Notamper;

    signal(SIGINT, signalint);
    signal(SIGTSTP, signaltstp);
    signal(SIGCHLD, signalChild);


    while(1){
        
        printf("prompt> ");
        //check for end of file
        if(fgets(buffer, MAXLINE, stdin) == NULL) exit(0);
        size_t len = strlen(buffer);
        if(buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
        
        if(strcmp(buffer, "exit") == 0) break;
        if(buffer[0] == NULL) continue;

        char* parse;
        parse = strtok(buffer, " ");
        int l = 0;
        while(parse != NULL)
        {
            argv[l] = parse;
            parse = strtok(NULL, " ");
            l++;
        }
        argv[l] = NULL;
        argc = l;
        Notamper = (ampersand(argv) == 0);
        in = in_redir(argv, &in_file);
        if(in == -1)
            printf("Error.");
        
        out = out_redir(argv, &out_file);
        if(out == -1)
            printf("Error.");
        executePrompt(argv, Notamper, in, out, in_file, out_file);
    }
    return EXIT_SUCCESS;
}