/* This includes the whole .c file to get access to static functions. */
#define PB_ENABLE_MALLOC
#include "pb_common.c"
#include "pb_decode.c"

#include <stdio.h>
#include <string.h>
#include "unittests.h"
#include "unittestproto.pb.h"

#define S(x) pb_istream_from_buffer((uint8_t*)x, sizeof(x) - 1)

bool stream_callback(pb_istream_t *stream, uint8_t *buf, size_t count)
{
    if (stream->state != NULL)
        return false; /* Simulate error */

    if (buf != NULL)
        memset(buf, 'x', count);
    return true;
}

/* Verifies that the stream passed to callback matches the byte array pointed to by arg. */
bool callback_check(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    int i;
    uint8_t byte;
    pb_bytes_array_t *ref = (pb_bytes_array_t*) *arg;

    for (i = 0; i < ref->size; i++)
    {
        if (!pb_read(stream, &byte, 1))
            return false;

        if (byte != ref->bytes[i])
            return false;
    }

    return true;
}

/* A callback that always fails - for testing error paths */
bool failing_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    PB_UNUSED(stream);
    PB_UNUSED(field);
    PB_UNUSED(arg);
    return false;
}

int main()
{
    int status = 0;

    {
        uint8_t buffer1[] = "foobartest1234";
        uint8_t buffer2[sizeof(buffer1)];
        pb_istream_t stream = pb_istream_from_buffer(buffer1, sizeof(buffer1));

        COMMENT("Test pb_read and pb_istream_t");
        TEST(pb_read(&stream, buffer2, 6))
        TEST(memcmp(buffer2, "foobar", 6) == 0)
        TEST(stream.bytes_left == sizeof(buffer1) - 6)
        TEST(pb_read(&stream, buffer2 + 6, stream.bytes_left))
        TEST(memcmp(buffer1, buffer2, sizeof(buffer1)) == 0)
        TEST(stream.bytes_left == 0)
        TEST(!pb_read(&stream, buffer2, 1))
    }

    {
        uint8_t buffer[20];
        pb_istream_t stream = {&stream_callback, NULL, 20};

        COMMENT("Test pb_read with custom callback");
        TEST(pb_read(&stream, buffer, 5))
        TEST(memcmp(buffer, "xxxxx", 5) == 0)
        TEST(!pb_read(&stream, buffer, 50))
        stream.state = (void*)1; /* Simulated error return from callback */
        TEST(!pb_read(&stream, buffer, 5))
        stream.state = NULL;
        TEST(pb_read(&stream, buffer, 15))
    }

    {
        pb_istream_t s;
        uint64_t u;
        int64_t i;

        COMMENT("Test pb_decode_varint");
        TEST((s = S("\x00"), pb_decode_varint(&s, &u) && u == 0));
        TEST((s = S("\x01"), pb_decode_varint(&s, &u) && u == 1));
        TEST((s = S("\xAC\x02"), pb_decode_varint(&s, &u) && u == 300));
        TEST((s = S("\xFF\xFF\xFF\xFF\x0F"), pb_decode_varint(&s, &u) && u == UINT32_MAX));
        TEST((s = S("\xFF\xFF\xFF\xFF\x0F"), pb_decode_varint(&s, (uint64_t*)&i) && i == UINT32_MAX));
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"),
              pb_decode_varint(&s, (uint64_t*)&i) && i == -1));
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"),
              pb_decode_varint(&s, &u) && u == UINT64_MAX));
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"),
              !pb_decode_varint(&s, &u)));
    }

    {
        pb_istream_t s;
        uint32_t u;

        COMMENT("Test pb_decode_varint32");
        TEST((s = S("\x00"), pb_decode_varint32(&s, &u) && u == 0));
        TEST((s = S("\x01"), pb_decode_varint32(&s, &u) && u == 1));
        TEST((s = S("\xAC\x02"), pb_decode_varint32(&s, &u) && u == 300));
        TEST((s = S("\xFF\xFF\xFF\xFF\x0F"), pb_decode_varint32(&s, &u) && u == UINT32_MAX));
        TEST((s = S("\xFF\xFF\xFF\xFF\x8F\x00"), pb_decode_varint32(&s, &u) && u == UINT32_MAX));
        TEST((s = S("\xFF\xFF\xFF\xFF\x10"), !pb_decode_varint32(&s, &u)));
        TEST((s = S("\xFF\xFF\xFF\xFF\x40"), !pb_decode_varint32(&s, &u)));
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\x01"), !pb_decode_varint32(&s, &u)));
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x80\x00"), !pb_decode_varint32(&s, &u)));
    }

    {
        pb_istream_t s;
        COMMENT("Test pb_skip_varint");
        TEST((s = S("\x00""foobar"), pb_skip_varint(&s) && s.bytes_left == 6))
        TEST((s = S("\xAC\x02""foobar"), pb_skip_varint(&s) && s.bytes_left == 6))
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01""foobar"),
              pb_skip_varint(&s) && s.bytes_left == 6))
        TEST((s = S("\xFF"), !pb_skip_varint(&s)))
    }

    {
        pb_istream_t s;
        COMMENT("Test pb_skip_string")
        TEST((s = S("\x00""foobar"), pb_skip_string(&s) && s.bytes_left == 6))
        TEST((s = S("\x04""testfoobar"), pb_skip_string(&s) && s.bytes_left == 6))
        TEST((s = S("\x04"), !pb_skip_string(&s)))
        TEST((s = S("\xFF"), !pb_skip_string(&s)))
    }

    {
        pb_istream_t s = S("\x01\x00");
        pb_field_iter_t f;
        uint32_t d;

        f.type = PB_LTYPE_VARINT;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_varint using uint32_t")
        TEST(pb_dec_varint(&s, &f) && d == 1)

        /* Verify that no more than data_size is written. */
        d = 0xFFFFFFFF;
        f.data_size = 1;
        TEST(pb_dec_varint(&s, &f) && (d == 0xFFFFFF00 || d == 0x00FFFFFF))
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        int32_t d;

        f.type = PB_LTYPE_SVARINT;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_varint using sint32_t")
        TEST((s = S("\x01"), pb_dec_varint(&s, &f) && d == -1))
        TEST((s = S("\x02"), pb_dec_varint(&s, &f) && d == 1))
        TEST((s = S("\xfe\xff\xff\xff\x0f"), pb_dec_varint(&s, &f) && d == INT32_MAX))
        TEST((s = S("\xff\xff\xff\xff\x0f"), pb_dec_varint(&s, &f) && d == INT32_MIN))
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        int64_t d;

        f.type = PB_LTYPE_SVARINT;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_varint using sint64_t")
        TEST((s = S("\x01"), pb_dec_varint(&s, &f) && d == -1))
        TEST((s = S("\x02"), pb_dec_varint(&s, &f) && d == 1))
        TEST((s = S("\xFE\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"), pb_dec_varint(&s, &f) && d == INT64_MAX))
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"), pb_dec_varint(&s, &f) && d == INT64_MIN))
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        int32_t d;

        f.type = PB_LTYPE_SVARINT;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_varint overflow detection using sint32_t");
        TEST((s = S("\xfe\xff\xff\xff\x0f"), pb_dec_varint(&s, &f)));
        TEST((s = S("\xfe\xff\xff\xff\x10"), !pb_dec_varint(&s, &f)));
        TEST((s = S("\xff\xff\xff\xff\x0f"), pb_dec_varint(&s, &f)));
        TEST((s = S("\xff\xff\xff\xff\x10"), !pb_dec_varint(&s, &f)));
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        uint32_t d;

        f.type = PB_LTYPE_UVARINT;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_varint using uint32_t")
        TEST((s = S("\x01"), pb_dec_varint(&s, &f) && d == 1))
        TEST((s = S("\x02"), pb_dec_varint(&s, &f) && d == 2))
        TEST((s = S("\xff\xff\xff\xff\x0f"), pb_dec_varint(&s, &f) && d == UINT32_MAX))
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        uint64_t d;

        f.type = PB_LTYPE_UVARINT;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_varint using uint64_t")
        TEST((s = S("\x01"), pb_dec_varint(&s, &f) && d == 1))
        TEST((s = S("\x02"), pb_dec_varint(&s, &f) && d == 2))
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"), pb_dec_varint(&s, &f) && d == UINT64_MAX))
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        uint32_t d;

        f.type = PB_LTYPE_UVARINT;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_varint overflow detection using uint32_t");
        TEST((s = S("\xff\xff\xff\xff\x0f"), pb_dec_varint(&s, &f)));
        TEST((s = S("\xff\xff\xff\xff\x10"), !pb_dec_varint(&s, &f)));
    }

    {
        pb_istream_t s;
        float d;

        COMMENT("Test pb_dec_fixed using float (failures here may be caused by imperfect rounding)")
        TEST((s = S("\x00\x00\x00\x00"), pb_decode_fixed32(&s, &d) && d == 0.0f))
        TEST((s = S("\x00\x00\xc6\x42"), pb_decode_fixed32(&s, &d) && d == 99.0f))
        TEST((s = S("\x4e\x61\x3c\xcb"), pb_decode_fixed32(&s, &d) && d == -12345678.0f))
        d = -12345678.0f;
        TEST((s = S("\x00"), !pb_decode_fixed32(&s, &d) && d == -12345678.0f))
    }

    if (sizeof(double) == 8)
    {
        pb_istream_t s;
        double d;

        COMMENT("Test pb_dec_fixed64 using double (failures here may be caused by imperfect rounding)")
        TEST((s = S("\x00\x00\x00\x00\x00\x00\x00\x00"), pb_decode_fixed64(&s, &d) && d == 0.0))
        TEST((s = S("\x00\x00\x00\x00\x00\xc0\x58\x40"), pb_decode_fixed64(&s, &d) && d == 99.0))
        TEST((s = S("\x00\x00\x00\xc0\x29\x8c\x67\xc1"), pb_decode_fixed64(&s, &d) && d == -12345678.0f))
    }

    {
        pb_istream_t s;
        struct { pb_size_t size; uint8_t bytes[5]; } d;
        pb_field_iter_t f;

        f.type = PB_LTYPE_BYTES;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_bytes")
        TEST((s = S("\x00"), pb_dec_bytes(&s, &f) && d.size == 0))
        TEST((s = S("\x01\xFF"), pb_dec_bytes(&s, &f) && d.size == 1 && d.bytes[0] == 0xFF))
        TEST((s = S("\x05xxxxx"), pb_dec_bytes(&s, &f) && d.size == 5))
        TEST((s = S("\x05xxxx"), !pb_dec_bytes(&s, &f)))

        /* Note: the size limit on bytes-fields is not strictly obeyed, as
         * the compiler may add some padding to the struct. Using this padding
         * is not a very good thing to do, but it is difficult to avoid when
         * we use only a single uint8_t to store the size of the field.
         * Therefore this tests against a 10-byte string, while otherwise even
         * 6 bytes should error out.
         */
        TEST((s = S("\x10xxxxxxxxxx"), !pb_dec_bytes(&s, &f)))
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        char d[5];

        f.type = PB_LTYPE_STRING;
        f.data_size = sizeof(d);
        f.pData = &d;

        COMMENT("Test pb_dec_string")
        TEST((s = S("\x00"), pb_dec_string(&s, &f) && d[0] == '\0'))
        TEST((s = S("\x04xyzz"), pb_dec_string(&s, &f) && strcmp(d, "xyzz") == 0))
        TEST((s = S("\x05xyzzy"), !pb_dec_string(&s, &f)))
    }

    {
        pb_istream_t s;
        IntegerArray dest;

        COMMENT("Testing pb_decode with repeated int32 field")
        TEST((s = S(""), pb_decode(&s, IntegerArray_fields, &dest) && dest.data_count == 0))
        TEST((s = S("\x08\x01\x08\x02"), pb_decode(&s, IntegerArray_fields, &dest)
             && dest.data_count == 2 && dest.data[0] == 1 && dest.data[1] == 2))
        s = S("\x08\x01\x08\x02\x08\x03\x08\x04\x08\x05\x08\x06\x08\x07\x08\x08\x08\x09\x08\x0A");
        TEST(pb_decode(&s, IntegerArray_fields, &dest) && dest.data_count == 10 && dest.data[9] == 10)
        s = S("\x08\x01\x08\x02\x08\x03\x08\x04\x08\x05\x08\x06\x08\x07\x08\x08\x08\x09\x08\x0A\x08\x0B");
        TEST(!pb_decode(&s, IntegerArray_fields, &dest))
    }

    {
        pb_istream_t s;
        IntegerArray dest;

        COMMENT("Testing pb_decode with packed int32 field")
        TEST((s = S("\x0A\x00"), pb_decode(&s, IntegerArray_fields, &dest)
            && dest.data_count == 0))
        TEST((s = S("\x0A\x01\x01"), pb_decode(&s, IntegerArray_fields, &dest)
            && dest.data_count == 1 && dest.data[0] == 1))
        TEST((s = S("\x0A\x0A\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A"), pb_decode(&s, IntegerArray_fields, &dest)
            && dest.data_count == 10 && dest.data[0] == 1 && dest.data[9] == 10))
        TEST((s = S("\x0A\x0B\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"), !pb_decode(&s, IntegerArray_fields, &dest)))

        /* Test invalid wire data */
        TEST((s = S("\x0A\xFF"), !pb_decode(&s, IntegerArray_fields, &dest)))
        TEST((s = S("\x0A\x01"), !pb_decode(&s, IntegerArray_fields, &dest)))
    }

    {
        pb_istream_t s;
        IntegerArray dest;

        COMMENT("Testing pb_decode with unknown fields")
        TEST((s = S("\x18\x0F\x08\x01"), pb_decode(&s, IntegerArray_fields, &dest)
            && dest.data_count == 1 && dest.data[0] == 1))
        TEST((s = S("\x19\x00\x00\x00\x00\x00\x00\x00\x00\x08\x01"), pb_decode(&s, IntegerArray_fields, &dest)
            && dest.data_count == 1 && dest.data[0] == 1))
        TEST((s = S("\x1A\x00\x08\x01"), pb_decode(&s, IntegerArray_fields, &dest)
            && dest.data_count == 1 && dest.data[0] == 1))
        TEST((s = S("\x1B\x08\x01"), !pb_decode(&s, IntegerArray_fields, &dest)))
        TEST((s = S("\x1D\x00\x00\x00\x00\x08\x01"), pb_decode(&s, IntegerArray_fields, &dest)
            && dest.data_count == 1 && dest.data[0] == 1))
    }

    {
        pb_istream_t s;
        CallbackArray dest;
        struct { pb_size_t size; uint8_t bytes[10]; } ref;
        dest.data.funcs.decode = &callback_check;
        dest.data.arg = &ref;

        COMMENT("Testing pb_decode with callbacks")
        /* Single varint */
        ref.size = 1; ref.bytes[0] = 0x55;
        TEST((s = S("\x08\x55"), pb_decode(&s, CallbackArray_fields, &dest)))
        /* Packed varint */
        ref.size = 3; ref.bytes[0] = ref.bytes[1] = ref.bytes[2] = 0x55;
        TEST((s = S("\x0A\x03\x55\x55\x55"), pb_decode(&s, CallbackArray_fields, &dest)))
        /* Packed varint with loop */
        ref.size = 1; ref.bytes[0] = 0x55;
        TEST((s = S("\x0A\x03\x55\x55\x55"), pb_decode(&s, CallbackArray_fields, &dest)))
        /* Single fixed32 */
        ref.size = 4; ref.bytes[0] = ref.bytes[1] = ref.bytes[2] = ref.bytes[3] = 0xAA;
        TEST((s = S("\x0D\xAA\xAA\xAA\xAA"), pb_decode(&s, CallbackArray_fields, &dest)))
        /* Single fixed64 */
        ref.size = 8; memset(ref.bytes, 0xAA, 8);
        TEST((s = S("\x09\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"), pb_decode(&s, CallbackArray_fields, &dest)))
        /* Unsupported field type */
        TEST((s = S("\x0B\x00"), !pb_decode(&s, CallbackArray_fields, &dest)))

        /* Just make sure that our test function works */
        ref.size = 1; ref.bytes[0] = 0x56;
        TEST((s = S("\x08\x55"), !pb_decode(&s, CallbackArray_fields, &dest)))
    }

    {
        pb_istream_t s;
        IntegerArray dest;

        COMMENT("Testing pb_decode message termination")
        TEST((s = S(""), pb_decode(&s, IntegerArray_fields, &dest)))
        TEST((s = S("\x08\x01"), pb_decode(&s, IntegerArray_fields, &dest)))
        TEST((s = S("\x08"), !pb_decode(&s, IntegerArray_fields, &dest)))
    }

    {
        pb_istream_t s;
        IntegerArray dest;

        COMMENT("Testing pb_decode_ex null termination")

        TEST((s = S("\x00"), pb_decode_ex(&s, IntegerArray_fields, &dest, PB_DECODE_NULLTERMINATED)))
        TEST((s = S("\x08\x01\x00"), pb_decode_ex(&s, IntegerArray_fields, &dest, PB_DECODE_NULLTERMINATED)))
    }

    {
        pb_istream_t s;
        IntegerArray dest;

        COMMENT("Testing pb_decode with invalid tag numbers")
        TEST((s = S("\x9f\xea"), !pb_decode(&s, IntegerArray_fields, &dest)));
        TEST((s = S("\x00"), !pb_decode(&s, IntegerArray_fields, &dest)));
    }

    {
        pb_istream_t s;
        IntegerContainer dest = {{0}};

        COMMENT("Testing pb_decode_delimited")
        TEST((s = S("\x09\x0A\x07\x0A\x05\x01\x02\x03\x04\x05"),
              pb_decode_delimited(&s, IntegerContainer_fields, &dest)) &&
              dest.submsg.data_count == 5)
    }

    {
        pb_istream_t s = {0};
        void *data = NULL;

        COMMENT("Testing allocate_field")
        TEST(allocate_field(&s, &data, 10, 10) && data != NULL);
        TEST(allocate_field(&s, &data, 10, 20) && data != NULL);

        {
            void *oldvalue = data;
            size_t very_big = (size_t)-1;
            size_t somewhat_big = very_big / 2 + 1;
            size_t not_so_big = (size_t)1 << (4 * sizeof(size_t));

            TEST(!allocate_field(&s, &data, very_big, 2) && data == oldvalue);
            TEST(!allocate_field(&s, &data, somewhat_big, 2) && data == oldvalue);
            TEST(!allocate_field(&s, &data, not_so_big, not_so_big) && data == oldvalue);
        }

        pb_free(data);
    }

    {
        pb_istream_t s = {0};
        void *data = NULL;

        COMMENT("Testing allocate_field with zero size")
        /* Test with data_size = 0 should fail */
        TEST(!allocate_field(&s, &data, 0, 10));
        /* Test with array_size = 0 should fail */
        TEST(!allocate_field(&s, &data, 10, 0));
    }

    {
        pb_istream_t s;
        uint32_t dest;

        COMMENT("Testing pb_decode_varint32 sign extension edge cases")
        /* Test varint32 with negative number sign extension (valid) */
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"), pb_decode_varint32(&s, &dest) && dest == UINT32_MAX));
        /* Test varint32 that's too long (invalid) */
        TEST((s = S("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"), !pb_decode_varint32(&s, &dest)));
    }

    {
        pb_istream_t s;

        COMMENT("Testing pb_skip_string with edge cases")
        /* Test skip_string with length that causes size_t overflow */
        TEST((s = S("\xFF\xFF\xFF\xFF\x0F"), !pb_skip_string(&s)));
    }

    {
        pb_istream_t s;

        COMMENT("Testing pb_make_string_substream edge cases")
        pb_istream_t substream;

        /* Test with parent stream too short */
        s = S("\x10"); /* Length prefix says 16 bytes but stream only has 0 bytes left */
        TEST(!pb_make_string_substream(&s, &substream));
    }

    {
        COMMENT("Testing pb_close_string_substream")
        pb_istream_t s;
        pb_istream_t substream;

        /* Create a stream with some data */
        s = S("\x05hello");
        TEST(pb_make_string_substream(&s, &substream));
        /* Read some but not all data from substream */
        uint8_t buf[3];
        TEST(pb_read(&substream, buf, 2));
        /* Close should skip remaining bytes */
        TEST(pb_close_string_substream(&s, &substream));
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        uint8_t d8;

        COMMENT("Testing pb_dec_varint with uint8_t")
        f.type = PB_LTYPE_UVARINT;
        f.data_size = sizeof(d8);
        f.pData = &d8;

        /* Test normal value */
        TEST((s = S("\x7F"), pb_dec_varint(&s, &f) && d8 == 127));
        /* Test value that would overflow uint8 (256) */
        TEST((s = S("\x80\x02"), !pb_dec_varint(&s, &f)));
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        uint16_t d16;

        COMMENT("Testing pb_dec_varint with uint16_t")
        f.type = PB_LTYPE_UVARINT;
        f.data_size = sizeof(d16);
        f.pData = &d16;

        /* Test normal value */
        TEST((s = S("\xFF\x7F"), pb_dec_varint(&s, &f) && d16 == 16383));
        /* Test value that would overflow uint16 (65536) */
        TEST((s = S("\x80\x80\x04"), !pb_dec_varint(&s, &f)));
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        int8_t d8;

        COMMENT("Testing pb_dec_varint with int8_t")
        f.type = PB_LTYPE_SVARINT;
        f.data_size = sizeof(d8);
        f.pData = &d8;

        /* Test normal value */
        TEST((s = S("\x02"), pb_dec_varint(&s, &f) && d8 == 1));
        /* Test value that would overflow int8 (128 as svarint = 256 wire) */
        TEST((s = S("\x80\x02"), !pb_dec_varint(&s, &f)));
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        int16_t d16;

        COMMENT("Testing pb_dec_varint with int16_t")
        f.type = PB_LTYPE_SVARINT;
        f.data_size = sizeof(d16);
        f.pData = &d16;

        /* Test normal value */
        TEST((s = S("\x02"), pb_dec_varint(&s, &f) && d16 == 1));
        /* Test value that would overflow int16 (32768 as svarint = 65536 wire) */
        TEST((s = S("\x80\x80\x04"), !pb_dec_varint(&s, &f)));
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        uint8_t d[10];

        COMMENT("Testing pb_dec_fixed_length_bytes with size 0")
        f.type = PB_LTYPE_FIXED_LENGTH_BYTES;
        f.data_size = sizeof(d);
        f.pData = d;
        memset(d, 0xAA, sizeof(d));

        /* Empty fixed length bytes should zero-fill destination */
        TEST((s = S("\x00"), pb_dec_fixed_length_bytes(&s, &f)));
        TEST(d[0] == 0 && d[9] == 0);
    }

    {
        pb_istream_t s;
        pb_field_iter_t f;
        uint8_t d[5];

        COMMENT("Testing pb_dec_fixed_length_bytes with mismatched size")
        f.type = PB_LTYPE_FIXED_LENGTH_BYTES;
        f.data_size = sizeof(d);
        f.pData = d;

        /* Wrong size should fail */
        TEST((s = S("\x03""abc"), !pb_dec_fixed_length_bytes(&s, &f)));
    }

    {
        pb_istream_t s;

        COMMENT("Testing pb_read skip mode with callback stream")
        /* Create callback stream for testing skip */
        s.callback = &stream_callback;
        s.state = NULL;
        s.bytes_left = 100;
        #ifndef PB_NO_ERRMSG
        s.errmsg = NULL;
        #endif

        /* Skip 32 bytes (tests the skip loop) */
        TEST(pb_read(&s, NULL, 32));
        TEST(s.bytes_left == 68);
    }

    {
        pb_istream_t s;
        bool d;

        COMMENT("Testing pb_decode_bool")
        /* Test decoding 0 as false */
        TEST((s = S("\x00"), pb_decode_bool(&s, &d) && d == false));
        /* Test decoding 1 as true */
        TEST((s = S("\x01"), pb_decode_bool(&s, &d) && d == true));
        /* Test decoding larger value as true */
        TEST((s = S("\xFF\x01"), pb_decode_bool(&s, &d) && d == true));
    }

    {
        pb_istream_t s;
        int64_t d;

        COMMENT("Testing pb_decode_svarint")
        /* Test decoding positive value */
        TEST((s = S("\x04"), pb_decode_svarint(&s, &d) && d == 2));
        /* Test decoding negative value */
        TEST((s = S("\x03"), pb_decode_svarint(&s, &d) && d == -2));
        /* Test decoding zero */
        TEST((s = S("\x00"), pb_decode_svarint(&s, &d) && d == 0));
    }

    {
        pb_istream_t s;
        CallbackArray dest;

        COMMENT("Testing pb_decode with failing callback")
        dest.data.funcs.decode = &failing_callback;
        dest.data.arg = NULL;

        /* The failing callback should cause pb_decode to fail */
        TEST((s = S("\x08\x55"), !pb_decode(&s, CallbackArray_fields, &dest)));
    }

    {
        pb_istream_t s;

        COMMENT("Testing pb_decode_fixed64 error case")
        double d = 0.0;
        /* Too short stream should fail */
        TEST((s = S("\x00\x00\x00"), !pb_decode_fixed64(&s, &d)));
    }

    if (status != 0)
        fprintf(stdout, "\n\nSome tests FAILED!\n");

    return status;
}
