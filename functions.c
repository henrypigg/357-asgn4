#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <pwd.h>
#include <math.h>
#include <time.h>
#include <grp.h>
#include "bytestream.h"
#include "macros.h"
#include "specialint.h"

/* struct header *read_header(int file_in) */

void preorder_dfs(char *, struct bytestream *, uint8_t);

void extract_dir(struct header *);

void extract_entry(struct header *, int, uint8_t);

void extract_file(struct header *, int);

void skip_entry(struct header *, int);

void extract_link(struct header *head);

struct header *build_header(char *);

int32_t read_octal(char *octal, int size)
{
    int32_t result, i;

    result = 0;

    for (i = size - 1; i > 0; i--)
    {
        result += (octal[i - 1] - '0') * pow(8, (size - i - 1));
    }

    return result;
}

struct header *read_header(int fd)
{
    char head_buf[512];
    char attribute[155];
    struct header *head;

    memset(attribute, 0, 155);
    memset(head_buf, 0, 512);

    if (read(fd, head_buf, 512) < 0)
    {
        perror("read");
        exit(1);
    }

    strncpy(attribute, head_buf + 257, 5);

    if (strcmp(attribute, "ustar"))
    {
        return NULL;
    }

    /* initialize header */
    head = malloc(sizeof(struct header));

    memset(head->name, 0, 101);
    memset(head->linkname, 0, 100);
    memset(head->uname, 0, 32);
    memset(head->gname, 0, 32);
    memset(head->prefix, 0, 155);

    strncpy(head->name, head_buf, 100); /* copy filename from header */

    strncpy(head->linkname, head_buf + 157, 100);

    strncpy(head->uname, head_buf + 265, 32);

    strncpy(head->gname, head_buf + 297, 32);

    strncpy(head->prefix, head_buf + 345, 155);

    strncpy(attribute, head_buf + 100, 8);
    head->mode = read_octal(attribute, 8);

    strncpy(attribute, head_buf + 108, 8);
    head->uid = read_octal(attribute, 8);

    strncpy(attribute, head_buf + 116, 8);
    head->gid = read_octal(attribute, 8);

    strncpy(attribute, head_buf + 148, 8);
    head->chksum = read_octal(attribute, 8);

    strncpy(attribute, head_buf + 124, 12);
    head->size = read_octal(attribute, 12);

    strncpy(attribute, head_buf + 136, 12);
    head->mtime = read_octal(attribute, 12);

    head->typeflag = head_buf[156];

    return head;
}

void get_permissions(mode_t mode, char type, char *permissions)
{
    memset(permissions, '-', 10);

    if (type == '5')
    {
        permissions[0] = 'd';
    }
    else if (type == '2')
    {
        permissions[0] = 'l';
    }

    if (mode & S_IRUSR)
    {
        permissions[1] = 'r';
    }
    if (mode & S_IWUSR)
    {
        permissions[2] = 'w';
    }
    if (mode & S_IXUSR)
    {
        permissions[3] = 'x';
    }
    if (mode & S_IRGRP)
    {
        permissions[4] = 'r';
    }
    if (mode & S_IWGRP)
    {
        permissions[5] = 'w';
    }
    if (mode & S_IXGRP)
    {
        permissions[6] = 'x';
    }
    if (mode & S_IROTH)
    {
        permissions[7] = 'r';
    }
    if (mode & S_IWOTH)
    {
        permissions[8] = 'w';
    }
    if (mode & S_IXOTH)
    {
        permissions[9] = 'x';
    }
}

int contains(char **files, int num_of_files, char *prefix, char *name)
{
    int i;
    char buf[256];

    memset(buf, 0, 256);

    for (i = 0; i < num_of_files; i++)
    {
        if (strlen(prefix))
        {
            strcpy(buf, prefix);
            strcat(buf, "/");
            strncat(buf, name, strlen(files[i]) - strlen(prefix));
        }
        else
        {
            strncpy(buf, name, strlen(files[i]));
        }

        if (strcmp(files[i], buf) == 0)
        {
            return 1;
        }

        memset(buf, 0, 256);
    }

    return 0;
}

int verify_archive(char *filename, uint8_t args)
{
    int fd;
    char buf[512];

    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        perror(filename);
        exit(1);
    }

    if (read(fd, buf, 512) < 0)
    {
        perror("read");
        exit(1);
    }

    if ((strncmp(buf + 257, "ustar", 5)))
    {
        if (close(fd))
        {
            perror("close");
            exit(1);
        }
        return -1;
    }

    if (args & STRICT_SET)
    {
        if (buf[262] != '\0' || strncmp(buf + 263, "00", 2))
        {
            if (close(fd))
            {
                perror("close");
                exit(1);
            }
            return -1;
        }
    }

    if (close(fd))
    {
        perror("close");
        exit(1);
    }

    return 0;
}

