/* ============================================================
 * page_table.c - 页框/内存管理实现
 * 负责人: Member 2
 *
 * 内存映射输出格式说明:
 *   每个字符代表一个 1MB 页框
 *   - '.'  表示空闲
 *   - 字母 表示某进程占用的页框
 *   - 大小写交替/数字, 用于区分相邻进程
 * ============================================================ */
#include "page_table.h"

Frame g_frames[TOTAL_FRAMES];

void init_frames(void) {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        g_frames[i].in_use       = false;
        g_frames[i].owner_pid    = -1;
        g_frames[i].owner_name[0]= '\0';
        g_frames[i].page_num     = -1;
        g_frames[i].load_time_ms = 0;
        g_frames[i].last_used_ms = 0;
        g_frames[i].use_count    = 0;
    }
}

int free_frame_count(void) {
    int c = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++)
        if (!g_frames[i].in_use) c++;
    return c;
}

int find_page(int pid, int page_num) {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (g_frames[i].in_use &&
            g_frames[i].owner_pid == pid &&
            g_frames[i].page_num  == page_num) {
            return i;
        }
    }
    return -1;
}

int allocate_frame(int pid, const char *pname, int page_num, int now_ms) {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!g_frames[i].in_use) {
            g_frames[i].in_use       = true;
            g_frames[i].owner_pid    = pid;
            strncpy(g_frames[i].owner_name, pname, MAX_PROC_NAME - 1);
            g_frames[i].owner_name[MAX_PROC_NAME - 1] = '\0';
            g_frames[i].page_num     = page_num;
            g_frames[i].load_time_ms = now_ms;
            g_frames[i].last_used_ms = now_ms;
            g_frames[i].use_count    = 1;
            return i;
        }
    }
    return -1; /* 无空闲页框 */
}

void free_frame(int frame_idx) {
    if (frame_idx < 0 || frame_idx >= TOTAL_FRAMES) return;
    g_frames[frame_idx].in_use       = false;
    g_frames[frame_idx].owner_pid    = -1;
    g_frames[frame_idx].owner_name[0]= '\0';
    g_frames[frame_idx].page_num     = -1;
    g_frames[frame_idx].use_count    = 0;
}

void free_process_frames(int pid) {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (g_frames[i].in_use && g_frames[i].owner_pid == pid) {
            free_frame(i);
        }
    }
}

void build_memory_map(char *out, int out_size) {
    int n = (out_size - 1 < TOTAL_FRAMES) ? (out_size - 1) : TOTAL_FRAMES;
    for (int i = 0; i < n; i++) {
        if (!g_frames[i].in_use) {
            out[i] = '.';
        } else {
            /* 取进程名首字符;若是 P02 这种则取数字 */
            char c = g_frames[i].owner_name[0];
            if (c == 'P' && g_frames[i].owner_name[1] != '\0') {
                /* 用 owner_pid % 26 后映射成小写字母, 区分 A-Z 段 */
                c = 'a' + (g_frames[i].owner_pid % 26);
            }
            out[i] = c;
        }
    }
    out[n] = '\0';
}
