#include <solution.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>


const int FILEPATH_MAX_SIZE = 4096;

void lsof(void)
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
		char fd_path[FILEPATH_MAX_SIZE];
		snprintf(fd_path, sizeof(fd_path), "/proc/%s/fd", entry->d_name);
		DIR *fd_dir = opendir(fd_path);
		if (fd_dir == NULL) 
		{
			report_error(fd_path, errno);
			continue;
		}
		struct dirent *fd_entry;
		while ((fd_entry = readdir(fd_dir)) != NULL)
		{
			if (strcmp(fd_entry->d_name, ".") == 0 || strcmp(fd_entry->d_name, "..") == 0) 
			{
				continue;
			}
			char link_path[FILEPATH_MAX_SIZE];
			snprintf(link_path, sizeof(link_path), "%s/%s", fd_path, fd_entry->d_name);
			char target_path[FILEPATH_MAX_SIZE];
			ssize_t len = readlink(link_path, target_path, sizeof(target_path) - 1);
			if (len == -1) 
			{
				report_error(link_path, errno);
				continue;
			}
			target_path[len] = '\0';
			report_file(target_path);
		}
	}
	closedir(dir);
}
