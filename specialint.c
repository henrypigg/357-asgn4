#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>

uint64_t extract_special_int(char *where, int len)
{
    /* For interoperability with GNU tar. GNU seems to
    * set the high–order bit of the first byte, then
    * treat the rest of the field as a binary integer
    * in network byte order.
    * I don’t know for sure if it’s a 32 or 64–bit int, but for
    * this version, we’ll only support 32. (well, 31)
    * returns the integer on success, –1 on failure.
    * In spite of the name of htonl(), it converts int32 t
    */
    int64_t val = -1;

    if ((len >= sizeof(val)) && (where[0] & 0x80))
    {
        /* the top bit is set and we have space
        * extract the last four bytes */
        val = *(int64_t *)(where + len - sizeof(val));
        val = ntohl(val); /* convert to host byte order */
    }

    return val;
}

int insert_special_int(char *where, size_t size, int32_t val)
{
    /* For interoperability with GNU tar. GNU seems to
    * set the high–order bit of the first byte, then
    * treat the rest of the field as a binary integer
    * in network byte order.
    * Insert the given integer into the given field
    * using this technique. Returns 0 on success, nonzero
    * otherwise
    */
    int err = 0;

    if (val < 0 || (size < sizeof((int32_t)val)))
    {
        err++;
    }
    else
    {
        memset(where, 0, size);
        *(int32_t *)(where + size - sizeof(val)) = htonl(val);
        *where |= 0x80;
    }

    return err;
}
