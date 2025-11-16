#include "SslConfig.h"

SslConfig::SslConfig()
    : version_(SSLVersion::TLS_1_2)
    //HIGH: 使用高强度加密算法
    //!aNULL: 禁用匿名DH算法（不安全）
    //!MDS: 可能是拼写错误，应为 !MD5（禁用MD5哈希）
    , cipherList_("HIGH:!aNULL:!MD5")
    , verifyClient_(false)
    , verifyDepth_(4)
    , sessionTimeout_(300)
    , sessionCacheSize_(20480L)
{
}
