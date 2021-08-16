#ifdef UNITTEST

#include "ipp.hpp"

#include <vector>

#include <gtest/gtest.h>

TEST(IppTest, MsgAppNone) {
    ipp::MsgAppNone msg;
    ASSERT_EQ(msg.length(), 0);
}

TEST(IppTest, MsgAppStart) {
    ipp::MsgAppStart msg;
    ASSERT_EQ(msg.length(), 0);
}

TEST(IppTest, MsgAppStop) {
    ipp::MsgAppStop msg;
    ASSERT_EQ(msg.length(), 0);
}

TEST(IppTest, MsgAppDacSet) {
    {
        const uint32_t value = (1 << 16) | (2 << 8) | 3;
        const ipp::MsgAppDacSet out{value};
        ASSERT_EQ(out.value(), value);
        ASSERT_EQ(out.length(), 3);

        std::vector<uint8_t> buffer(out.length(), 0);
        out.store(buffer.data());
        ASSERT_EQ(3, buffer[0]);
        ASSERT_EQ(2, buffer[1]);
        ASSERT_EQ(1, buffer[2]);
    }
    {
        std::vector<uint8_t> buffer{1, 2, 3};
        const auto result = ipp::MsgAppDacSet::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        ASSERT_EQ(in.value(), (3 << 16) | (2 << 8) | 1);
    }
}

TEST(IppTest, MsgAppAdcReq) {
    {
        const uint8_t index = 42;
        const ipp::MsgAppAdcReq out{index};
        ASSERT_EQ(out.index(), index);
        ASSERT_EQ(out.length(), 1);

        std::vector<uint8_t> buffer(out.length(), 0);
        out.store(buffer.data());
        ASSERT_EQ(index, buffer[0]);
    }
    {
        const uint8_t index = 24;
        std::vector<uint8_t> buffer{index};
        const auto result = ipp::MsgAppAdcReq::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        ASSERT_EQ(in.index(), index);
    }
}

TEST(IppTest, MsgMcuNone) {
    ipp::MsgMcuNone msg;
    ASSERT_EQ(msg.length(), 0);
}

TEST(IppTest, MsgMcuAdcVal) {
    {
        uint8_t index = 42;
        uint32_t value = (1 << 16) | (2 << 8) | 3;
        const ipp::MsgMcuAdcVal out{index, value};
        ASSERT_EQ(out.length(), 1 + 3);

        std::vector<uint8_t> buffer(out.length(), 0);
        out.store(buffer.data());
        ASSERT_EQ(buffer[0], index);
        ASSERT_EQ(buffer[1], 3);
        ASSERT_EQ(buffer[2], 2);
        ASSERT_EQ(buffer[3], 1);
    }
    {
        std::vector<uint8_t> buffer{24, 1, 2, 3};
        const auto result = ipp::MsgMcuAdcVal::load(buffer.data(), buffer.size());
        ASSERT_EQ(result.index(), 0);
        const auto &in = std::get<0>(result);
        ASSERT_EQ(in.index(), 24);
        ASSERT_EQ(in.value(), (3 << 16) | (2 << 8) | 1);
    }
}

TEST(IppTest, MsgMcuError) {
    {
        uint8_t code = 42;
        std::string text{"Error message"};
        const ipp::MsgMcuError out{code, std::string{text}};
        ASSERT_EQ(out.length(), 1 + 13 + 1);

        std::vector<uint8_t> buffer(out.length(), 0);
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
        ASSERT_EQ(out.length(), 13 + 1);

        std::vector<uint8_t> buffer(out.length(), 0);
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
        const ipp::MsgAppAny out{ipp::MsgAppDacSet{(1 << 16) | (2 << 8) | 3}};
        ASSERT_EQ(out.length(), 1 + 3);
        std::vector<uint8_t> buffer(out.length(), 0);
        out.store(buffer.data());
        ASSERT_EQ(buffer[0], IppTypeApp::IPP_APP_DAC_SET);
        ASSERT_EQ(buffer[1], 3);
        ASSERT_EQ(buffer[2], 2);
        ASSERT_EQ(buffer[3], 1);
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
        const ipp::MsgMcuAny out{ipp::MsgMcuAdcVal{42, (1 << 16) | (2 << 8) | 3}};
        ASSERT_EQ(out.length(), 1 + 1 + 3);
        std::vector<uint8_t> buffer(out.length(), 0);
        out.store(buffer.data());
        ASSERT_EQ(buffer[0], IppTypeMcu::IPP_MCU_ADC_VAL);
        ASSERT_EQ(buffer[1], 42);
        ASSERT_EQ(buffer[2], 3);
        ASSERT_EQ(buffer[3], 2);
        ASSERT_EQ(buffer[4], 1);
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

#endif // UNITTEST
