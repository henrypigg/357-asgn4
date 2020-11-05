#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "header.h"
#include "specialint.h"

struct bytestream
{
    char uwblock[512]; /* unwritten block */
    int index;         /* offset in unwritten block */
    int fd;            /* output file descriptor */
};

void write_to_stream(struct bytestream *bs, char *bytes, int size)
{
    /*  writes given number of bytes to buffer and writes buffer if full 
        args:
            bs (struct bytestream *): output bytestream
            bytes (char *): bytes to write to output
            size (int): number of bytes to write
    */

    int i;

    for (i = 0; i < size; i++)
    {
        /* if uwblock is full, write it to the output and empty it */
        if (bs->index >= 512)
        {
            if (write(bs->fd, bs->uwblock, 512) == -1)
            {
                perror("write");
                exit(1);
            }
            memset(bs->uwblock, 0, 512);
            bs->index = 0;
        }

        /* add byte to stream */
        bs->uwblock[bs->index] = bytes[i];

        bs->index++;
    }
}

int32_t get_chksum(struct bytestream *bs, struct header *h)
{
    int32_t sum = 0;
    int i;
    char *temp;

    for (i = 0; i < bs->index; i++)
    {
        sum += bs->uwblock[i];
    }

    sum += (unsigned char)h->typeflag;

    temp = h->linkname;

    while (*temp != '\0')
    {
        sum += *temp;
        temp++;
    }

    /* value of "ustar" */
    sum += 559;

    sum += ((int)'0') * 2;
    sum += ((int)' ') * 8;

    temp = h->uname;

    while (*temp != '\0')
    {
        sum += *temp;
        temp++;
    }

    temp = h->gname;

    while (*temp != '\0')
    {
        sum += *temp;
        temp++;
    }

    temp = h->prefix;

    while (*temp != '\0')
    {
        sum += *temp;
        temp++;
    }

    return sum;
}