void list_toc(char *archivefile, char **pathnames,
              int num_of_files, uint8_t args)
{
    int fd, offset;
    struct header *head;
    struct tm *filetime;
    char *permissions;
    char *time_buf;
    char owner_group[17];

    permissions = malloc(10);

    if (verify_archive(archivefile, args) < 0)
    {
        exit(1);
    }

    if ((fd = open(archivefile, O_RDONLY)) < 0)
    {
        perror(archivefile);
        exit(1);
    }

    while ((head = read_header(fd)))
    {
        if (contains(pathnames, num_of_files, head->prefix,
                     head->name) ||
            num_of_files == 0)
        {
            if (args & VERB_SET)
            {
                get_permissions(head->mode, head->typeflag, permissions);

                strcpy(owner_group, head->uname);
                strcat(owner_group, "/");
                strcat(owner_group, head->gname);

                time_buf = ctime(&head->mtime);
                time_buf[strlen(time_buf) - 1] = '\0';

                filetime = localtime(&head->mtime);

                /* permissions, owner, group, and size */
                printf("%s %-17s %8d ", permissions, owner_group,
                                                 (int)head->size);

                /* time */
                printf("%04d-%02d-%02d %02d:%02d", 1900 +
                       (int)filetime->tm_year,
                       (int)filetime->tm_mon + 1, (int)filetime->tm_mday,
                       (int)filetime->tm_hour, (int)filetime->tm_min);

                /* path */
                if (strlen(head->prefix))
                {
                    printf(" %s/%s\n", head->prefix, head->name);
                }
                else
                {
                    printf(" %s\n", head->name);
                }
            }
            else
            {
                if (strlen(head->prefix))
                {
                    printf(" %s/%s\n", head->prefix, head->name);
                }
                else
                {
                    printf(" %s\n", head->name);
                }
            }
        }

        offset = head->size;
        if (head->size % 512)
        {
            offset += (512 - head->size % 512);
        }

        if (head->typeflag != '5' && head->typeflag != '2')
        {
            lseek(fd, offset, SEEK_CUR);
        }
        free(head);
    }

    free(permissions);
}

void create_archive(char **paths, int num_of_files,
                    char *filename, uint8_t args)
{
    /*  creates archive of path
        args:
            path (char *): path to archive
            filename (char *): filename of archive file
            options (uint8_t): options of file
    */

    struct bytestream *bs;
    struct header *head;
    int fd, i;

    /* open archive file */
    if ((fd = open(filename, (O_WRONLY | O_CREAT | O_TRUNC),
                   (S_IRUSR | S_IWUSR))) < 0)
    {
        perror(filename);
        exit(1);
    }

    /* initialize bytestream for writing to output */
    bs = malloc(sizeof(struct bytestream));
    memset(bs->uwblock, 0, 512);
    bs->index = 0;
    bs->fd = fd;

    for (i = 0; i < num_of_files; i++)
    {
        /* build header with root path */
        if ((head = build_header(paths[i])) != NULL)
        {
            /* write header and file contents to output */
            write_header(bs, head);
            if (head->typeflag == '0')
            {
                write_contents(bs, paths[i]);
            }

            /* print entry if verbose is set */
            if (args & VERB_SET)
            {
                printf("%s/\n", paths[i]);
            }

            free(head);
            /* traverse path with preorder dfs and add every file to archive */
            preorder_dfs(paths[i], bs, args);
        }
    }

    /* pad end of archive file with 2 blocks of 512 bytes */
    pad_eof(bs);

    free(bs);
}

char get_type(mode_t mode)
{
    /*  returns file type from mode_t 
        args:
            mode (mode_t): st_mode of file
        returns:
            char: '0': regular file, '2': symbolic link, '5': directory
    */

    if (S_ISREG(mode))
    {
        return '0';
    }
    else if (S_ISLNK(mode))
    {
        return '2';
    }
    else if (S_ISDIR(mode))
    {
        return '5';
    }
    return 0;
}

struct header *build_header(char *path)
{
    /*  builds header struct from path 
        args:
            path (char *): path of file
        returns:
            struct header: header struct built from file
    */

    struct header *head;
    struct stat statbuf;
    struct passwd *pws;
    struct group *grp;
    int sz, offset;
    char buff[100];
    char *temp;
    char type;

    /* initialize header */
    head = malloc(sizeof(struct header));
    memset(head->name, 0, 101);
    memset(head->linkname, 0, 100);
    memset(head->uname, 0, 32);
    memset(head->gname, 0, 32);
    memset(head->prefix, 0, 155);
    memset(buff, 0, 100);

    /* stat file */
    if (lstat(path, &statbuf) < 0)
    {
        perror("lstat");
        return NULL;
    }

