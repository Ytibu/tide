#include "../tide/log.h"
#include "../tide/utils.h"
#include <assert.h>
#include "../tide/macro.h"

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void test_assert()
{
    TIDE_LOG_INFO(g_logger) << tide::BacktraceToString(10);
    TIDE_ASSERT2(0==1, "acbekl");
}

int main()
{
    test_assert();
    return 0;
}