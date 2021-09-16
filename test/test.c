#include "test.h"
#include "test_fs.h"

int num_test_tasks = 1;

task_info_t fs_task = {"test_fs", test_fs, USER_PROCESS};

task_info_t *test_tasks[1] = {(uint32_t)&fs_task};