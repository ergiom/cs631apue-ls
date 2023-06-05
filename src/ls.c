#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>

#include "ls.h"

#define DIR_READ_ERROR 1
#define BAD_ARGS_ERROR 2
#define FILE_READ_ERROR 3

const char *bin_name(const char *);
int statat(const char *, const char *, struct stat *);
void print_permission(const mode_t);
long exp_ten(int);

int main(int argc, char const *argv[])
{
    int return_flag = 0;
    DIR *dir;
    struct dirent *file;
    struct stat stat_s;
    data_width_t data_width = { .hard_links = 0 };
    int number_of_hard_links;
    int unsigned temp;
    size_t string_len;

    if (argc != 2) {
        printf("Usage: %s file/dir\n", bin_name(*argv));
        exit(BAD_ARGS_ERROR);
    }

    if ((stat(*(argv + 1), &stat_s)) == -1) {
        perror("Couldn't open selected dir");
        exit(BAD_ARGS_ERROR);
    }

    if (S_ISDIR(stat_s.st_mode)) {
        if ((dir = opendir(*(argv + 1))) == NULL) {
            perror("Couldn't open selected directory");
            return DIR_READ_ERROR;
        }

        while ((file = readdir(dir)) != NULL) {
            temp = 0;
            if ((statat(*(argv + 1), file->d_name, &stat_s)) != 0) {
                break;
            }

            number_of_hard_links = stat_s.st_nlink;
            while (number_of_hard_links / exp_ten(temp++));

            if (temp > data_width.hard_links) {
                data_width.hard_links = temp;
            }

            string_len = strlen(getpwuid(stat_s.st_uid)->pw_name);
            
            if (string_len > data_width.username_len) {
                data_width.username_len = string_len;
            }

            string_len = strlen(getgrgid(stat_s.st_gid)->gr_name);
            
            if (string_len > data_width.groupname_len) {
                data_width.groupname_len = string_len;
            }
        }

        rewinddir(dir);
        printf("max len: %li\n", data_width.hard_links);
        errno = 0;
        while ((file = readdir(dir)) != NULL) {
            if ((statat(*(argv + 1), file->d_name, &stat_s)) != 0) {
                break;
            }
            print_permission(stat_s.st_mode);
            putchar(' ');

            printf("%*lu %*s %-*s %*lu %lu %lu %s\n",
                (int)data_width.hard_links, /* number of links width */
                stat_s.st_nlink, /* number of links */
                (int)data_width.username_len,
                getpwuid(stat_s.st_uid)->pw_name,   /* owner id */
                (int)data_width.groupname_len,
                getgrgid(stat_s.st_gid)->gr_name,   /* group id */
                5, stat_s.st_size,  /* byte size */
                stat_s.st_mtime, /* modification time */
                file->d_ino,     /* inode number */
                file->d_name     /* file name */
            );

            errno = 0;
        }

        if (errno != 0) {
            perror("Error reading dir contents");
            return_flag = DIR_READ_ERROR;
        }

        closedir(dir);
    } else if (S_ISREG(stat_s.st_mode)) {
        if ((stat(*(argv + 1), &stat_s)) != 0) {
            perror("Error reading file");
            return_flag = FILE_READ_ERROR;
        }

        print_permission(stat_s.st_mode);
        putchar(' ');

        printf("%lu %i %i %*lu %lu\n",
            stat_s.st_nlink, /* number of links */
            stat_s.st_uid,   /* owner id */
            stat_s.st_gid,   /* group id */
            5, stat_s.st_size,  /* byte size */
            stat_s.st_mtime /* modification time */
        );
    }

    return return_flag;
}

const char *bin_name(const char *path) {
    int slash_pos = strlen(path);
    while (*(path + slash_pos) != '/' && slash_pos > 0) {
        slash_pos--;
    }
    return path + slash_pos + 1;
}

int statat(const char *dir, const char *location, struct stat *stat_s) {
    char *path;
    int dir_len = strlen(dir);
    int location_len = strlen(location);
    int return_flag = 0;

    if ((path = malloc(dir_len + 1 + location_len + 1)) == NULL) {
        printf("Could not allocate memory\n");
        errno = ENOMEM;
        return -1;
    }

    char *pos = path;
    memcpy(pos, dir, dir_len);
    pos += dir_len;
    *(pos) = '/';
    pos++;
    memcpy(pos, location, location_len);
    pos += location_len;
    *(pos) = '\0';

    if ((lstat(path, stat_s)) == -1) {
        perror("Couldn't open selected dir");
        return_flag = -1;
    }
    free(path);
    return return_flag;
}

void print_permission(const mode_t mode) {
    if (S_ISLNK(mode)) {
        putchar('l');
    }
    else if (S_ISDIR(mode)) {
        putchar('d');
    }
    else if (S_ISFIFO(mode)) {
        putchar('p');
    }
    else if (S_ISBLK(mode)) {
        putchar('b');
    }
    else if (S_ISCHR(mode)) {
        putchar('c');
    }
    else if (__S_IFSOCK & mode) {
        putchar('s');
    }
    else {
        putchar('-');
    }

    putchar(S_IRUSR & mode ? 'r' : '-');
    putchar(S_IWUSR & mode ? 'w' : '-');
    if (S_ISUID & mode) {
        putchar((S_IXUSR & mode) ? 's' : 'S');
    }
    else if (S_IXUSR & mode) {
        putchar('x');
    }
    else {
        putchar('-');
    }

    putchar(S_IRGRP & mode ? 'r' : '-');
    putchar(S_IWGRP & mode ? 'w' : '-');
    if (S_ISGID & mode) {
        putchar((S_IXGRP & mode) ? 's' : 'S');
    }
    else if (S_IXGRP & mode) {
        putchar('x');
    }
    else {
        putchar('-');
    }

    putchar(S_IROTH & mode ? 'r' : '-');
    putchar(S_IWOTH & mode ? 'w' : '-');
    if (__S_ISVTX & mode) {
        putchar((S_IXOTH & mode) ? 't' : 'T');
    }
    else if (S_IXOTH & mode) {
        putchar('x');
    }
    else {
        putchar('-');
    }
}

long exp_ten(int n) {
    long result = 1;
    while (n-- > 0) {
        result *= 10;
    }
    return result;
}
