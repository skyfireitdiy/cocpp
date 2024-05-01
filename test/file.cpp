#include <gtest/gtest.h>
#include "cocpp/io/co_file.h"

using namespace cocpp;

bool file_exists(const char *filename)
{
    struct stat st;
    return stat(filename, &st) == 0;
}

int get_file_mode(const char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_mode;
}

int get_file_group(const char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_gid;
}

int get_file_user(const char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_uid;
}

TEST(file, open_null)
{
    co_file f;
    int ret = f.open("/dev/null", O_RDONLY, 0644);
    EXPECT_EQ(ret, 0);
    f.close();
}

TEST(file, open_disk)
{
    co_file f;
    int ret = f.open("test_open_disk_file", O_RDWR | O_CREAT, 0644);
    EXPECT_EQ(ret, 0);
    f.close();
    // 判断文件是否存在
    EXPECT_TRUE(file_exists("test_open_disk_file"));
    remove("test_open_disk_file");
}

TEST(file, open_invalid)
{
    co_file f;
    int ret = f.open("invalid_file", O_RDONLY, 0644);
    EXPECT_EQ(ret, -1);
    EXPECT_FALSE(file_exists("invalid_file"));
}

TEST(file, read_write)
{
    co_file f;
    int ret = f.open("test_write_file", O_RDWR | O_CREAT, 0644);
    EXPECT_EQ(ret, 0);
    ret = f.write("hello", 5);
    EXPECT_EQ(ret, 5);
    f.close();

    EXPECT_TRUE(file_exists("test_write_file"));

    co_file f2;
    ret = f2.open("test_write_file", O_RDONLY, 0644);
    EXPECT_EQ(ret, 0);
    char buf[10] = {0};
    ret = f2.read(buf, 10);
    EXPECT_EQ(ret, 5);
    EXPECT_STREQ(buf, "hello");
    f2.close();

    remove("test_write_file");
}

TEST(file, lseek)
{
    co_file f;
    int ret = f.open("test_lseek_file", O_RDWR | O_CREAT, 0644);
    EXPECT_EQ(ret, 0);
    ret = f.write("hello", 5);
    EXPECT_EQ(ret, 5);
    f.close();

    EXPECT_TRUE(file_exists("test_lseek_file"));

    co_file f2;
    ret = f2.open("test_lseek_file", O_RDONLY, 0644);
    EXPECT_EQ(ret, 0);
    ret = f2.lseek(2, SEEK_SET);
    EXPECT_EQ(ret, 2);
    char buf[10] = {0};
    ret = f2.read(buf, 10);
    EXPECT_EQ(ret, 3);
    EXPECT_STREQ(buf, "llo");
    f2.close();
    remove("test_lseek_file");
}

TEST(file, fchmod)
{
    co_file f;
    int ret = f.open("test_chmod_file", O_RDWR | O_CREAT, 0644);
    EXPECT_EQ(ret, 0);

    ret = f.fchmod(0755);
    EXPECT_EQ(ret, 0);
    f.close();

    int mode = get_file_mode("test_chmod_file");
    EXPECT_EQ(mode & ~S_IFMT, 0755);

    remove("test_chmod_file");
}

TEST(file, fchown)
{
    co_file f;
    int ret = f.open("test_chown_file", O_RDWR | O_CREAT, 0644);
    EXPECT_EQ(ret, 0);

    ret = f.fchown(1000, 1000);
    EXPECT_EQ(ret, 0);
    f.close();

    EXPECT_EQ(get_file_user("test_chown_file"), 1000);
    EXPECT_EQ(get_file_group("test_chown_file"), 1000);
}
