// Copyright (c) 2017 Krzysztof Sitko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

typedef struct
{
    int length;       /* binary length in bytes  */
    char local_hash[32];   /* local MD5hash */
    char remote_hash[32];  /* remote MD5hash */
}
ota_header_context;

/*
 * Initialize and zero context variable.
 */
void validate_init(ota_header_context *context);

/*
 * Confirm that buffer conatins valid ESP32 vinary start bits
 */
bool validate_bin(char *text);

/*
 * Extract the size (in bytes) of the binary from the header
 */
bool remote_get_size(ota_header_context *context, char *header);

/*
 * Extract the Etag (MD5sum) of the binary from the header
 */
bool remote_get_hash(ota_header_context *context, char *header);

/*
 * Extract meta binary information from header
 */
bool validate_remote_headers(ota_header_context *context, char *header);

/*
 * Calculate MD5sum of provided local partion. Return as a char array.
 */
bool local_get_hash(ota_header_context *context, const esp_partition_t* partition);

/*
 * Compare local and remote hashes.
 */
bool validate_hashes(ota_header_context *context, const esp_partition_t* partition);

/*
 * Get extracted binary length
 */
int bin_length(ota_header_context *context);

/*
How To use:
ota_header_context ctx;
validate_init(&ctx);
validate_remote_headers(&ctx,buffer_containing_header); //after pulling binary header
validate_bin(buffer_containing_bin); //confirm first bin is valid via first 2 bytes
bin_length(&ctx); //write bin_length number of bytes
validate_hashes(&ctx, &partition); //after writing binary to partition
*/
