/* ============================================================
 * simulator.h - 模拟器引擎头文件
 * 负责人: Member 5
 *
 * 提供:
 *   - run_simulation(): 用指定算法跑一次完整模拟 (1分钟)
 *   - 局部性引用算法: next_page_with_locality()
 * ============================================================ */
#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "common.h"

/* 用指定算法跑一次模拟,返回该次的统计 */
RunStats run_simulation(AlgorithmType alg,
                        Process *workload_template,
                        bool verbose);

/* 局部性引用: 给定当前页 i 和进程页数 size, 返回下一个引用页号 */
int next_page_with_locality(int i, int size);

/* 深拷贝工作负载链表 (每次模拟独立) */
Process *clone_workload(Process *src);

#endif
