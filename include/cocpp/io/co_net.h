_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/io/co_io.h"

CO_NAMESPACE_BEGIN

class co_net : public co_io
{
public:
    co_net() = default;
    int socket(int domain, int type, int protocol);
    int accept(struct sockaddr *addr, socklen_t *addrlen);
    int bind(const struct sockaddr *addr, socklen_t addrlen);
    int listen(int backlog);
    int connect(const struct sockaddr *addr, socklen_t addrlen);
    int send(const void *buf, size_t len, int flags);
    int sendto(const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
    int recv(void *buf, size_t len, int flags);
    int recvfrom(void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    int getsockopt(int level, int optname, void *optval, socklen_t *optlen);
    int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
    int shutdown(int how);

    int sendmsg(const struct msghdr *msg, int flags);
    int recvmsg(struct msghdr *msg, int flags);
    int socketpair(int domain, int type, int protocol, int sv[2]);
};

CO_NAMESPACE_END