    /* fill header struct */
    temp = path;
    offset = 0;

    while ((strlen(path) > 99) &&
           (offset < (strlen(path) - 99) || *temp != '/'))
    {
        offset++;
        temp++;
    }
    if (offset > 0)
    {
        strncpy(head->prefix, path, offset);
        strncpy(head->name, path + offset + 1, strlen(path) - offset - 1);
    }
    else
    {
        strcpy(head->name, path);
    }

    if ((type = get_type(statbuf.st_mode)) != 0)
    {
        head->typeflag = type;
    }

    head->mode = statbuf.st_mode & 0x1FF;
    head->uid = statbuf.st_uid;
    head->gid = statbuf.st_gid;

    if (head->typeflag == '0')
    {
        head->size = statbuf.st_size;
    }
    else
    {
        head->size = 0;
        strcat(head->name, "/");
    }

    head->mtime = statbuf.st_mtime;

    if (head->typeflag == '2')
    {
        if ((sz = readlink(path, buff, 100)) < 0)
        {
            perror("readlink");
            exit(1);
        }

        strcpy(head->linkname, buff);
    }
    pws = getpwuid(statbuf.st_uid);
    strcpy(head->uname, pws->pw_name);
    grp = getgrgid(statbuf.st_gid);
    strcpy(head->gname, grp->gr_name);

    return head;
}

void print_header(struct header *head)
{
    printf("------------------------------\n");
    printf("Path: %s\n", head->name);
    printf("Mode: %o\n", (unsigned int)head->mode);
    printf("UID: %o\n", (unsigned int)head->uid);
    printf("GID: %o\n", (unsigned int)head->gid);
    printf("Size: %o\n", (unsigned int)head->size);
    printf("mTime: %o\n", (unsigned int)head->mtime);
    printf("Type: %c\n", head->typeflag);
    printf("uname: %s\n", head->uname);
    printf("gname: %s\n", head->gname);
    printf("------------------------------\n\n");
}

void preorder_dfs(char *path, struct bytestream *bs, uint8_t args)
{
    /*  traverses directory with preorder dfs and adds entries to archive 
        attributes:
            path (char *): current directory
            bs (struct bytestream *): output bytestream
            options (uint8_t): file options
    */

    char new_path[1024];
    struct dirent *dp;
    DIR *dir;
    struct stat statbuf;
    struct header *head;

    /* check if path is directory, return if not */
    stat(path, &statbuf);
    if (S_ISDIR(statbuf.st_mode))
    {
        if ((dir = opendir(path)) == NULL)
        {
            perror(path);
            return;
        }
    }
    else
    {
        return;
    }

    /* read every directory entry and add them to the archive */
    while ((dp = readdir(dir)))
    {
        /* skip over . and .. entries */
        if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
        {
            /* build new path with new filename */
            strcpy(new_path, path);
            strcat(new_path, "/");
            strcat(new_path, dp->d_name);

            /* write header and contents to archive */
            if ((head = build_header(new_path)))
            {
                write_header(bs, head);

                if (head->typeflag == '0')
                {
                    write_contents(bs, new_path);
                }

                /* print path if verbose is set */
                if (args & VERB_SET)
                {
                    printf("%s/\n", new_path);
                }

                free(head);
            }

            /* recurse with new path */
            preorder_dfs(new_path, bs, args);
        }
    }

    closedir(dir);
}

void extract_entry(struct header *head, int f_in, uint8_t options)
{

    if (options & VERB_SET)
    {
        printf("%s\n", head->name);
    }
    if (head->typeflag == '5')
    {
        /*this is a dirrectory*/
        extract_dir(head);
    }

    if (head->typeflag == '0' || head->typeflag == '\0')
    {
        /*not dirrectory write contents to file */
        extract_file(head, f_in);
    }

    if (head->typeflag == '2')
    {
        /*symbolic link (do not write contents of file)*/
        extract_link(head);
    }
}

void extract_link(struct header *head)
{
    char buf[512];
    char full_link[100];
    if (strlen(head->name) > 99 && strlen(head->prefix) != 0)
    {
        strcpy(full_link, head->prefix);
        strcat(full_link, "/");
        strcat(full_link, head->name);
    }
    else
    {
        strcpy(full_link, head->name);
    }
    readlink(head->linkname, buf, 512);
    if (symlink(full_link, buf) != 0)
    {
        perror("symlink error");
        exit(1);
    }
}

