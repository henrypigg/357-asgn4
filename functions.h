#include "bytestream.h"

void preorder_dfs(char *, struct bytestream *, uint8_t);

void create_archive(char **, int, char *, uint8_t);

void list_toc(char *, char **, int, uint8_t);

void extract_archive(char *, uint8_t, char **, int);

void extract_dir(struct header *);

void extract_entry(struct header *, int, uint8_t);

void extract_file(struct header *, int);

void skip_entry(struct header *, int);

void extract_link(struct header *head);
