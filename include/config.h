#pragma once
#define LOG_NAME_LEN 128
extern char config_file[LOG_NAME_LEN];
extern char server_log_dir[LOG_NAME_LEN];
extern char user_log_dir[LOG_NAME_LEN];
extern char server_log_file[LOG_NAME_LEN];
extern char thread_pool_log_file[LOG_NAME_LEN];
extern char reg_fifo[LOG_NAME_LEN];
extern char login_fifo[LOG_NAME_LEN];
extern char msg_fifo[LOG_NAME_LEN];
extern char logout_fifo[LOG_NAME_LEN];

extern int config_thread_pool_size;
