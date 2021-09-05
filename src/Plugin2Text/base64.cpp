/*
   base64.cpp and base64.h
   base64 encoding and decoding with C++.
   More information at
     https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
   Version: 2.rc.08 (release candidate)
   Copyright (C) 2004-2017, 2020, 2021 René Nyffenegger
   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.
   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:
   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.
   3. This notice may not be removed or altered from any source distribution.
   René Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/

#include "base64.hpp"
#include "common.hpp"

//
// Depending on the url parameter in base64_chars, one of
// two sets of base64 characters needs to be chosen.
// They differ in their last two characters.
//
static const char* base64_chars = {
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789"
             "+/", };

static unsigned int pos_of_char(const unsigned char chr) {
    //
    // Return the position of chr within base64_encode()
    //

    if (chr >= 'A' && chr <= 'Z') return chr - 'A';
    else if (chr >= 'a' && chr <= 'z') return chr - 'a' + ('Z' - 'A') + 1;
    else if (chr >= '0' && chr <= '9') return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
    else if (chr == '+' || chr == '-') return 62; // Be liberal with input and accept both url ('-') and non-url ('+') base 64 characters (
    else if (chr == '/' || chr == '_') return 63; // Ditto for '/' and '_'
    else {
        verify(false);
        return 0;
    }
}

size_t base64_encode(const uint8_t* bytes_to_encode, size_t in_len, char* out_buffer, size_t out_buffer_size) {
    size_t len_encoded = (in_len + 2) / 3 * 4;
    verify(len_encoded <= out_buffer_size);

    size_t pos = 0;

    auto now = out_buffer;

    while (pos < in_len) {
        *now++ = base64_chars[(bytes_to_encode[pos + 0] & 0xfc) >> 2];

        if (pos + 1 < in_len) {
            *now++ = base64_chars[((bytes_to_encode[pos + 0] & 0x03) << 4) + ((bytes_to_encode[pos + 1] & 0xf0) >> 4)];

            if (pos + 2 < in_len) {
                *now++ = base64_chars[((bytes_to_encode[pos + 1] & 0x0f) << 2) + ((bytes_to_encode[pos + 2] & 0xc0) >> 6)];
                *now++ = base64_chars[bytes_to_encode[pos + 2] & 0x3f];
            } else {
                *now++ = base64_chars[(bytes_to_encode[pos + 1] & 0x0f) << 2];
                *now++ = '=';
            }
        } else {
            *now++ = base64_chars[(bytes_to_encode[pos + 0] & 0x03) << 4];
            *now++ = '=';
            *now++ = '=';
        }

        pos += 3;
    }

    return len_encoded;
}

size_t base64_decode(const char* encoded_string, size_t encoded_string_count, uint8_t* out_buffer, size_t out_buffer_size) {
    size_t length_of_string = encoded_string_count;
    size_t pos = 0;

    auto now = out_buffer;
    auto end = out_buffer + out_buffer_size;

    while (pos < length_of_string) {
        //
        // Iterate over encoded input string in chunks. The size of all
        // chunks except the last one is 4 bytes.
        //
        // The last chunk might be padded with equal signs or dots
        // in order to make it 4 bytes in size as well, but this
        // is not required as per RFC 2045.
        //
        // All chunks except the last one produce three output bytes.
        //
        // The last chunk produces at least one and up to three bytes.
        //

        int pos_of_char_1 = pos_of_char(encoded_string[pos + 1]);

        //
        // Emit the first output byte that is produced in each chunk:
        //
        verify(now < end);
        *now++ = ((pos_of_char(encoded_string[pos + 0])) << 2) + ((pos_of_char_1 & 0x30) >> 4);

        // Check for data that is not padded with equal signs (which is allowed by RFC 2045)
        if ((pos + 2 < length_of_string) && encoded_string[pos + 2] != '=') {
            //
            // Emit a chunk's second byte (which might not be produced in the last chunk).
            //
            unsigned int pos_of_char_2 = pos_of_char(encoded_string[pos + 2]);
            verify(now < end);
            *now++ = ((pos_of_char_1 & 0x0f) << 4) + ((pos_of_char_2 & 0x3c) >> 2);

            if ((pos + 3 < length_of_string) && encoded_string[pos + 3] != '=') {
                //
                // Emit a chunk's third byte (which might not be produced in the last chunk).
                //
                verify(now < end);
                *now++ = ((pos_of_char_2 & 0x03) << 6) + pos_of_char(encoded_string[pos + 3]);
            }
        }

        pos += 4;
    }

    return now - out_buffer;
}
