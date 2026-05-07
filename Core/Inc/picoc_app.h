/* PicoC 应用层头文件 — 串口 REPL 交互与脚本加载引擎 API
 *
 * 对外暴露三个接口，供 main.c 和平台适配层调用：
 *   PicocApp_Init()              — 初始化 PicoC 解释器并显示启动提示符
 *   PicocApp_Task()              — 主循环轮询函数（需在 main 大循环中反复调用）
 *   PicocApp_ConsoleGetCharBlocking() — 阻塞读取单个控制台字符（供平台 I/O 层使用）
 */

#ifndef __PICOC_APP_H__
#define __PICOC_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 PicoC 应用：创建解释器实例，重置缓冲区，显示 REPL 提示符 */
void PicocApp_Init(void);

/* 主循环任务：轮询 UART 串口，处理输入字符并执行脚本 */
void PicocApp_Task(void);

/* 阻塞读取单个控制台字符，无数据时忙等待 */
int PicocApp_ConsoleGetCharBlocking(void);

#ifdef __cplusplus
}
#endif

#endif /* __PICOC_APP_H__ */
