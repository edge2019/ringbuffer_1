#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "sbuf.h"

int debug = 0;

void sbuf_print(struct sbuf *sb)
{
	printf("////------------sbuf----------\n");
	printf("rcursor:%d, wcursor:%d, length:%d\n", sb->rcursor, sb->wcursor, sb->length);
	for(int i = 0; i < sb->length; i++){
		printf("0x%.2x, ", ((unsigned char)(sb->buffer[i]))&0xFF);
		if((i) % 10 == 0){
			printf("\n");
		}
	}
	printf("\n");
	printf("------------sbuf----------\\\\\\\n");
}

struct sbuf* sbuf_init(unsigned int len)
{
	if( 0 >= len){
		return NULL;	
	}

	unsigned char *p = (unsigned char *)malloc(len);
	struct sbuf *sb = (struct sbuf *)malloc(sizeof(struct sbuf));
	if(NULL == sb || NULL == p){
		return NULL;
	}
	
	sb->buffer = p;
	sb->rcursor = sb->wcursor = 0;
	sb->length = len;

	return sb;
}

int sbuf_get_count(struct sbuf *sb)
{
	return (sb->wcursor + sb->length - sb->rcursor) % sb->length;
}

bool sbuf_is_full(struct sbuf *sb)
{
	if((sb->wcursor + 1)%(sb->length) == sb->rcursor){
		return true;
	} else {
		return false;
	}	
}

bool sbuf_is_empty(struct sbuf *sb)
{
	if(sb->wcursor == sb->rcursor){
		return true;
	} else {
		return false;
	}	
}

bool sbuf_is_overflow(struct sbuf *sb, unsigned int len)
{
	if(sbuf_get_count(sb) + len > (sb->length -1))
		return true;
	return false;
}

int sbuf_in(struct sbuf *sb, unsigned char *data, unsigned int len)
{
	if(NULL == sb || NULL == data || 0 == len){
		return -1;
	}
	
	if(!sbuf_is_overflow(sb, len)){
		int left = sb->length - sb->wcursor;

		if(len <= left){
			memcpy(sb->buffer+sb->wcursor, data, len);
			sb->wcursor += len; 
		} else {
			memcpy(sb->buffer+sb->wcursor, data, left);
			memcpy(sb->buffer, data+left, len-left);
			sb->wcursor = len - left; 
		}	

		return len;
	}

	return -1;
}

int sbuf_out(struct sbuf *sb, unsigned char *data, unsigned int len)
{
	if(NULL == sb || NULL == data || 0 >= len){
		return -1;
	}

	int count = sbuf_get_count(sb);
	int left = sb->length - sb->rcursor;

	len = (len <= count? len:count);

	if(len <= left){
		memcpy(data, sb->buffer+sb->rcursor, len);
		sb->rcursor += len; 
	} else {
		memcpy(data, sb->buffer+sb->rcursor, left);
		memcpy(data+left, sb->buffer, len-left);
		sb->rcursor = len -left; 
	}	

	return len;
}

int sbuf_out_byte(struct sbuf *sb, unsigned char *ch, unsigned int off)
{
	if(NULL == sb || NULL == ch ){
		return -1;
	}

	*ch = (sb->buffer[off]) & 0xFF;
	return 1;
}

int sbuf_cmp(struct sbuf *sb, unsigned char *data, unsigned int len)
{
	if(NULL == sb || NULL == data || 0 >= len){
		return -1;
	}

	int reta =1,  retb = 1, retc = 1;
	if(sb->rcursor + len <= sb->length){		
		reta = memcmp(sb->buffer+sb->rcursor, data, len);
		return reta;
	} else {
		int counta = sb->length - sb->rcursor;
		retb = memcmp(sb->buffer+sb->rcursor, data, counta);
		if(0 == retb){
			retc = memcmp(sb->buffer, data + counta, len - counta);
			return retc; 
		} else {
			return retb;
		}
	}
}

