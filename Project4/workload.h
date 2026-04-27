/* ============================================================
 * workload.h - 工作负载生成模块
 * 负责人: Member 1 (Team Lead)
 * 功能: 生成随机进程列表 (按到达时间排序的链表)
 * ============================================================ */
#ifndef WORKLOAD_H
#define WORKLOAD_H

#include "common.h"

/* 生成 NUM_PROCESSES 个随机进程,返回按到达时间排序的链表头 */
Process *generate_workload(int count);

/* 释放进程链表 */
void free_workload(Process *head);

/* 打印工作负载（调试用） */
void print_workload(Process *head);

#endif
