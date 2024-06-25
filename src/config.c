#include "config.h"
char config_file[LOG_NAME_LEN] = "./server.conf";
char server_log_dir[LOG_NAME_LEN] = "/tmp/";
char user_log_dir[LOG_NAME_LEN] = "/tmp/";
char server_log_file[LOG_NAME_LEN] = "/tmp/server.log";
char thread_pool_log_file[LOG_NAME_LEN] = "/tmp/thread_pool.log";

char reg_fifo[LOG_NAME_LEN] = "/tmp/reg_fifo";
char login_fifo[LOG_NAME_LEN] = "/tmp/log_fifo";
char msg_fifo[LOG_NAME_LEN] = "/tmp/msg_fifo";
char logout_fifo[LOG_NAME_LEN] = "/tmp/logout_fifo";

int config_thread_pool_size = 10;