#ifndef __SBUF_H__
#define __SBUF_H__

#include <stdbool.h>

extern int debug;

// sbuf: a ring buffer, the last byte indicate the buffer state.
typedef struct sbuf{
	unsigned char *buffer;
	unsigned int rcursor;	// read pointer
	unsigned int wcursor;	// write pointer
	unsigned int length;	// the length of buffer; buffer contains at most length-1 bytes
} sbuf_t, *psbuf_t;

struct sbuf* sbuf_init(unsigned int len);
int sbuf_get_count(struct sbuf *sb);
bool sbuf_is_full(struct sbuf *sb);
bool sbuf_is_overflow(struct sbuf *sb, unsigned int len);
int sbuf_in(struct sbuf *sb, unsigned char *data, unsigned int len);
int sbuf_out(struct sbuf *sb, unsigned char *data, unsigned int len);
int sbuf_cmp(struct sbuf *sb, unsigned char *data, unsigned int len);
void sbuf_change_cursor(struct sbuf *sb, char mode, unsigned int off);
void sbuf_destroy(struct sbuf *sb);

typedef bool (*func_check)(const unsigned char *data, unsigned int len);
int sbuf_search_with_count(struct sbuf *sb, unsigned char *token, unsigned int token_len, unsigned char *out, unsigned int out_len, func_check func);
int sbuf_search_with_out_count(struct sbuf *sb, unsigned char *token, unsigned int token_len, unsigned char *out, unsigned int *out_len, func_check func);

void sbuf_print(struct sbuf *sb);

#endif
