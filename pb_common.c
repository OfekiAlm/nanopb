/* pb_common.c: Common support functions for pb_encode.c and pb_decode.c.
 *
 * 2014 Petteri Aimonen <jpa@kapsi.fi>
 */

#include "pb_common.h"

/* Field descriptor format explanation:
 * 
 * Nanopb uses a compact binary format to encode field descriptors. The format
 * is optimized for code size by using variable-length encoding based on field
 * complexity. There are four format variants: 1-word, 2-word, 4-word, and 8-word.
 * 
 * Format selection (determined by bits 0-1 of word0):
 *   0 = 1-word: Simple fields (tag < 64, small offsets/sizes)
 *   1 = 2-word: Medium fields (tag < 16384, medium offsets/sizes)
 *   2 = 4-word: Complex fields (large tags/offsets, no array)
 *   3 = 8-word: Maximum complexity (large tags/offsets/arrays)
 * 
 * All formats encode these properties:
 *   - Field type (wire type + allocation type + handling type)
 *   - Tag number (protobuf field number)
 *   - Data offset (byte offset from message start to field data)
 *   - Data size (size of field in bytes)
 *   - Size offset (byte offset from field to its size counter, if any)
 *   - Array size (element count for repeated fields)
 * 
 * The bit layouts are optimized to fit common values in fewer words.
 * See comments in each case for specific bit positions.
 */
