#include "ipp.hpp"

#include <vector>

#include <gtest/gtest.h>

TEST(IppTest, MsgAppNone) {
    ipp::MsgAppNone msg;
    ASSERT_EQ(msg.size(), 0);
}

TEST(IppTest, MsgAppStart) {
    ipp::MsgAppStart msg;
    ASSERT_EQ(msg.size(), 0);
}

TEST(IppTest, MsgAppStop) {
    ipp::MsgAppStop msg;
    ASSERT_EQ(msg.size(), 0);
}

TEST(IppTest, MsgAppWfData) {
    {
        const auto out = [&]() {
            std::vector<uint8_t> data{0, 1, 2, 3};
            return ipp::MsgAppWfData{std::move(data)};
        }();
        ASSERT_EQ(out.size(), 2 + 4);

        std::vector<uint8_t> buffer(out.size(), 0);
        out.store(buffer.data());
        ASSERT_EQ(4, buffer[0]);
        ASSERT_EQ(0, buffer[1]);
        for (size_t i = 2; i < buffer.size(); ++i) {
            ASSERT_EQ(static_cast<uint8_t>(i) - 2, buffer[i]);
        }
    }
    {
        std::vector<uint8_t> buffer{4, 0, 3, 2, 1, 0};
        const auto result = ipp::MsgAppWfData::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        const auto &data = in.data();
        ASSERT_EQ(data.size(), 4);
        for (size_t i = 0; i < data.size(); ++i) {
            ASSERT_EQ(static_cast<uint8_t>(data.size() - i - 1), data[i]);
        }
    }
}

TEST(IppTest, MsgMcuNone) {
    ipp::MsgMcuNone msg;
    ASSERT_EQ(msg.size(), 0);
}

TEST(IppTest, MsgMcuWfReq) {
    ipp::MsgMcuWfReq msg;
    ASSERT_EQ(msg.size(), 0);
}

TEST(IppTest, MsgMcuError) {
    {
        uint8_t code = 42;
        std::string text{"Error message"};
        const ipp::MsgMcuError out{code, std::string{text}};
        ASSERT_EQ(out.size(), 1 + 13 + 1);

        std::vector<uint8_t> buffer(out.size(), 0);
        out.store(buffer.data());
        ASSERT_EQ(code, buffer[0]);
        for (size_t i = 1; i < buffer.size() - 1; ++i) {
            ASSERT_EQ(text[i - 1], static_cast<char>(buffer[i]));
        }
        ASSERT_EQ(0, buffer[1 + text.size()]);
    }
    {
        std::vector<uint8_t> buffer{24, 'F', 'o', 'o', 0};
        const auto result = ipp::MsgMcuError::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        ASSERT_EQ(in.code(), 24);
        ASSERT_EQ(in.message(), std::string{"Foo"});
    }
}

TEST(IppTest, MsgMcuDebug) {
    {
        std::string text{"Debug message"};
        const ipp::MsgMcuDebug out{std::string{text}};
        ASSERT_EQ(out.size(), 13 + 1);

        std::vector<uint8_t> buffer(out.size(), 0);
        out.store(buffer.data());
        for (size_t i = 0; i < buffer.size() - 1; ++i) {
            ASSERT_EQ(text[i], static_cast<char>(buffer[i]));
        }
        ASSERT_EQ(0, buffer[1 + text.size()]);
    }
    {
        std::vector<uint8_t> buffer{'B', 'a', 'r', 0};
        const auto result = ipp::MsgMcuDebug::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        ASSERT_EQ(in.message(), std::string{"Bar"});
    }
}

TEST(IppTest, MsgAppAny) {
    {
        std::vector<uint8_t> data{0, 1, 2, 3};
        const ipp::MsgAppAny out{ipp::MsgAppWfData{std::move(data)}};
        ASSERT_EQ(out.size(), 7);
        std::vector<uint8_t> buffer(out.size(), 0);
        out.store(buffer.data());
        ASSERT_EQ(buffer[0], IppTypeApp::IPP_APP_WF_DATA);
        ASSERT_EQ(4, buffer[1]);
        ASSERT_EQ(0, buffer[2]);
        for (size_t i = 3; i < buffer.size(); ++i) {
            ASSERT_EQ(static_cast<uint8_t>(i) - 3, buffer[i]);
        }
    }
    {
        std::vector<uint8_t> buffer{IppTypeApp::IPP_APP_START};
        const auto result = ipp::MsgAppAny::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        ASSERT_TRUE(std::holds_alternative<ipp::MsgAppStart>(in.variant()));
    }
}

TEST(IppTest, MsgMcuAny) {
    {
        const ipp::MsgMcuAny out{ipp::MsgMcuWfReq{}};
        ASSERT_EQ(out.size(), 1);
        std::vector<uint8_t> buffer(out.size(), 0);
        out.store(buffer.data());
        ASSERT_EQ(buffer[0], IppTypeMcu::IPP_MCU_WF_REQ);
    }
    {
        std::vector<uint8_t> buffer{IppTypeMcu::IPP_MCU_DEBUG, 'F', 'o', 'o', 0};
        const auto result = ipp::MsgMcuAny::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        ASSERT_TRUE(std::holds_alternative<ipp::MsgMcuDebug>(in.variant()));
        ASSERT_EQ(std::get<ipp::MsgMcuDebug>(in.variant()).message(), std::string{"Foo"});
    }
}
