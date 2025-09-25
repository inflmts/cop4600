#ifndef _PROCESS_LOG_H
#define _PROCESS_LOG_H

#include <stdlib.h>
#include <unistd.h>

#define __NR_get_proc_log_level 500
#define __NR_set_proc_log_level 501
#define __NR_proc_log_message 502

// Library functions

static inline int get_proc_log_level(void)
{
	return syscall(__NR_get_proc_log_level);
}

static inline int set_proc_log_level(int new_level)
{
	return syscall(__NR_set_proc_log_level, (long)new_level);
}

static inline int proc_log_message(int level, char *message)
{
	return syscall(__NR_proc_log_message, message, (long)level);
}


// Harness functions

#define PROC_LOG_CALL __NR_proc_log_message

static inline int *retrieve_set_level_params(int new_level)
{
	int *params = (int *)calloc(3, sizeof(int));
	params[0] = __NR_set_proc_log_level;
	params[1] = 1;
	params[2] = new_level;
	return params;
}

static inline int *retrieve_get_level_params(void)
{
	int *params = (int *)calloc(2, sizeof(int));
	params[0] = __NR_get_proc_log_level;
	params[1] = 0;
	return params;
}

static inline int interpret_set_level_result(int value)
{
	return value;
}

static inline int interpret_get_level_result(int value)
{
	return value;
}

static inline int interpret_log_message_result(int value)
{
	return value;
}

#endif
