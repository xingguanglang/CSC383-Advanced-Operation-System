#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "common.h"

extern Frame g_frames[TOTAL_FRAMES];

void init_frames(void);

int  free_frame_count(void);

int  find_page(int pid, int page_num);

int  allocate_frame(int pid, const char *pname, int page_num, int now_ms);

void free_frame(int frame_idx);

void free_process_frames(int pid);

void build_memory_map(char *out, int out_size);

#endif
