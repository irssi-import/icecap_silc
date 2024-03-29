#ifndef __BASE64_H
#define __BASE64_H

/* Translates binary data into base64. The src must not point to dest buffer. */
void base64_encode(const void *src, size_t src_size, buffer_t *dest);

/* Translates base64 data into binary and appends it to dest buffer. dest may
   point to same buffer as src. Returns 0 if all ok, -1 if data is invalid.
   Any CR, LF characters are ignored, as well as whitespace at beginning or
   end of line.

   This function may be called multiple times for parsing the same stream.
   If src_pos is non-NULL, it's updated to first non-translated character in
   src. */
int base64_decode(const void *src, size_t src_size,
		  size_t *src_pos_r, buffer_t *dest);

/* max. buffer size required for base64_encode() */
#define MAX_BASE64_ENCODED_SIZE(size) \
	((size) / 3 * 4 + 2+2)
/* max. buffer size required for base64_decode() */
#define MAX_BASE64_DECODED_SIZE(size) \
	((size) / 4 * 3 + 3)
#endif
