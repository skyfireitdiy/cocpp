_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_error.h"
#include <unistd.h>
#include <fcntl.h>

CO_NAMESPACE_BEGIN

class co_io
{
protected:
    int fd__ = {-1};

public:
    co_io() = default;

    int init_fd(int fd);

    virtual int close();
    virtual int read(void *buf, size_t count);
    virtual int write(const void *buf, size_t count);
    virtual int lseek(off_t offset, int whence);
    virtual int fchmod(mode_t mode);
    virtual int fchown(uid_t owner, gid_t group);
    virtual int fcntl(int cmd, ...);
    virtual int ioctl(int cmd, ...);
    virtual int ftruncate(off_t length);
    virtual int fstat(struct stat *buf);
    virtual int fsync();
    virtual int fdatasync();

    virtual ~co_io() = default;
};

CO_NAMESPACE_END
