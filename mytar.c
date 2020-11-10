#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "functions.h"
#include "macros.h"

char parse_options(char *args)
{
    uint8_t options = 0;
    char *temp;

    temp = args;

    while (*temp != '\0')
    {
        if (*temp == 'c')
        {
            options = options | CREATE_SET;

            if ((options & TOC_SET) || (options & EXTRACT_SET))
            {
                perror("you may only choose one of the 'ctx' options");
                printf("usage: [ctxSp[f tarfile]] [file1 [ file2 [...] ] ]\n");
                exit(1);
            }
        }
        else if (*temp == 't')
        {
            options = options | TOC_SET;

            if ((options & CREATE_SET) || (options & EXTRACT_SET))
            {
                perror("you may only choose one of the 'ctx' options");
                printf("usage: [ctxSp[f tarfile]] [file1 [ file2 [...] ] ]\n");
                exit(1);
            }
        }
        else if (*temp == 'x')
        {
            options = options | EXTRACT_SET;

            if ((options & TOC_SET) || (options & CREATE_SET))
            {
                perror("you may only choose one of the 'ctx' options");
                printf("usage: [ctxSp[f tarfile]] [file1 [ file2 [...] ] ]\n");
                exit(1);
            }
        }
        else if (*temp == 'v')
        {
            options = options | VERB_SET;
        }
        else if (*temp == 'S')
        {
            options = options | STRICT_SET;
        }

        temp++;
    }

    return options;
}

int main(int argc, char *argv[])
{
    uint8_t options;
    char *filenames[1028];
    int i;

    options = parse_options(argv[1]);

    if (!((options & CREATE_SET) || (options & TOC_SET)
                                 || (options & EXTRACT_SET)))
    {
        perror("you must choose one of the 'ctx' options");
        printf("usage: [ctxSp[f tarfile]] [file1 [ file2 [...] ] ]\n");
        exit(1);
    }

    if (options & CREATE_SET)
    {
        for (i = 3; i < argc; i++)
        {
            filenames[i - 3] = malloc(strlen(argv[i]));
            strcpy(filenames[i - 3], argv[i]);
        }
        create_archive(filenames, argc - 3, argv[2], options);
        for (i = 0; i < argc - 3; i++)
        {
            free(filenames[i]);
        }
    }

    if (options & TOC_SET)
    {
        for (i = 3; i < argc; i++)
        {
            filenames[i - 3] = malloc(strlen(argv[i]));
            strcpy(filenames[i - 3], argv[i]);
        }
        list_toc(argv[2], filenames, argc - 3, options);
        for (i = 0; i < argc - 3; i++)
        {
            free(filenames[i]);
        }
    }

    if (options & EXTRACT_SET)
    {
        for (i = 3; i < argc; i++)
        {
            filenames[i - 3] = malloc(strlen(argv[i]));
            strcpy(filenames[i - 3], argv[i]);
        }
        extract_archive(argv[2], options, filenames, argc - 3);
        for (i = 0; i < argc - 3; i++)
        {
            free(filenames[i]);
        }
    }

    return 0;
}
