#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>

#include "ls.h"

#define DIR_READ_ERROR 1
#define BAD_ARGS_ERROR 2
#define FILE_READ_ERROR 3

#define TIME_BUFF_SIZE 13
#define PERMISSION_BUFF_SIZE 11

#define OPTS "AacdFfhiklnqRrSstuw"

const char *bin_name(const char *);
int statat(const char *, const char *, struct stat *);
void format_permission(const mode_t, char *);
long exp_ten(int);
void format_time(time_t, char *);
int read_data_width(data_width_t *, const char *);
int print_info(const char *);

enum {
    ALL,
    NO_REL,
    NO_DOTS
    } filter = NO_DOTS;         /* Aa */
enum {
    STATUS,
    MODIFICATION,
    ACCESS
    } time_disp = MODIFICATION; /* cu */
enum {
    REDIRECT,
    NO_REDIRECT
    } redirect = REDIRECT;      /* d */
enum {
    VERBOSE,
    STANDARD
    } verbose_types = STANDARD; /* F */
enum {
    ALPHABETICAL,
    SIZE,
    TIME_MOD,
    NONE
    } sorted = ALPHABETICAL;    /* fSt */
enum {
    VARIABLE,
    KBYTES,
    BYTES
    } size = BYTES;             /* hk */
enum {
    SHOW,
    HIDE
    } inode = HIDE;             /* i */
enum {
    LONG,
    NUMERIC,
    SHORT
    } verbosity = SHORT;        /* ln */
enum {
    BLOCKS,
    NO_BLOCKS
    } blocks = NO_BLOCKS;       /* s */
enum {
    RAW,
    PRINT
    } non_printable = PRINT;    /* qw */
enum {
    RECURSIVE,
    FLAT
    } recursive = FLAT;         /* R */
enum {
    REVERSE,
    NORMAL
    } reverse = NORMAL;         /* r */

int main(int argc, char *argv[])
{
    const char *item_path;
    int err;
    char opt;

    //default set for system state:
        // if user == root -> filter=NO_REL
        // else filter=NO_DOTS

        // if is terminal -> non_printable=HIDE
        // else non_printable=SHOW
    //end

    while ((opt = getopt(argc, argv, OPTS)) != -1) {
        switch (opt) {
            case 'A':
                filter = NO_REL; break;
            case 'a':
                filter = ALL; break;
            case 'c':
                time_disp = STATUS; break;
            case 'd':
                redirect = NO_REDIRECT; break;
            case 'F':
                verbose_types = VERBOSE; break;
            case 'f':
                sorted = NONE; break;
            case 'h':
                size = VARIABLE; break;
            case 'i':
                inode = SHOW; break;
            case 'k':
                size = KBYTES; break;
            case 'l':
                verbosity = LONG; break;
            case 'n':
                verbosity = NUMERIC; break;
            case 'q':
                non_printable = RAW; break;
            case 'R':
                recursive = RECURSIVE; break;
            case 'r':
                reverse = NORMAL; break;
            case 'S':
                sorted = SIZE; break;
            case 's':
                blocks = BLOCKS; break;
            case 't':
                sorted = TIME_MOD; break;
            case 'u':
                time_disp = ACCESS; break;
            case 'w':
                non_printable = RAW; break;
        }
    }

    if (argc - optind < 1) {
        if ((err = print_info(".")) != 0) {
            exit(err);
        }
        putchar('\n');
    }
    else {
        for (int i = optind; i < argc; i++) {
            item_path = *(argv + i);
            if ((err = print_info(item_path)) != 0) {
                exit(err);
            }
            putchar('\n');
        }
    }
}

