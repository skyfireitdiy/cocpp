_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/io/co_io.h"

CO_NAMESPACE_BEGIN

class co_file : public co_io
{
public:
    co_file() = default;

    /**
     * @brief 打开文件
     *
     * @param pathname 文件路径
     * @param flags 打开标志，会自动添加 O_NONBLOCK
     * @param mode 打开模式
     *
     * @return -1 失败，0 成功
     */
    int open(const char *pathname, int flags, mode_t mode = 0);
};

CO_NAMESPACE_END