static bool load_descriptor_values(pb_field_iter_t *iter)
{
    uint32_t word0;
    uint32_t data_offset;
    int_least8_t size_offset;

    if (iter->index >= iter->descriptor->field_count)
        return false;

    word0 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index]);
    iter->type = (pb_type_t)((word0 >> 8) & 0xFF);

    switch(word0 & 3)
    {
        case 0: {
            /* 1-word format: For simple fields with small tags and offsets.
             * 
             * Bit layout of word0:
             *   [1:0]   = format (00 = 1-word)
             *   [7:2]   = tag number (0-63)
             *   [15:8]  = field type
             *   [23:16] = data offset (0-255 bytes)
             *   [27:24] = size offset (0-15 bytes, signed relative to field)
             *   [31:28] = data size (0-15 bytes)
             * 
             * Array size is always 1 (non-array field).
             */
            iter->array_size = 1;
            iter->tag = (pb_size_t)((word0 >> 2) & 0x3F);
            size_offset = (int_least8_t)((word0 >> 24) & 0x0F);
            data_offset = (word0 >> 16) & 0xFF;
            iter->data_size = (pb_size_t)((word0 >> 28) & 0x0F);
            break;
        }

        case 1: {
            /* 2-word format: For fields with medium-sized tags and arrays.
             * 
             * Bit layout:
             * word0:
             *   [1:0]   = format (01 = 2-word)
             *   [7:2]   = tag number low 6 bits
             *   [15:8]  = field type
             *   [27:16] = array size (0-4095 elements)
             *   [31:28] = size offset (0-15 bytes)
             * 
             * word1:
             *   [15:0]  = data offset (0-65535 bytes)
             *   [27:16] = data size (0-4095 bytes)
             *   [31:28] = tag number high 4 bits (allows tag 0-1023)
             */
            uint32_t word1 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 1]);

            iter->array_size = (pb_size_t)((word0 >> 16) & 0x0FFF);
            iter->tag = (pb_size_t)(((word0 >> 2) & 0x3F) | ((word1 >> 28) << 6));
            size_offset = (int_least8_t)((word0 >> 28) & 0x0F);
            data_offset = word1 & 0xFFFF;
            iter->data_size = (pb_size_t)((word1 >> 16) & 0x0FFF);
            break;
        }

        case 2: {
            /* 4-word format: For fields with large tags or offsets, no large arrays.
             * 
             * Bit layout:
             * word0:
             *   [1:0]   = format (10 = 4-word)
             *   [7:2]   = tag number low 6 bits
             *   [15:8]  = field type
             *   [31:16] = array size (0-65535 elements)
             * 
             * word1:
             *   [7:0]   = size offset (0-127 bytes, signed)
             *   [31:8]  = tag number high 24 bits (allows very large tags)
             * 
             * word2:
             *   [31:0]  = data offset (full 32-bit offset)
             * 
             * word3:
             *   [31:0]  = data size (full 32-bit size)
             */
            uint32_t word1 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 1]);
            uint32_t word2 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 2]);
            uint32_t word3 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 3]);

            iter->array_size = (pb_size_t)(word0 >> 16);
            iter->tag = (pb_size_t)(((word0 >> 2) & 0x3F) | ((word1 >> 8) << 6));
            size_offset = (int_least8_t)(word1 & 0xFF);
            data_offset = word2;
            iter->data_size = (pb_size_t)word3;
            break;
        }

        default: {
            /* 8-word format: Maximum complexity (large arrays + large tags/offsets).
             * 
             * Bit layout:
             * word0:
             *   [1:0]   = format (11 = 8-word)
             *   [7:2]   = tag number low 6 bits
             *   [15:8]  = field type
             *   [31:16] = (reserved, unused in current implementation)
             * 
             * word1:
             *   [7:0]   = size offset (0-127 bytes, signed)
             *   [31:8]  = tag number high 24 bits
             * 
             * word2:
             *   [31:0]  = data offset (full 32-bit offset)
             * 
             * word3:
             *   [31:0]  = data size (full 32-bit size)
             * 
             * word4:
             *   [31:0]  = array size (full 32-bit element count)
             * 
             * Remaining words (5-7) are reserved for future use.
             */
            uint32_t word1 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 1]);
            uint32_t word2 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 2]);
            uint32_t word3 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 3]);
            uint32_t word4 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index + 4]);

            iter->array_size = (pb_size_t)word4;
            iter->tag = (pb_size_t)(((word0 >> 2) & 0x3F) | ((word1 >> 8) << 6));
            size_offset = (int_least8_t)(word1 & 0xFF);
            data_offset = word2;
            iter->data_size = (pb_size_t)word3;
            break;
        }
    }

    if (!iter->message)
    {
        /* Avoid doing arithmetic on null pointers, it is undefined */
        iter->pField = NULL;
        iter->pSize = NULL;
    }
    else
    {
        iter->pField = (char*)iter->message + data_offset;

        if (size_offset)
        {
            iter->pSize = (char*)iter->pField - size_offset;
        }
        else if (PB_HTYPE(iter->type) == PB_HTYPE_REPEATED &&
                 (PB_ATYPE(iter->type) == PB_ATYPE_STATIC ||
                  PB_ATYPE(iter->type) == PB_ATYPE_POINTER))
        {
            /* Fixed count array */
            iter->pSize = &iter->array_size;
        }
        else
        {
            iter->pSize = NULL;
        }

        if (PB_ATYPE(iter->type) == PB_ATYPE_POINTER && iter->pField != NULL)
        {
            iter->pData = *(void**)iter->pField;
        }
        else
        {
            iter->pData = iter->pField;
        }
    }

    if (PB_LTYPE_IS_SUBMSG(iter->type))
    {
        iter->submsg_desc = iter->descriptor->submsg_info[iter->submessage_index];
    }
    else
    {
        iter->submsg_desc = NULL;
    }

    return true;
}

