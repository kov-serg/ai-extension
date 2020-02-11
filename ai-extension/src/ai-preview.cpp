// ai-preview.c

static int find_fragment(
	const char* head, int head_len, const char* tail, int tail_len,
	int (*read) (void* ctx,void* data,int size),void* read_ctx,
	int (*write)(void* ctx,void* data,int size),void* write_ctx,
	char *block, int block_limit)
{
	int block_size,state,pos,i,k,m,m0;
	state=0; m=0;
	do {
		pos=0;
		block_size=read(read_ctx,block,block_limit);
		if (state==0) {
			for(i=pos;i<block_size;++i) {
				if (block[i]==head[m]) {
					if (++m==head_len) { state=1; m=0; pos=i+1; break; }
				} else m=0;
			}
		}
		if (state==1) {
			m0=m;
			for(i=pos;i<block_size;++i) {
				if (block[i]==tail[m]) {
					if (++m==tail_len) { state=2;--m; m0=0; break; }
				} else {
					m=0;
				}
			}
			k=i-pos-m;
			if (m0) { write(write_ctx,(void*)tail,m0); }
			if (k>0) write(write_ctx,&block[pos],k);
		}
		if (state==2) break;
	} while(block_size==block_limit);
	return state==2 ? 0 : 1;
}

//------------------------------------------------------------------------------

typedef struct base64decoder_s {
	char *buffer;
	int buffer_size, buffer_limit;
	int st, acc_len, acc;
	int (*write)(void *ctx,void *data,int size); void *write_ctx;
} base64_decoder_t;

static void base64_decoder_reset(base64_decoder_t *it) {
	it->st=0;
	it->acc_len=0;
	it->acc=0;
	it->buffer_size=0;
}

static void base64_decoder_flush(base64_decoder_t *it) {
	if (it->buffer_size) {
		it->write(it->write_ctx,it->buffer,it->buffer_size);
		it->buffer_size=0;
	}
	if (it->acc_len) {
		char buf[3];int sz;
		it->acc<<=6*(4-it->acc_len);
		buf[0]=(it->acc>>16)&255;
		buf[1]=(it->acc>>8)&255;
		buf[2]=(it->acc)&255;
		if (it->acc_len<=2) sz=1; else if (it->acc_len==4) sz=3; else sz=2;
		it->write(it->write_ctx,buf,sz);
		it->acc_len=0;
	}
}

static int base64_c2i(char c) {
	if (c>='A' && c<='Z') return c-'A';
	if (c>='a' && c<='z') return c-'a'+26;
	if (c>='0' && c<='9') return c-'0'+52;
	return c=='+' ? 62 : c=='/' ? 63 : -1;
}

// A=== -> 0
// AA== -> 0
// AAA= -> 0,0
// AAAA -> 0,0,0
// st=0 wait b64 char
// st=1 found & skip all until ;
static int base64_decoder_write(base64_decoder_t *it,const char* text,int size)
{
	int i,j,c,a;char ch;
	for(i=0;i<size;++i) {
		ch=text[i];
		if (it->st==0) {
			c=base64_c2i(ch);
			if (c>=0) {
				it->acc=(it->acc<<6)|c;
				if (++it->acc_len>=4) {
					a=it->acc; it->acc_len=0; it->acc=0;
					for(j=0;j<3;++j,a<<=8) {
						it->buffer[it->buffer_size]=(a>>16)&255;
						if (++it->buffer_size>=it->buffer_limit) {
							base64_decoder_flush(it);
						}
					}
				}
			} else {
				// ignore filler ch=='='
				if (ch=='&') it->st=1; // skip new lines &#xA; and other &.*;
			}
		} else if (it->st==1) {
			if (ch==';') it->st=0;
		}
	}
	return size;
}

//------------------------------------------------------------------------------

static int getimage_write(void* ctx,void* data,int size) {
	return base64_decoder_write((base64_decoder_t*)ctx,(const char*)data,size);
}

int getimage(
	int (*read) (void* ctx,void* data,int size),void *read_ctx,
	int (*write)(void* ctx,void* data,int size),void *write_ctx
	)
{
	enum { read_buffer_size=4096, write_buffer_size=4096 };
	const char *head="image>", *tail="</"; enum { head_len=6, tail_len=2 };
	char read_buffer[read_buffer_size], write_buffer[write_buffer_size];
	base64_decoder_t b64[1];
	b64->buffer=write_buffer;
	b64->buffer_limit=write_buffer_size;
	b64->write=write;
	b64->write_ctx=write_ctx;
	base64_decoder_reset(b64);
	find_fragment(head,head_len,tail,tail_len,
		read,read_ctx,getimage_write,b64,
		read_buffer,read_buffer_size);
	base64_decoder_flush(b64);
	return 0;
}


