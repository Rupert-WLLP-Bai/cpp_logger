// log_test.cpp

#include "log.h"
#include <gtest/gtest.h>

TEST(LoggerTest, LogFunctionAddsMessageToBuffer) {
    Logger logger("test.log");
    std::string message = "Test message";

    logger.Log(message);

    boost::circular_buffer<std::string>& buffer = logger.GetBuffer();
    EXPECT_EQ(message, buffer[0]);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}