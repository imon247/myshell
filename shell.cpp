#include <stdio.h>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

constexpr size_t MAX_STRLEN = 256;
constexpr size_t NUM_REDIRECT_SYMBOL = 5;

char **arg_list;
void split(char*, std::vector<std::string>&);
int split(char*, char**);

int lookup_builtin(char*);
void builtin_cd(int, char**);
void builtin_pwd(int, char**);
void (*builtin_func_list[])(int, char**){
    builtin_cd,
    builtin_pwd,
};
const char *builtin_cmd_list[] = {"cd", "pwd", nullptr};
const char *redirect_symbol_list[] = {"<", ">", ">>", "2>", "2>>"};
int stdio_file_desc[3] = {0, 1, 2};
enum IO_MODE {READ, APPEND, CREATE};
int find_token(int, const char**, const char*);
int redirect_in(char**, int);
int redirect_out(char**, int, int);
int redirect_err(char**, int, int);
void remove_from_arg_list(int&, char**, int, ...);
void rearrange_arg_list(int*, char**);
void redirect(int*, char**);
void reset_io_desc();

int main(){
    char *in;
    size_t cmd_len = MAX_STRLEN;
    // std::vector<std::string> tokens;
    int pid, status;
    printf("> ");
    while((getline(&in, &cmd_len, stdin))!=-1){
        in[strlen(in)-1] = '\0';
        if(strlen(in)==0){
            printf("> ");
            continue;
        }
        if(strcmp(in, "exit")==0) break;
        int num = split(in, arg_list);
        if(num==0) {
            printf("> ");
            continue;
        }
        arg_list = new char*[num+1];
        split(in, arg_list);
        arg_list[num] = NULL;
        int i_cmd = lookup_builtin(arg_list[0]);
        if(i_cmd>=0) {
            builtin_func_list[i_cmd](num, arg_list);
            printf("> ");
            continue;
        }
        
        
        redirect(&num, arg_list);
        int pos = find_token(num, arg_list, "|");
        while(pos>=0){
            pid = fork();
        }
        // printf("%d\n", num);
        pid = fork();
        if(pid==0){
            execvp(arg_list[0], arg_list);
            perror("Unable to execute program");
            exit(EXIT_FAILURE);
        }
        
        pid = wait(&status);
        reset_io_desc();
        printf("> ");
        // clear_arg_list(num, arg_list);
        for(int i=0;i<num;i++){
            delete[] arg_list[i];
        }
        delete[] arg_list;
    }
}



void redirect(int* argc, char** arg_list){
    int redirect_pos[NUM_REDIRECT_SYMBOL];
    int i=0;
    for(const char* symbol : redirect_symbol_list){
        redirect_pos[i] = find_token(*argc, (const char**)arg_list, redirect_symbol_list[i]);
        ++i;
    }
    int r = -1;
    if(redirect_pos[0]>=0){
        r = redirect_in(arg_list, redirect_pos[0]);
        stdio_file_desc[0] = r;
        if(r>=0) remove_from_arg_list(*argc, arg_list, 2, redirect_pos[0], redirect_pos[0]+1);
    }
    if(redirect_pos[1]>redirect_pos[2]){
        r = redirect_out(arg_list, redirect_pos[1], O_WRONLY | O_CREAT | O_TRUNC);
        stdio_file_desc[1] = r;
        if(r>=0) remove_from_arg_list(*argc, arg_list, 2, redirect_pos[1], redirect_pos[1]+1);
    }
    else if(redirect_pos[1]<redirect_pos[2]){
        r = redirect_out(arg_list, redirect_pos[2], O_WRONLY | O_APPEND);
        stdio_file_desc[1] = r;
        if(r>=0) remove_from_arg_list(*argc, arg_list, 2, redirect_pos[2], redirect_pos[2]+1);
    }
    if(redirect_pos[3]>redirect_pos[4]){
        r = redirect_err(arg_list, redirect_pos[3], O_WRONLY | O_CREAT | O_TRUNC);
        stdio_file_desc[2] = r;
        if(r>=0) remove_from_arg_list(*argc, arg_list, 2, redirect_pos[3], redirect_pos[3]+1);
    }
    else if(redirect_pos[3]<redirect_pos[4]){
        r = redirect_err(arg_list, redirect_pos[4], O_APPEND);
        stdio_file_desc[2] = r;
        if(r>=0) remove_from_arg_list(*argc, arg_list, 2, redirect_pos[4], redirect_pos[4]+1);
    }
    rearrange_arg_list(argc, arg_list);
}

