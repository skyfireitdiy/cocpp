#include <gtest/gtest.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <optional>
#include <sys/socket.h>
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/io/co_net.h"
#include "cocpp/co/co_co.h"

using namespace cocpp;

auto net_test_env = co::create_env(true);

TEST(net, socket)
{
    auto net = co_net::socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_NE(net, std::nullopt);
    net->close();
}

TEST(net, tcp_socket)
{

    co server_co({with_bind_env(net_test_env)}, [&]() {
        auto server = co_net::socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_NE(server, std::nullopt);
        int reuse_addr = 1;
        int ret = server->setsockopt(SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
        EXPECT_EQ(ret, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8888);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        ret = server->bind((struct sockaddr *)&addr, sizeof(addr));
        EXPECT_EQ(ret, 0);
        ret = server->listen(128);
        EXPECT_EQ(ret, 0);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        auto client = server->accept((struct sockaddr *)&client_addr, &len);
        EXPECT_NE(client, std::nullopt);

        char buf[1024] = {0};
        ret = client->recv(buf, sizeof(buf), 0);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf, "hello");

        ret = client->send("world", 5, 0);
        EXPECT_EQ(ret, 5);
        server->shutdown(SHUT_RDWR);
        server->close();
        client->shutdown(SHUT_RDWR);
        client->close();
    });

    co client_co({with_bind_env(net_test_env)}, [&]() {
        auto client = co_net::socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_NE(client, std::nullopt);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8888);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        int ret = client->connect((struct sockaddr *)&addr, sizeof(addr));
        EXPECT_EQ(ret, 0);
        ret = client->send("hello", 5, 0);
        EXPECT_EQ(ret, 5);
        char buf[1024] = {0};
        ret = client->recv(buf, sizeof(buf), 0);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf, "world");
        client->shutdown(SHUT_RDWR);
        client->close();
    });

    server_co.join();
    client_co.join();
}

TEST(net, udp_socket)
{
    co server_co({with_bind_env(net_test_env)}, [&]() {
        auto server = co_net::socket(AF_INET, SOCK_DGRAM, 0);
        EXPECT_NE(server, std::nullopt);
        int reuse_addr = 1;
        int ret = server->setsockopt(SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
        EXPECT_EQ(ret, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8889);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        ret = server->bind((struct sockaddr *)&addr, sizeof(addr));
        EXPECT_EQ(ret, 0);
        char buf[1024] = {0};
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        ret = server->recvfrom(buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &len);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf, "hello");
        ret = server->sendto("world", 5, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        EXPECT_EQ(ret, 5);
        server->shutdown(SHUT_RDWR);
        server->close();
    });

    co client_co({with_bind_env(net_test_env)}, [&]() {
        auto client = co_net::socket(AF_INET, SOCK_DGRAM, 0);
        EXPECT_NE(client, std::nullopt);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8889);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        int ret = client->sendto("hello", 5, 0, (struct sockaddr *)&addr, sizeof(addr));
        EXPECT_EQ(ret, 5);
        char buf[1024] = {0};
        socklen_t len = sizeof(addr);
        ret = client->recvfrom(buf, sizeof(buf), 0, (struct sockaddr *)&addr, &len);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf, "world");
        client->shutdown(SHUT_RDWR);
        client->close();
    });

    server_co.join();
    client_co.join();
}

