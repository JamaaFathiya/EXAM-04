#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define fatal "error: fatal\n" // if any sys call except (execve and chdir) failed return this error and aexit the program.
#define cd_arg "error: cd: bad arguments\n" // for cd arguments

int erno(char *s){
    while (*s)
        write(2, s++, 1);
    return 1;
}

int exec(char **c, int i, int tmp, char **env){
    c[i] = 0;
    if (dup2(tmp, STDIN_FILENO) == -1 || close(tmp) == -1)// dup the read-end 
        return erno(fatal);
    execve(c[0], c, env);
    return erno("error: cannot execute ") && erno(c[0]) && erno("\n");
}

int main(int ac, char **av, char **env){
    (void) ac;
    int i = 0;
    int fd[2]; // for pipe
    int tmp = dup(STDIN_FILENO); // save the std input
    if (tmp == -1)
        return erno(fatal);

    while(av[i] && av[i + 1]){
        av = av + i + 1;
        i = 0;

        while(av[i] && strcmp(av[i], ";") && strcmp(av[i], "|"))// count commad args
            i++;

        if (strcmp(av[0], "cd") == 0){
            if (i != 2) // if the number of arguments ( cd directory)  is not 2 return an error
                 erno(cd_arg);
            else if (chdir(av[1]) != 0) //if chdir failed print an error.
            {
                erno("error: cd: cannot change directory to ");
                erno(av[1]);
                erno("\n");
            }
        }
        else if (i != 0 && (av[i] == NULL || strcmp(av[i], ";") == 0)){ // if the ";" it's not the first argument or it's the end of the command  execute it
            int pid = fork();
            if (pid < 0)
                return erno(fatal);
            if (pid == 0){
                if (exec(av, i, tmp, env)) //check if execve failed ,if so terminate the child process
                    return 1;
            }
            else{
                if (close(tmp) == -1) //close tmp to reset it to std in again
                    return erno(fatal);
                while (waitpid(-1, NULL, WUNTRACED) != -1)
                    ;
                tmp = dup(STDIN_FILENO); 
                if (tmp == -1)
                    return erno(fatal);
            }
        }
            else if (i!=0 && strcmp(av[i] , "|") == 0){ // if pipe...
              if (  pipe(fd) == -1)
                  return erno(fatal);
              int pid = fork();
              if (pid < 0)
                  return erno(fatal);
              if ( pid  == 0)
              {
                if (dup2(fd[1], STDOUT_FILENO) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)// dup the write-end of  pipe , close both ends to not have pipe leaks.
                    return erno(fatal);
                if (exec(av, i, tmp, env))
                    return 1;
              }
              else{
                  if (close(fd[1]) == -1 || close(tmp) == -1) // close write-end  and  tmp.
                      return erno(fatal);
                  tmp = fd[0]; //save the read-end for the command after pipe.
            }
        }
    }
    close(tmp);// tmp is still open, close it.
    return 0;
}
