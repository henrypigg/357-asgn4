#include <stdint.h>
#include <sys/stat.h>

struct header
{
    char name[100];
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    time_t mtime;
    int32_t chksum;
    char typeflag;
    char linkname[101];
    char version[2];
    char uname[32];
    char gname[32];
    char prefix[155];
};