TEST(net, sendmsg_on_tcp)
{
    co server_co({with_bind_env(net_test_env)}, [&]() {
        auto server = co_net::socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_NE(server, std::nullopt);
        int reuse_addr = 1;
        int ret = server->setsockopt(SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
        EXPECT_EQ(ret, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8890);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        ret = server->bind((struct sockaddr *)&addr, sizeof(addr));
        EXPECT_EQ(ret, 0);
        ret = server->listen(128);
        EXPECT_EQ(ret, 0);
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        auto client = server->accept((struct sockaddr *)&client_addr, &len);
        EXPECT_NE(client, std::nullopt);

        // recvmsg
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        struct iovec iov;
        char buf[1024];
        iov.iov_base = buf;
        iov.iov_len = 1024;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        ret = client->recvmsg(&msg, 0);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf, "hello");

        // sendmsg
        struct msghdr msg2;
        memset(&msg2, 0, sizeof(msg2));
        struct iovec iov2;
        char buf2[1024] = "world";
        iov2.iov_base = buf2;
        iov2.iov_len = 5;
        msg2.msg_iov = &iov2;
        msg2.msg_iovlen = 1;
        ret = client->sendmsg(&msg2, 0);
        EXPECT_EQ(ret, 5);

        server->shutdown(SHUT_RDWR);
        server->close();

        client->shutdown(SHUT_RDWR);
        client->close();
    });

    co client_co({with_bind_env(net_test_env)}, [&]() {
        auto client = co_net::socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_NE(client, std::nullopt);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8890);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        int ret = client->connect((struct sockaddr *)&addr, sizeof(addr));
        EXPECT_EQ(ret, 0);

        // sendmsg
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        struct iovec iov;
        char buf[1024] = "hello";
        iov.iov_base = buf;
        iov.iov_len = 5;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        ret = client->sendmsg(&msg, 0);
        EXPECT_EQ(ret, 5);

        // recvmsg
        struct msghdr msg2;
        memset(&msg2, 0, sizeof(msg2));
        struct iovec iov2;
        char buf2[1024];
        iov2.iov_base = buf2;
        iov2.iov_len = 1024;
        msg2.msg_iov = &iov2;
        msg2.msg_iovlen = 1;
        ret = client->recvmsg(&msg2, 0);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf2, "world");

        client->shutdown(SHUT_RDWR);
        client->close();
    });
}

TEST(net, sendmsg_on_udp)
{
    co server_co({with_bind_env(net_test_env)}, [&]() {
        auto server = co_net::socket(AF_INET, SOCK_DGRAM, 0);
        EXPECT_NE(server, std::nullopt);
        int reuse_addr = 1;
        int ret = server->setsockopt(SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
        EXPECT_EQ(ret, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8891);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        struct sockaddr_in client_addr;
        client_addr.sin_port = htons(8892);

        ret = server->bind((struct sockaddr *)&addr, sizeof(addr));
        EXPECT_EQ(ret, 0);

        // recvmsg
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        struct iovec iov;
        char buf[1024];
        iov.iov_base = buf;
        iov.iov_len = 1024;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        ret = server->recvmsg(&msg, 0);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf, "hello");

        // sendmsg
        struct msghdr msg2;
        memset(&msg2, 0, sizeof(msg2));
        struct iovec iov2;
        char buf2[1024] = "world";
        iov2.iov_base = buf2;
        iov2.iov_len = 5;
        msg2.msg_iov = &iov2;
        msg2.msg_iovlen = 1;
        msg2.msg_name = (void *)&client_addr;
        msg2.msg_namelen = sizeof(client_addr);
        ret = server->sendmsg(&msg2, 0);
        EXPECT_EQ(ret, 5);

        server->shutdown(SHUT_RDWR);
        server->close();
    });

    co client_co({with_bind_env(net_test_env)}, [&]() {
        auto client = co_net::socket(AF_INET, SOCK_DGRAM, 0);
        EXPECT_NE(client, std::nullopt);
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(8891);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        struct sockaddr_in client_addr = server_addr;
        client_addr.sin_port = htons(8892);

        int reuse_addr = 1;
        int ret = client->setsockopt(SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
        EXPECT_EQ(ret, 0);
        ret = client->bind((struct sockaddr *)&client_addr, sizeof(client_addr));
        EXPECT_EQ(ret, 0);

        // sendmsg
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        struct iovec iov;
        char buf[1024] = "hello";
        iov.iov_base = buf;
        iov.iov_len = 5;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_name = (void *)&server_addr;
        msg.msg_namelen = sizeof(server_addr);
        ret = client->sendmsg(&msg, 0);
        EXPECT_EQ(ret, 5);

        // recvmsg
        struct msghdr msg2;
        memset(&msg2, 0, sizeof(msg2));
        struct iovec iov2;
        char buf2[1024];
        iov2.iov_base = buf2;
        iov2.iov_len = 1024;
        msg2.msg_iov = &iov2;
        msg2.msg_iovlen = 1;
        ret = client->recvmsg(&msg2, 0);
        EXPECT_EQ(ret, 5);
        EXPECT_STREQ(buf2, "world");

        client->shutdown(SHUT_RDWR);
        client->close();
    });

    server_co.join();
    client_co.join();
}