void write_header(struct bytestream *bs, struct header *h)
{
    /*  writes header to output
        args:
            bs (struct bytestream *): output bytestream
            h (struct header *): header to write
    */

    char byte[12];
    char byte8[8];
    int i, n;
    char temp;

    write_to_stream(bs, h->name, 100);

    /* mode in octal */
    memset(byte, 0, 12);
    i = 0;
    n = h->mode;
    /* build octal number in reverse*/
    while (n != 0)
    {
        byte[i] = (char)(n % 8) + '0';
        n = n / 8;
        i++;
    }
    /* reverse octal number */
    for (i = 0; i < strlen(byte) / 2; i++)
    {
        temp = byte[i];
        byte[i] = byte[strlen(byte) - i - 1];
        byte[strlen(byte) - i - 1] = temp;
    }
    /* pad with zeros */
    for (i = 0; i < 7 - strlen(byte); i++)
    {
        write_to_stream(bs, "0", 1);
    }
    write_to_stream(bs, byte, strlen(byte));
    write_to_stream(bs, "\0", 1);

    /* uid in octal */
    if (h->uid > 2097151)
    {
        /* too big for 7 digit octal */
        insert_special_int(byte8, 8, (int32_t)h->uid);
        write_to_stream(bs, byte8, 8);
    }
    else
    {
        memset(byte, 0, 12);
        i = 0;
        n = h->uid;
        /* build octal number in reverse*/
        while (n != 0)
        {
            byte[i] = (char)(n % 8) + '0';
            n = n / 8;
            i++;
        }
        /* reverse octal number */
        for (i = 0; i < strlen(byte) / 2; i++)
        {
            temp = byte[i];
            byte[i] = byte[strlen(byte) - i - 1];
            byte[strlen(byte) - i - 1] = temp;
        }
        /* pad with zeros */
        for (i = 0; i < 7 - strlen(byte); i++)
        {
            write_to_stream(bs, "0", 1);
        }
        write_to_stream(bs, byte, strlen(byte));
        write_to_stream(bs, "\0", 1);
    }

    /* gid in octal */
    memset(byte, 0, 12);
    i = 0;
    n = h->gid;
    while (n != 0)
    {
        byte[i] = (char)(n % 8) + '0';
        n = n / 8;
        i++;
    }
    for (i = 0; i < strlen(byte) / 2; i++)
    {
        temp = byte[i];
        byte[i] = byte[strlen(byte) - i - 1];
        byte[strlen(byte) - i - 1] = temp;
    }
    for (i = 0; i < 7 - strlen(byte); i++)
    {
        write_to_stream(bs, "0", 1);
    }
    write_to_stream(bs, byte, strlen(byte));
    write_to_stream(bs, "\0", 1);

    /* size in octal */
    memset(byte, 0, 12);
    i = 0;
    n = h->size;
    while (n != 0)
    {
        byte[i] = (char)(n % 8) + '0';
        n = n / 8;
        i++;
    }
    for (i = 0; i < strlen(byte) / 2; i++)
    {
        temp = byte[i];
        byte[i] = byte[strlen(byte) - i - 1];
        byte[strlen(byte) - i - 1] = temp;
    }
    for (i = 0; i < 11 - strlen(byte); i++)
    {
        write_to_stream(bs, "0", 1);
    }
    write_to_stream(bs, byte, strlen(byte));
    write_to_stream(bs, "\0", 1);

    /* mtime in octal */
    memset(byte, 0, 12);
    i = 0;
    n = h->mtime;
    while (n != 0)
    {
        byte[i] = (char)(n % 8) + '0';
        n = n / 8;
        i++;
    }
    for (i = 0; i < strlen(byte) / 2; i++)
    {
        temp = byte[i];
        byte[i] = byte[strlen(byte) - i - 1];
        byte[strlen(byte) - i - 1] = temp;
    }
    for (i = 0; i < 11 - strlen(byte); i++)
    {
        write_to_stream(bs, "0", 1);
    }
    write_to_stream(bs, byte, strlen(byte));
    write_to_stream(bs, "\0", 1);

    /* checksum in octal */
    memset(byte, 0, 12);
    i = 0;
    n = get_chksum(bs, h);
    while (n != 0)
    {
        byte[i] = (char)(n % 8) + '0';
        n = n / 8;
        i++;
    }
    for (i = 0; i < strlen(byte) / 2; i++)
    {
        temp = byte[i];
        byte[i] = byte[strlen(byte) - i - 1];
        byte[strlen(byte) - i - 1] = temp;
    }
    for (i = 0; i < 7 - strlen(byte); i++)
    {
        write_to_stream(bs, "0", 1);
    }
    write_to_stream(bs, byte, strlen(byte));
    write_to_stream(bs, "\0", 1);

    write_to_stream(bs, (char *)&h->typeflag, 1);

    write_to_stream(bs, h->linkname, 100);

    write_to_stream(bs, "ustar\0", 6);

    write_to_stream(bs, "00", 2);

    write_to_stream(bs, h->uname, 32);

    write_to_stream(bs, h->gname, 32);

    /* dev major */
    memset(byte8, 0, 8);
    write_to_stream(bs, byte8, 8);

    /*devminor*/
    write_to_stream(bs, byte8, 8);

    write_to_stream(bs, h->prefix, 155);

    if (write(bs->fd, bs->uwblock, 512) == -1)
    {
        perror("write");
        exit(1);
    }
    memset(bs->uwblock, 0, 512);
    bs->index = 0;
}

void write_contents(struct bytestream *bs, char *filename)
{
    /*  writes file contents to output
        args:
            bs (struct bytestream *): output bytestream
            filename (char *): file to write
    */

    int fd, rd;
    struct stat statbuf;
    char buf[4096];

    memset(buf, 0, 4096);

    /* open file to read */
    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        perror(filename);
        exit(1);
    }

    fstat(fd, &statbuf);

    /* if file is a directory, don't print file contents */
    if (S_ISDIR(statbuf.st_mode) || S_ISLNK(statbuf.st_mode))
    {
        return;
    }

    /* read blocks of 4096 from file and write them to archive */
    if ((rd = read(fd, buf, 4096)) < 0)
    {
        perror("read");
        exit(1);
    }
    while (rd > 0)
    {
        write_to_stream(bs, buf, rd);

        if ((rd = read(fd, buf, 4096)) < 0)
        {
            perror("read");
            exit(1);
        }
    }

    if (bs->index > 0)
    {
        if (write(bs->fd, bs->uwblock, 512) == -1)
        {
            perror("write");
            exit(1);
        }
        memset(bs->uwblock, 0, 512);
        bs->index = 0;
    }
}

void pad_eof(struct bytestream *bs)
{
    /*  write two blocks of 512 null bytes
        args:
            bs (struct bytestream *): output bytestream
    */

    char pad[1028];

    memset(pad, 0, 1028);

    if (bs->index > 0)
    {
        if (write(bs->fd, bs->uwblock, 512) == -1)
        {
            perror("write");
            exit(1);
        }
        memset(bs->uwblock, 0, 512);
        bs->index = 0;
    }

    if (write(bs->fd, pad, 1028) == -1)
    {
        perror("write");
        exit(1);
    }
}