void split(char * line, std::vector<std::string> &tokens){
    char buffer[MAX_STRLEN];
    int i=0, previous=0, len=0;
    char d = ' ';
    while(line[i]==d){
        i++; previous++;
    }

    while(i<strlen(line)){
        if(line[i]==d){
            strncpy(buffer, line+previous, i-previous);
            buffer[i-previous] = '\0';
            std::string s = buffer;
            tokens.push_back(s);
            previous = i+1;
        }
        ++i;
    }
    strcpy(buffer, line+previous);
    std::string s = buffer;
    tokens.push_back(s);
}


int split(char * line, char ** str_arr){
    size_t token_len;
    size_t i = 0, begin = 0, num_tokens=0;
    // char *token;
    while(i<strlen(line)){
        while(i<strlen(line) && line[i]==' ') i++;
        begin = i;
        while(i<strlen(line) && line[i]!=' ') i++;
        if(str_arr!=NULL && begin!=i){
            str_arr[num_tokens] = new char[i-begin+1];
            strncpy(str_arr[num_tokens], line+begin, i-begin);
            str_arr[num_tokens][i-begin] = '\0';
            begin = i+1;
        }
        if(begin!=i) num_tokens++;
    }
    return num_tokens;

}

int lookup_builtin(char* cmd){
    // char **builtin = builtin_cmd_list;
    int i=0;
    while(builtin_cmd_list[i]!=NULL){
        if(strcmp(builtin_cmd_list[i], cmd)==0) return i;
        ++i;
    }
    return -1;
}

void builtin_cd(int argc, char** argv){
    // change to home dir
    if(argc==1){
        int i = chdir(getenv("HOME"));
        if(i!=0) printf("No such file or directory.\n");
    }
    // change to a specified directory
    else{
        int i = chdir(argv[1]);
        if(i!=0) printf("No such file or directory.\n");
    }
}

void builtin_pwd(int argc, char** argv){
    char s[100];
    printf("%s\n", getcwd(s, 100));
}

int find_token(int argc, const char** argv, const char* symbol){
    for(int i=0;i<argc;i++){
        if(strcmp(argv[i], symbol)==0) return i;
    }
    return -1;
}

void remove_from_arg_list(int& argc, char** argv, int count, ...){
    va_list pos_list;
    int i=0;
    va_start(pos_list, count);
    
    for(i=0;i<count;i++){
        int j = va_arg(pos_list, int);
        delete[] argv[j];
        argv[j] = NULL;
        
    }
    va_end(pos_list);
}
void rearrange_arg_list(int* argc, char** arg_list){
    int next_i = 0, p=0;
    while(p<*argc){
        while(p<*argc && arg_list[p]==NULL) p++;
        if(p>=*argc) break;
        arg_list[next_i] = arg_list[p];
        next_i++;
        p++;
    }
    *argc = next_i;
}
int redirect_in(char** arg_list, int pos){
    // printf("redirect in\n");
    if(arg_list[pos+1]!=NULL){
        int stdin_copy = dup(STDIN_FILENO);
        int file_desc = open(arg_list[pos+1], O_RDONLY);
        if(file_desc>0){
            dup2(file_desc, 0);
            return stdin_copy;
        }
    }
    return -1;
    
}
int redirect_out(char** arg_list, int pos, int io_mode){
    // printf("redirect out\n");

    if(arg_list[pos+1]!=NULL){
        int stdout_copy = dup(STDOUT_FILENO);
        int file_desc = open(arg_list[pos+1], io_mode);
        // printf("file_desc: %d\n", file_desc);
        if(file_desc>0){
            dup2(file_desc, 1);
            return stdout_copy;
        }
    }
    return -1;
}
int redirect_err(char** arg_list, int pos, int io_mode){
    // printf("redirect err\n");
    if(arg_list[pos+1]!=NULL){
        int stderr_copy = dup(STDOUT_FILENO);
        int file_desc = open(arg_list[pos+1], io_mode);
        if(file_desc>0){
            dup2(file_desc, 2);
            return stderr_copy;
        }
    }
    return -1;
}
void reset_io_desc(){
    if(stdio_file_desc[0]!=0){
            dup2(stdio_file_desc[0], STDIN_FILENO);
            close(stdio_file_desc[0]);
            stdio_file_desc[0] = 0;
    }
    if(stdio_file_desc[1]!=1){
        dup2(stdio_file_desc[1], STDOUT_FILENO);
        close(stdio_file_desc[1]);
        stdio_file_desc[1] = 1;
    }
    if(stdio_file_desc[2]!=2){
        dup2(stdio_file_desc[2], STDERR_FILENO);
        close(stdio_file_desc[2]);
        stdio_file_desc[2] = 2;
    }
}