void extract_dir(struct header *head)
{
    struct stat temp_stat;
    char new_dir[256];
    if (strlen(head->prefix))
    {
        strcpy(new_dir, "");
        strncat(new_dir, head->prefix, strlen(head->prefix));
        strncat(new_dir, "/", 1);
        strncat(new_dir, head->name, strlen(head->name));
    }
    else
    {
        strcpy(new_dir, "");
        strncat(new_dir, head->name, strlen(head->name));
    }
    /*printf("start extract dir");*/
    stat(new_dir, &temp_stat);
    if (!(temp_stat.st_mode & F_OK))
    {
        mkdir(new_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    else
    {
        perror("cant create directory");
    }
}

void create_extra_dir(char extra_dirs[512])
{
    char delimiter[] = "/";
    struct stat temp_stat;
    char temp_path[512];
    char path[512];
    char *ptr;
    strcpy(path, extra_dirs);

    ptr = strtok(path, delimiter);
    strcpy(temp_path, "");

    while (ptr != NULL)
    {

        strcat(temp_path, ptr);

        /*check if dirrectory exists*/
        stat(temp_path, &temp_stat);
        if (!(S_ISDIR(temp_stat.st_mode)) && strcmp(temp_path, extra_dirs) != 0)
        {
            /*director does not exitst ... create new dir */
            /*fprintf(stderr, "making new dir %s \n", ptr);*/
            mkdir(temp_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }
        strcat(temp_path, "/");

        ptr = strtok(NULL, delimiter);
    }

    free(ptr);
}

void extract_file(struct header *head, int f_in)
{
    char new_dir[512];
    struct bytestream *bs;
    int remainder;
    char buf[512];
    int blocks;
    int i, rd, fd;
    i = 0;
    bs = malloc(sizeof(struct bytestream));
    memset(bs->uwblock, 0, 512);
    if (strlen(head->prefix))
    {
        strcpy(new_dir, "");
        strncat(new_dir, head->prefix, strlen(head->prefix));
        strcat(new_dir, "/");
        strncat(new_dir, head->name, strlen(head->name));
    }
    else
    {
        strcpy(new_dir, "");
        strncat(new_dir, head->name, strlen(head->name));
    }
    /*fprintf(stderr, "extract_file : %s \n", new_dir);*/

    /*fprintf(stderr,"trying to create file with path : %s \n", new_dir);*/
    create_extra_dir(new_dir);
    if ((fd = open(new_dir, O_WRONLY | O_CREAT | O_TRUNC, head->mode)) < 0)
    {
        perror(new_dir);
    }
    bs->index = 0;

    bs->fd = fd;

    /*calculate the number of full blocks*/
    blocks = head->size / 512;
    /*calculat the remaining bytes*/
    remainder = head->size % 512;
    i = 0;
    while (i < blocks)
    {
        /* read block of file into buffer*/
        if ((rd = read(f_in, buf, 512)) < 0)
        {
            perror("read error");
            exit(1);
        }
        /*write contents of block to stream*/
        write_to_stream(bs, buf, 512);
        i++;
    }

    /*read contents remainder of file*/
    rd = read(f_in, buf, remainder);
    write_to_stream(bs, buf, remainder);

    /*write remaing contents of stream to file */
    if (bs->index > 0)
    {
        /*write remaining values in stream to file*/
        if (write(bs->fd, bs->uwblock, bs->index) == -1)
        {
            perror("write");
            exit(1);
        }
        memset(bs->uwblock, 0, bs->index);
    }
    /* attempt to read remaining null bits in block*/
    read(f_in, buf, 512 - remainder);
    free(bs);
}

void skip_entry(struct header *head, int f_in)
{
    int remainder;
    char buf[512];
    int blocks;
    int i, rd;
    /*fprintf("skiping entry: %s", head->name);*/
    if (head->typeflag == '5' || head->typeflag == '2')
    {
        return;
    }
    /*calculate the number of full blocks*/
    blocks = head->size / 512;
    /*calculat the remaining bytes*/
    remainder = head->size % 512;
    while (i < blocks)
    {
        /* read block of file into buffer*/
        if ((rd = read(f_in, buf, 512)) == -1)
        {
            perror("read error");
            exit(1);
        }

        i++;
    }

    /*read contents remainder of file*/
    rd = read(f_in, buf, remainder);

    /* attempt to read remaining null bits in block*/
    read(f_in, buf, 512 - remainder);
}

void extract_archive(char *filename, uint8_t options,
                     char **filenames, int argc)
{
    /*this function loops through the entries in the archive and decides
      which ones to skip based of
      of the filenames argument*/
    struct header *head;
    int f_in;

    if ((f_in = open(filename, O_RDONLY)) < 0)
    {
        perror(filename);
        exit(1);
    }

    while ((head = read_header(f_in)))
    {
        if (((options | LIST_SET) && (contains(filenames,
            argc, head->prefix, head->name))) || argc == 0)
        {
            extract_entry(head, f_in, options);
        }
        else
        {
            /*skip entry*/
            skip_entry(head, f_in);
        }

        free(head);
    }
    close(f_in);
}
