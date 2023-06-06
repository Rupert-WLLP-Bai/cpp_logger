// log_test.cpp

#include "log.h"
#include <gtest/gtest.h>

TEST(LoggerTest, LogFunctionAddsMessageToBuffer) {
    Logger logger("test_log.txt");
    std::string message = "Test message";

    logger.Log(message);

    const boost::circular_buffer<std::string>& buffer = logger.GetBuffer();
    ASSERT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.front(), message);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}