#include <clickhouse/base/wire_format.h>
#include <clickhouse/base/output.h>
#include <clickhouse/base/input.h>
#include <clickhouse/base/compressed.h>

#include <gtest/gtest.h>

using namespace clickhouse;

namespace {

Buffer MakeCompressedBlockHeader(uint32_t compressed_size) {
    Buffer buffer;
    BufferOutput output(&buffer);

    uint8_t hash[16] = {};
    const uint8_t method = 0x82;
    const uint32_t original_size = 0;

    WireFormat::WriteFixed(output, hash);
    WireFormat::WriteFixed(output, method);
    WireFormat::WriteFixed(output, compressed_size);
    WireFormat::WriteFixed(output, original_size);
    output.Flush();

    return buffer;
}

}

TEST(CodedStreamCase, Varint64) {
    Buffer buf;

    {
        BufferOutput output(&buf);
        WireFormat::WriteVarint64(output, 18446744071965638648ULL);
        output.Flush();
    }

    {
        ArrayInput input(buf.data(), buf.size());
        uint64_t value = 0;
        ASSERT_TRUE(WireFormat::ReadVarint64(input, &value));
        ASSERT_EQ(value, 18446744071965638648ULL);
    }
}

TEST(CompressedInputCase, RejectsBlocksShorterThanHeader) {
    for (const uint32_t compressed_size : {0U, 1U, 8U}) {
        Buffer buffer = MakeCompressedBlockHeader(compressed_size);
        ArrayInput input(buffer.data(), buffer.size());
        CompressedInput compressed_input(&input);
        const void* data = nullptr;

        EXPECT_THROW(compressed_input.Next(&data, 1), CompressionError)
            << "compressed size: " << compressed_size;
    }
}