int print_info(const char *item_path) {
    int return_flag = 0;
    DIR *dir;
    struct dirent *file;
    struct stat stat_s;
    data_width_t data_width;
    char time_buff[TIME_BUFF_SIZE];
    char permissions[PERMISSION_BUFF_SIZE];

    if ((stat(item_path, &stat_s)) == -1) {
        perror("Couldn't open selected dir");
        exit(BAD_ARGS_ERROR);
    }

    if (S_ISDIR(stat_s.st_mode)) {
        if (read_data_width(&data_width, item_path) != 0) {
            perror("Couldn't open selected directory");
        }

        if ((dir = opendir(item_path)) == NULL) {
            perror("Couldn't open selected directory");
            return DIR_READ_ERROR;
        }

        printf("total %i\n", (int)((data_width.total_size + 512 - 1) / 512)); //incorrect?
        errno = 0;
        while ((file = readdir(dir)) != NULL) {
            if ((statat(item_path, file->d_name, &stat_s)) != 0) {
                break;
            }

            format_permission(stat_s.st_mode, permissions);
            format_time(stat_s.st_mtime, time_buff);

            printf("%s %*lu %*s %-*s %*lu %s %s\n",
                permissions,
                (int)data_width.hard_links, /* number of links width */
                stat_s.st_nlink, /* number of links */
                (int)data_width.username_len,
                getpwuid(stat_s.st_uid)->pw_name,   /* owner id */
                (int)data_width.groupname_len,
                getgrgid(stat_s.st_gid)->gr_name,   /* group id */
                (int)data_width.bytes_len,
                stat_s.st_size,  /* byte size */
                time_buff,
                file->d_name
            );

            errno = 0;
        }

        if (errno != 0) {
            perror("Error reading dir contents");
            return_flag = DIR_READ_ERROR;
        }

        closedir(dir);
    } else if (S_ISREG(stat_s.st_mode)) {
        if ((stat(item_path, &stat_s)) != 0) {
            perror("Error reading file");
            return_flag = FILE_READ_ERROR;
        }

        format_permission(stat_s.st_mode, permissions);
        format_time(stat_s.st_mtime, time_buff);

        printf("%s %*lu %*s %-*s %*lu %s %s\n",
            permissions,
            (int)data_width.hard_links, /* number of links width */
            stat_s.st_nlink, /* number of links */
            (int)data_width.username_len,
            getpwuid(stat_s.st_uid)->pw_name,   /* owner id */
            (int)data_width.groupname_len,
            getgrgid(stat_s.st_gid)->gr_name,   /* group id */
            (int)data_width.bytes_len,
            stat_s.st_size,  /* byte size */
            time_buff,
            item_path
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

void format_permission(const mode_t mode, char *permissions) {
    if (S_ISLNK(mode)) {
        permissions[0] = 'l';
    }
    else if (S_ISDIR(mode)) {
        permissions[0] = 'd';
    }
    else if (S_ISFIFO(mode)) {
        permissions[0] = 'p';
    }
    else if (S_ISBLK(mode)) {
        permissions[0] = 'b';
    }
    else if (S_ISCHR(mode)) {
        permissions[0] = 'c';
    }
    else if (S_ISREG(mode)) {
        permissions[0] = '-';
    }
    else {
        permissions[0] = 's';
    }

    permissions[1] = S_IRUSR & mode ? 'r' : '-';
    permissions[2] = S_IWUSR & mode ? 'w' : '-';
    if (S_ISUID & mode) {
        permissions[3] = (S_IXUSR & mode) ? 's' : 'S';
    }
    else if (S_IXUSR & mode) {
        permissions[3] = 'x';
    }
    else {
        permissions[3] = '-';
    }

    permissions[4] = S_IRGRP & mode ? 'r' : '-';
    permissions[5] = S_IWGRP & mode ? 'w' : '-';
    if (S_ISGID & mode) {
        permissions[6] = (S_IXGRP & mode) ? 's' : 'S';
    }
    else if (S_IXGRP & mode) {
        permissions[6] = 'x';
    }
    else {
        permissions[6] = '-';
    }

    permissions[7] = S_IROTH & mode ? 'r' : '-';
    permissions[8] = S_IWOTH & mode ? 'w' : '-';
    if (__S_ISVTX & mode) {
        permissions[9] = (S_IXOTH & mode) ? 't' : 'T';
    }
    else if (S_IXOTH & mode) {
        permissions[9] = 'x';
    }
    else {
        permissions[9] = '-';
    }

    permissions[10] = '\0';
}

long exp_ten(int n) {
    long result = 1;
    while (n-- > 0) {
        result *= 10;
    }
    return result;
}

void format_time(time_t time_epoch, char *time_buff) {
    struct tm time_s; 

    time_s = *localtime(&time_epoch);
    strftime(time_buff, TIME_BUFF_SIZE, "%b %d %k:%M", &time_s);
}

int read_data_width(data_width_t *data_width, const char *dir_path) {
    DIR *dir;
    struct dirent *file;
    struct stat stat_s;
    int number_of_hard_links;
    int number_of_bytes;
    int unsigned temp;
    size_t string_len;

    data_width->hard_links = 1;
    data_width->bytes_len = 1;
    data_width->username_len = 1;
    data_width->groupname_len = 1;
    data_width->total_size = 0;

    if ((dir = opendir(dir_path)) == NULL) {
        return DIR_READ_ERROR;
    }

    while ((file = readdir(dir)) != NULL) {
        temp = 0;
        if ((statat(dir_path, file->d_name, &stat_s)) != 0) {
            break;
        }

        number_of_hard_links = stat_s.st_nlink;
        while (number_of_hard_links / exp_ten(temp)) {
            temp++;
        }

        if (temp > data_width->hard_links) {
            data_width->hard_links = temp;
        }

        string_len = strlen(getpwuid(stat_s.st_uid)->pw_name);
        
        if (string_len > data_width->username_len) {
            data_width->username_len = string_len;
        }

        string_len = strlen(getgrgid(stat_s.st_gid)->gr_name);
        
        if (string_len > data_width->groupname_len) {
            data_width->groupname_len = string_len;
        }

        number_of_bytes = stat_s.st_size;
        data_width->total_size += number_of_bytes;
        temp = 0;
        while (number_of_bytes / exp_ten(temp)) {
            temp++;
        }

        if (temp > data_width->bytes_len) {
            data_width->bytes_len = temp;
        }
    }

    return 0;
}
