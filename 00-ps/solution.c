#include <solution.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

/*
SYNOPSIS
       #include <dirent.h>

       struct dirent *readdir(DIR *dirp);


executable: /proc/pid/exe
args: /proc/pid/cmdline
env_variables: /proc/pid/environ 

to check if name is a number: atoi()

need a funciton to parse environment variables
need a function to parse command line arguments
need a function to parse executable path

 // Pointer array + data buffer in one allocation.
    char **result = malloc(
        (count + 1) * sizeof(*result) + n + 1
    );

    if (result == NULL) {
        free(buffer);
        return NULL;
    }

    char *data = (char *)(result + count + 1);
    memcpy(data, buffer, n + 1);
    free(buffer);

    if (count > 0) {
        result[0] = data;

        size_t j = 1;
        for (size_t i = 0; i < n; i++) {
            if (data[i] == '\0' && i + 1 < n) {
                result[j++] = &data[i + 1];
            }
        }
    }

    result[count] = NULL;
    return result;
}
*/
const int FILEPATH_MAX_SIZE = 4096;

const char* get_executable_name(const char* path) 
{
       char *buffer = malloc(FILEPATH_MAX_SIZE);
       ssize_t n = readlink(path, buffer, FILEPATH_MAX_SIZE - 1);
       if (n < 0) 
       {
              report_error(path, errno);
              free(buffer);
              return NULL;
       }
       buffer[n] = '\0';
       const char* data = buffer;
       return data;
}

char** get_splited_data(const char* path) 
{
       FILE* file = fopen(path, "rb");
       if (file == NULL) 
       {
              report_error(path, errno);
              char** empty = malloc(sizeof(char*));
              empty[0] = NULL;
              return empty;
       }
       size_t cap = 4096, n = 0, r;
       char *buffer = malloc(cap + 1);
       while ((r = fread(buffer + n, 1, cap - n, file)) > 0)
       {
              n += r;
              if (n == cap)
              {
                     cap *= 2;
                     buffer = realloc(buffer, cap + 1);
              }
       }
       fclose(file);
       buffer[n] = '\0';
       long file_size = n;

       int amount_of_strings = 1;
       for (int i = 0; i < file_size-1; i++) 
       {
              if (buffer[i] == '\0') 
              {
                     amount_of_strings++;
              }
       }
       if (file_size == 0)
       {
              char** empty = malloc(sizeof(char*));
              empty[0] = NULL;
              return empty;
       }

       char** result = malloc((amount_of_strings + 1)* sizeof(char*) + file_size + 1);
       char *data = (char *)(result + amount_of_strings + 1);
       memcpy(data, buffer, n + 1);
       if (file_size == 0)
       {
              return result;
       }
       int j = 0;
       result[0] = &data[0];
       for (int i = 0; i < file_size-1; i++) 
       {
              if (buffer[i] == '\0') 
              {
                     result[++j] = &data[i + 1];
              }
       }
       result[amount_of_strings] = NULL;
       return result;
}

void ps(void)
{
	char* processes_path = "/proc/";
       DIR *dir = opendir(processes_path);
       if (dir == NULL) 
       {
              report_error(processes_path, errno);
              return;
       }
       struct dirent *entry;
       while ((entry = readdir(dir)) != NULL)
       {
              int pid = atoi(entry->d_name);
              if (pid == 0) 
              {
                     continue;
              }
              char executable_path[FILEPATH_MAX_SIZE];
              char cmdline_path[FILEPATH_MAX_SIZE];
              char environ_path[FILEPATH_MAX_SIZE];
              snprintf(executable_path, sizeof(executable_path), "/proc/%s/exe", entry->d_name);
              snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", entry->d_name);
              snprintf(environ_path, sizeof(environ_path), "/proc/%s/environ", entry->d_name);
              const char* exe_path = get_executable_name(executable_path);
              char** argv = get_splited_data(cmdline_path);
              char** envp = get_splited_data(environ_path);
              report_process(pid, exe_path, argv, envp);
              free((void *)exe_path);
              free(argv);
              free(envp);
       }
       closedir(dir);
}