static void advance_iterator(pb_field_iter_t *iter)
{
    iter->index++;

    if (iter->index >= iter->descriptor->field_count)
    {
        /* Restart */
        iter->index = 0;
        iter->field_info_index = 0;
        iter->submessage_index = 0;
        iter->required_field_index = 0;
    }
    else
    {
        /* Increment indexes based on previous field type.
         * All field info formats have the following fields:
         * - lowest 2 bits tell the amount of words in the descriptor (2^n words)
         * - bits 2..7 give the lowest bits of tag number.
         * - bits 8..15 give the field type.
         */
        uint32_t prev_descriptor = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index]);
        pb_type_t prev_type = (prev_descriptor >> 8) & 0xFF;
        pb_size_t descriptor_len = (pb_size_t)(1 << (prev_descriptor & 3));

        /* Add to fields.
         * The cast to pb_size_t is needed to avoid -Wconversion warning.
         * Because the data is is constants from generator, there is no danger of overflow.
         */
        iter->field_info_index = (pb_size_t)(iter->field_info_index + descriptor_len);
        iter->required_field_index = (pb_size_t)(iter->required_field_index + (PB_HTYPE(prev_type) == PB_HTYPE_REQUIRED));
        iter->submessage_index = (pb_size_t)(iter->submessage_index + PB_LTYPE_IS_SUBMSG(prev_type));
    }
}

bool pb_field_iter_begin(pb_field_iter_t *iter, const pb_msgdesc_t *desc, void *message)
{
    memset(iter, 0, sizeof(*iter));

    iter->descriptor = desc;
    iter->message = message;

    return load_descriptor_values(iter);
}

bool pb_field_iter_begin_extension(pb_field_iter_t *iter, pb_extension_t *extension)
{
    const pb_msgdesc_t *msg = (const pb_msgdesc_t*)extension->type->arg;
    bool status;

    uint32_t word0 = PB_PROGMEM_READU32(msg->field_info[0]);
    if (PB_ATYPE(word0 >> 8) == PB_ATYPE_POINTER)
    {
        /* For pointer extensions, the pointer is stored directly
         * in the extension structure. This avoids having an extra
         * indirection. */
        status = pb_field_iter_begin(iter, msg, &extension->dest);
    }
    else
    {
        status = pb_field_iter_begin(iter, msg, extension->dest);
    }

    iter->pSize = &extension->found;
    return status;
}

bool pb_field_iter_next(pb_field_iter_t *iter)
{
    advance_iterator(iter);
    (void)load_descriptor_values(iter);
    return iter->index != 0;
}

/* Find a field by tag number.
 * 
 * This function performs a circular search through the field descriptors to find
 * a field with the given tag number. The search is optimized with a fast path
 * that checks only the low 6 bits of the tag before doing a full descriptor load.
 * 
 * Search strategy:
 * 1. If we're already on the target tag, return immediately
 * 2. If tag is larger than any in this message, return false
 * 3. If tag is less than current position, wrap around to start
 * 4. Search forward from current position using circular iteration
 * 5. Fast check: Compare low 6 bits of tag (stored in all formats)
 * 6. On match, do full descriptor load and verify complete tag
 * 7. Continue until we wrap back to starting position
 * 
 * Why circular search?
 * Fields are generated in tag number order, but the user may decode fields
 * in any order (especially when skipping unknown fields). Starting from the
 * last found position and wrapping around gives O(n) worst case but often
 * O(1) for sequential access.
 */
bool pb_field_iter_find(pb_field_iter_t *iter, uint32_t tag)
{
    if (iter->tag == tag)
    {
        return true; /* Nothing to do, correct field already. */
    }
    else if (tag > iter->descriptor->largest_tag)
    {
        return false;
    }
    else
    {
        pb_size_t start = iter->index;
        uint32_t fieldinfo;

        if (tag < iter->tag)
        {
            /* Fields are in tag number order, so we know that tag is between
             * 0 and our start position. Setting index to end forces
             * advance_iterator() call below to restart from beginning. */
            iter->index = iter->descriptor->field_count;
        }

        do
        {
            /* Advance iterator but don't load values yet */
            advance_iterator(iter);

            /* Do fast check for tag number match */
            fieldinfo = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index]);

            if (((fieldinfo >> 2) & 0x3F) == (tag & 0x3F))
            {
                /* Good candidate, check further */
                (void)load_descriptor_values(iter);

                if (iter->tag == tag &&
                    PB_LTYPE(iter->type) != PB_LTYPE_EXTENSION)
                {
                    /* Found it */
                    return true;
                }
            }
        } while (iter->index != start);

        /* Searched all the way back to start, and found nothing. */
        (void)load_descriptor_values(iter);
        return false;
    }
}

