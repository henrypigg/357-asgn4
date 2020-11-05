#include "header.h"

struct bytestream
{
    char uwblock[512]; /* unwritten block */
    int index;         /* offset in unwritten block */
    int fd;            /* output file descriptor */
};

void write_to_stream(struct bytestream *, char *, int);

void write_header(struct bytestream *, struct header *);

void write_contents(struct bytestream *, char *);

void pad_eof(struct bytestream *);

