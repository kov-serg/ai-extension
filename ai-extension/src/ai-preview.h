// ai-preview.h
#pragma once

int getimage(
	int (*read )(void* ctx,void* data,int size),void *read_ctx,
	int (*write)(void* ctx,void* data,int size),void *write_ctx
);