bool pb_field_iter_find_extension(pb_field_iter_t *iter)
{
    if (PB_LTYPE(iter->type) == PB_LTYPE_EXTENSION)
    {
        return true;
    }
    else
    {
        pb_size_t start = iter->index;
        uint32_t fieldinfo;

        do
        {
            /* Advance iterator but don't load values yet */
            advance_iterator(iter);

            /* Do fast check for field type */
            fieldinfo = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index]);

            if (PB_LTYPE((fieldinfo >> 8) & 0xFF) == PB_LTYPE_EXTENSION)
            {
                return load_descriptor_values(iter);
            }
        } while (iter->index != start);

        /* Searched all the way back to start, and found nothing. */
        (void)load_descriptor_values(iter);
        return false;
    }
}

static void *pb_const_cast(const void *p)
{
    /* Note: this casts away const, in order to use the common field iterator
     * logic for both encoding and decoding. The cast is done using union
     * to avoid spurious compiler warnings. */
    union {
        void *p1;
        const void *p2;
    } t;
    t.p2 = p;
    return t.p1;
}

bool pb_field_iter_begin_const(pb_field_iter_t *iter, const pb_msgdesc_t *desc, const void *message)
{
    return pb_field_iter_begin(iter, desc, pb_const_cast(message));
}

bool pb_field_iter_begin_extension_const(pb_field_iter_t *iter, const pb_extension_t *extension)
{
    return pb_field_iter_begin_extension(iter, (pb_extension_t*)pb_const_cast(extension));
}

bool pb_default_field_callback(pb_istream_t *istream, pb_ostream_t *ostream, const pb_field_t *field)
{
    if (field->data_size == sizeof(pb_callback_t))
    {
        pb_callback_t *pCallback = (pb_callback_t*)field->pData;

        if (pCallback != NULL)
        {
            if (istream != NULL && pCallback->funcs.decode != NULL)
            {
                return pCallback->funcs.decode(istream, field, &pCallback->arg);
            }

            if (ostream != NULL && pCallback->funcs.encode != NULL)
            {
                return pCallback->funcs.encode(ostream, field, &pCallback->arg);
            }
        }
    }

    return true; /* Success, but didn't do anything */

}

#ifdef PB_VALIDATE_UTF8

/* This function checks whether a string is valid UTF-8 text.
 *
 * Algorithm is adapted from https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
 * Original copyright: Markus Kuhn <http://www.cl.cam.ac.uk/~mgk25/> 2005-03-30
 * Licensed under "Short code license", which allows use under MIT license or
 * any compatible with it.
 */

bool pb_validate_utf8(const char *str)
{
    const pb_byte_t *s = (const pb_byte_t*)str;
    while (*s)
    {
        if (*s < 0x80)
        {
            /* 0xxxxxxx */
            s++;
        }
        else if ((s[0] & 0xe0) == 0xc0)
        {
            /* 110XXXXx 10xxxxxx */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[0] & 0xfe) == 0xc0)                        /* overlong? */
                return false;
            else
                s += 2;
        }
        else if ((s[0] & 0xf0) == 0xe0)
        {
            /* 1110XXXX 10Xxxxxx 10xxxxxx */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||    /* overlong? */
                (s[0] == 0xed && (s[1] & 0xe0) == 0xa0) ||    /* surrogate? */
                (s[0] == 0xef && s[1] == 0xbf &&
                (s[2] & 0xfe) == 0xbe))                 /* U+FFFE or U+FFFF? */
                return false;
            else
                s += 3;
        }
        else if ((s[0] & 0xf8) == 0xf0)
        {
            /* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80 ||
                (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||    /* overlong? */
                (s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4) /* > U+10FFFF? */
                return false;
            else
                s += 4;
        }
        else
        {
            return false;
        }
    }

    return true;
}

#endif