// off is + or -
void sbuf_change_cursor(struct sbuf *sb, char mode, unsigned int off)
{
	if(NULL == sb){
		return ;
	}

	if('r' == mode){
		sb->rcursor = (sb->rcursor + off + sb->length) % sb->length;
	} else if('w' == mode) {
		sb->wcursor = (sb->wcursor + off + sb->length) % sb->length;	
	}	
}

unsigned int sbuf_get_position(struct sbuf *sb, char mode, unsigned int off)
{
	unsigned int pos = 0;
	if('r' == mode){
		pos = sb->rcursor;
	} else if('w' == mode) {
		pos = sb->wcursor;	
	}	

	return ((pos + off + sb->length) % sb->length);	
}

void sbuf_destroy(struct sbuf *sb)
{
	if(sb->buffer)
		free(sb->buffer);

	if(sb)
		free(sb);

	sb = NULL;
}

typedef bool (*func_check)(const unsigned char *data, unsigned int len);


// out contains token, and token_len <= out_len
// func_check using buffer with the size FUNC_BUFFER_SIZE to check data. 
// input: sb, token, token_len, out, out_len, func_check_func
// output: out
#define FUNC_BUFFER_SIZE	512 
int sbuf_search_with_count(struct sbuf *sb, unsigned char *token, unsigned int token_len, unsigned char *out, unsigned int out_len, func_check func)
{
	if((NULL == sb || NULL == token || NULL == out) || (token_len <= 0 || out_len <= 0) ){
		return -1;
	}	

	int j = 0;
	int count = sbuf_get_count(sb);
	int len = token_len > out_len? token_len:out_len;

	if(len <= count){
		for(j=0; j <= count - len; j++, sbuf_change_cursor(sb, 'r', 1)){

			if(!sbuf_cmp(sb, token, token_len)){
				
				unsigned char func_buf[FUNC_BUFFER_SIZE] = {0};
				sbuf_out(sb, func_buf, out_len);
				if((func&&func(func_buf, out_len)) || (!func)){
					memcpy(out, func_buf, out_len);
					return out_len;
				} else {
					sbuf_change_cursor(sb, 'r', 0-out_len);					
				}
			}
		}
	} 

	return 0;	
}

// out contains token, and token_len <= out_len
// func_check using buffer with the size FUNC_BUFFER_SIZE to check data. 
// input: sb, token, token_len, out, func_check_func
// output: out, out_len
int sbuf_search_with_out_count(struct sbuf *sb, unsigned char *token, unsigned int token_len, unsigned char *out, unsigned int *out_len, func_check func)
{
	if((NULL == sb || NULL == token || NULL == out) || (token_len <= 0 || out_len <= 0) ){
		return -1;
	}	

	int j = 0;
	int count = sbuf_get_count(sb);
	int counta = 0;
	// data_len, the first byte after token
	unsigned char data_len = 0;
	int len = token_len+1;
	unsigned int pos = 0;

	if(len <= count){
		for(j=0; j <= count - len; j++, sbuf_change_cursor(sb, 'r', 1)){

			if(!sbuf_cmp(sb, token, token_len)){
				counta = sbuf_get_count(sb);
				// get out_len, the first byte after token
				if(counta > len){
					pos = sbuf_get_position(sb, 'r', token_len);
					if(1 != sbuf_out_byte(sb, &data_len, pos)){
						return 0;
					} 
				} else {
					return 0;
				}

				if(1 == debug)
					printf("the frame info: pos [%d+1]:[0x%x], data_len [%d]:[0x%x], count [%d]\n", sb->rcursor, pos, data_len, sb->buffer[pos], counta);

				// check sb count
				if(counta < (data_len + token_len + 1)){
					return 0;	
				}
				
				*out_len = data_len + token_len + 1;
				unsigned char func_buf[FUNC_BUFFER_SIZE] = {0};
				sbuf_out(sb, func_buf, *out_len);
				if((func&&func(func_buf, *out_len)) || (!func)){
					memcpy(out, func_buf, *out_len);
					return *out_len;
				} else {
					sbuf_change_cursor(sb, 'r', 0-(*out_len));					
				}
			}
		}
	} 

	return 0;	
}

