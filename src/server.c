#include "config.h"
#include "list.h"
#include "log.h"
#include "msg.h"
#include "timestamp.h"
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_NAME_LEN 32
#define VERSION_LEN 32
int RegFifoFd, LoginFifoFd, MsgFifoFd, LogoutFifoFd;
char serverName[SERVER_NAME_LEN];
char version[VERSION_LEN] = "1.0";

ListNode *user_list;
pthread_rwlock_t user_list_lock;

typedef struct UserInfo {
  char name[USER_NAME_LEN];
  char password[PASSWORD_LEN];
  char fifo_name[USER_FIFO_NAME_LEN];
  char fail_log_file_name[LOG_NAME_LEN];
  char log_file_name[LOG_NAME_LEN];
  FILE *log_file;
  FILE *fail_log_file;
  int msg_fifo_fd;
  ListNode list;
  bool is_online;
} UserInfo;

void OpenUserLogFileAndFiFo(UserInfo *user) {
  char log_file_name[LOG_NAME_LEN];
  char fail_log_file_name[LOG_NAME_LEN];
  strcpy(log_file_name, user_log_dir);
  strcat(log_file_name, user->name);
  strcat(log_file_name, ".log");
  strcpy(user->log_file_name, log_file_name);
  user->log_file = fopen(log_file_name, "a");
  if (user->log_file == NULL) {
    perror("Error opening user log file");
    exit(1);
  }
  strcpy(fail_log_file_name, user_log_dir);
  strcat(fail_log_file_name, user->name);
  strcat(fail_log_file_name, ".fail.log");
  strcpy(user->fail_log_file_name, fail_log_file_name);
  user->fail_log_file = fopen(fail_log_file_name, "a+");
  if (user->fail_log_file == NULL) {
    perror("Error opening user fail log file");
    exit(1);
  }

  user->msg_fifo_fd = open(user->fifo_name, O_WRONLY | O_NONBLOCK);
  if (user->msg_fifo_fd == -1) {
    perror("Error opening user msg FIFO");
    exit(1);
  }
}

UserInfo *CreateUser(char *name, char *password, char *fifo_name) {
  pthread_rwlock_wrlock(&user_list_lock);
  UserInfo *user = (UserInfo *)malloc(sizeof(UserInfo));
  assert(user != NULL);
  strcpy(user->name, name);
  strcpy(user->password, password);
  strcpy(user->fifo_name, fifo_name);
  user->log_file = NULL;
  user->fail_log_file = NULL;
  list_add(&user->list, user_list);

  OpenUserLogFileAndFiFo(user);
  user->is_online = true;
  pthread_rwlock_unlock(&user_list_lock);
  return user;
}

void InfoAllUserStatus() {
  ListNode *node;
  char buf[1024];
  memset(buf, 0, sizeof(buf));
  int count = 0;
  list_for_each(node, user_list) {
    UserInfo *user = list_entry(node, UserInfo, list);
    if (user->is_online) {
      count++;
      // append at last
      strcat(buf, user->name);
      strcat(buf, " ");
    }
  }

  // append user count
  sprintf(&buf[strlen(buf)], "Total %d users online.", count);
  ReplyMsg reply_msg;
  memset(&reply_msg, 0, sizeof(reply_msg));
  INIT_MSG_HEADER(reply_msg, Reply, serverName, "", "");
  strcpy(reply_msg.msg, buf);

  // send to all online users
  list_for_each(node, user_list) {
    UserInfo *user = list_entry(node, UserInfo, list);
    if (user->is_online) {
      int nbytes = write(user->msg_fifo_fd, &reply_msg, sizeof(reply_msg));
      if (nbytes == -1) {
        perror("Error writing to user FIFO");
        exit(1);
      }
    }
  }
}

UserInfo *FindUser(char *name) {
  pthread_rwlock_rdlock(&user_list_lock);
  ListNode *node;
  list_for_each(node, user_list) {
    UserInfo *user = list_entry(node, UserInfo, list);
    if (strcmp(user->name, name) == 0) {
      pthread_rwlock_unlock(&user_list_lock);
      return user;
    }
  }
  pthread_rwlock_unlock(&user_list_lock);
  return NULL;
}

static void initConfig();

static void initConfig() {
  // read config file ex:
  // REG_FIFO=ycy+register
  // LOGIN_FIFO=ycy+login
  // MSG_FIFO=ycy+sendmsg
  // LOGOUT_FIFO=ycy+logout
  // LOGFILES_SERVER=/var/log/chat-logs/server/
  // LOGFILES_USERS=/var/log/chat-logs/users/
  FILE *fp;
  char line[LOG_NAME_LEN];
  char *key, *value;
  fp = fopen(config_file, "r");
  if (fp == NULL) {
    perror("Error opening config file");
    exit(1);
  }
  while (fgets(line, sizeof(line), fp) != NULL) {
    // remove newline character
    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';
    key = strtok(line, "=");
    value = strtok(NULL, "=");
    assert(key != NULL && value != NULL);
    // assert value size
    assert(strlen(value) < LOG_NAME_LEN);
    if (strcmp(key, "REG_FIFO") == 0) {
      strcpy(reg_fifo, value);
    } else if (strcmp(key, "LOGIN_FIFO") == 0) {
      strcpy(login_fifo, value);
    } else if (strcmp(key, "MSG_FIFO") == 0) {
      strcpy(msg_fifo, value);
    } else if (strcmp(key, "LOGOUT_FIFO") == 0) {
      strcpy(logout_fifo, value);
    } else if (strcmp(key, "LOGFILES_SERVER") == 0) {
      strcpy(server_log_dir, value);
      assert(strlen(server_log_dir) + strlen("server.log") < LOG_NAME_LEN);
      strcpy(server_log_file, server_log_dir);
      strcat(server_log_file, "server.log");

    } else if (strcmp(key, "LOGFILES_USERS") == 0) {
      strcpy(user_log_dir, value);
    } else if (strcmp(key, "SERVER_NAME") == 0) {
      strcpy(serverName, value);
    } else if (strcmp(key, "SERVER_VERSION") == 0) {
      strcpy(version, value);
    }
  }
  fclose(fp);
}

void initLogger() {
  init_server_log(server_log_file);
  assert(server_log_file != NULL);
}

void RegistHandler(struct Msg *msg) {
  RegistMsg *regist_msg = (RegistMsg *)msg;
  UserInfo *user = FindUser(regist_msg->msg_header.username);
  if (user != NULL) {
    ReplyMsg reply_msg;
    memset(&reply_msg, 0, sizeof(reply_msg));
    INIT_MSG_HEADER(reply_msg, Reply, serverName, "", "");
    strcpy(reply_msg.msg, "User already exists");
    int nbytes = write(user->msg_fifo_fd, &reply_msg, sizeof(reply_msg));
    if (nbytes == -1) {
      LOG("Error writing to user FIFO\n");
      return;
    }
    return;
  }
  user = CreateUser(msg->username, msg->password, msg->fifo_name);
  LOG_FILE(user->log_file, "User %s registered\n",
           regist_msg->msg_header.username);
  InfoAllUserStatus();
}

void reTryMessageFromUserFailLog(UserInfo *user) {
  // read user fail log file
  // for each line, send message to target user
  char line[1024];
  while (fgets(line, sizeof(line), user->fail_log_file) != NULL) {
    char *from_user = strtok(line, " ");
    char *target_user = strtok(NULL, " ");
    char *msg = strtok(NULL, " ");
    char *time_str = strtok(NULL, " ");
    // send message to target user
    ReplyMsg reply_msg;
    memset(&reply_msg, 0, sizeof(reply_msg));
    INIT_MSG_HEADER(reply_msg, Reply, from_user, user->password,
                    user->fifo_name);
    strcpy(reply_msg.msg, msg);
    // concat user name and message
    strcat(reply_msg.msg, " from ");
    strcat(reply_msg.msg, from_user);
    strcat(reply_msg.msg, " at ");
    strcat(reply_msg.msg, time_str);
    int nbytes = write(user->msg_fifo_fd, &reply_msg, sizeof(reply_msg));
    if (nbytes == -1) {
      perror("Error writing to user FIFO");
      exit(1);
    }
  }

  // clear fail log file
  fclose(user->fail_log_file);
  user->fail_log_file = fopen(user->fail_log_file_name, "w");
  if (user->fail_log_file == NULL) {
    perror("Error opening user fail log file");
    exit(1);
  }
  fclose(user->fail_log_file);

  // reset to read and write mode
  user->fail_log_file = fopen(user->fail_log_file_name, "a+");
}

void sendBackErrorMsg(struct Msg *msg) {
  char *user_fifo_name = msg->fifo_name;
  if (user_fifo_name == NULL) {
    return;
  }
  int fifo_fd = open(user_fifo_name, O_WRONLY | O_NONBLOCK);
  if (fifo_fd == -1) {
    return;
  }
  ReplyMsg reply_msg;
  memset(&reply_msg, 0, sizeof(reply_msg));
  INIT_MSG_HEADER(reply_msg, Reply, serverName, "", "");
  strcpy(reply_msg.msg, "Failed");
  int nbytes = write(fifo_fd, &reply_msg, sizeof(reply_msg));
  if (nbytes == -1) {
    close(fifo_fd);
    return;
  }
  close(fifo_fd);
}

void LoginHandler(struct Msg *msg) {
  LoginMsg *login_msg = (LoginMsg *)msg;
  UserInfo *user = FindUser(login_msg->msg_header.username);

  if (user == NULL) {
    LOG("User %s not exists\n", login_msg->msg_header.username);
    sendBackErrorMsg(msg);
    return;
  }
  if (strcmp(user->password, login_msg->msg_header.password) != 0) {
    LOG("User %s login failed\n", login_msg->msg_header.username);
    sendBackErrorMsg(msg);
    return;
  }
  if (user->is_online) {
    LOG("User %s already online\n", login_msg->msg_header.username);
    sendBackErrorMsg(msg);
    return;
  }

  OpenUserLogFileAndFiFo(user);
  user->is_online = true;
  reTryMessageFromUserFailLog(user);
  LOG_FILE(user->log_file, "User %s login success\n",
           login_msg->msg_header.username);
  InfoAllUserStatus();
}

void LogOutHandler(struct Msg *msg) {
  LogoutMsg *logout_msg = (LogoutMsg *)msg;
  UserInfo *user = FindUser(logout_msg->msg_header.username);
  if (user == NULL) {
    LOG("User %s not exists\n", logout_msg->msg_header.username);
    return;
  }
  if (strncmp(user->password, logout_msg->msg_header.password, PASSWORD_LEN) !=
      0) {
    LOG("User %s logout failed\n", logout_msg->msg_header.username);
    return;
  }
  user->is_online = false;
  LOG_FILE(user->log_file, "User %s logout success\n",
           logout_msg->msg_header.username);
  // send logout success message to user TODO:
  InfoAllUserStatus();
}

void saveMessageToUserFailLog(UserInfo *from_user, UserInfo *target_user,
                              char *msg) {

  // save message to target user fail log file
  // format: sender receiver message time
  char time_str[32];
  getTimeStamp(time_str, sizeof(time_str));
  fprintf(target_user->fail_log_file, "%s %s %s %s\n", from_user->name,
          target_user->name, msg, time_str);
  fflush(target_user->fail_log_file);
}

void SendMsgHandler(struct Msg *msg) {
  SendMsg *send_msg = (SendMsg *)msg;
  UserInfo *user = FindUser(send_msg->msg_header.username);
  if (user == NULL) {
    LOG("User %s not exists\n", send_msg->msg_header.username);
    return;
  }
  if (strncmp(user->password, send_msg->msg_header.password, PASSWORD_LEN) !=
      0) {
    LOG("User %s send message failed\n", send_msg->msg_header.username);
    return;
  }
  if (!user->is_online) {
    return;
  }

  UserInfo *target_user = FindUser(send_msg->target_user);
  if (target_user == NULL) {
    LOG("Target user %s not exists\n", send_msg->target_user);
    return;
  }
  if (!target_user->is_online) {
    LOG_FILE(user->log_file, "Target user %s not online\n",
             send_msg->target_user);
    saveMessageToUserFailLog(user, target_user, send_msg->msg);
    return;
  }
  LOG_FILE(user->log_file, "User %s send message to %s\n",
           send_msg->msg_header.username, send_msg->target_user);
  // send message to target user TODO:
  // append user name to message
  ReplyMsg reply_msg;
  memset(&reply_msg, 0, sizeof(reply_msg));
  reply_msg.msg_header.type = Reply;
  strcpy(reply_msg.msg_header.username, send_msg->msg_header.username);
  strcpy(reply_msg.msg, send_msg->msg);
  // concat user name and message
  strcat(reply_msg.msg, " from ");
  strcat(reply_msg.msg, send_msg->msg_header.username);

  char time_str[32];
  getTimeStamp(time_str, sizeof(time_str));
  strcat(reply_msg.msg, " at ");
  strcat(reply_msg.msg, time_str);
  int nbytes = write(target_user->msg_fifo_fd, &reply_msg, sizeof(reply_msg));
  if (nbytes == -1) {
    perror("Error writing to user FIFO");
    exit(1);
  }
}

void initFIFO() {
  // create FIFOs
  if (access(reg_fifo, F_OK) == -1) {
    if (mkfifo(reg_fifo, 0666) == -1) {
      perror("Error creating REG_FIFO");
      exit(1);
    }
  }

  if (access(login_fifo, F_OK) == -1) {
    if (mkfifo(login_fifo, 0666) == -1) {
      perror("Error creating LOGIN_FIFO");
      exit(1);
    }
  }

  if (access(msg_fifo, F_OK) == -1) {
    if (mkfifo(msg_fifo, 0666) == -1) {
      perror("Error creating MSG_FIFO");
      exit(1);
    }
  }

  if (access(logout_fifo, F_OK) == -1) {
    if (mkfifo(logout_fifo, 0666) == -1) {
      perror("Error creating LOGOUT_FIFO");
      exit(1);
    }
  }

  // open FIFOs for reading
  RegFifoFd = open(reg_fifo, O_RDONLY | O_NONBLOCK);
  if (RegFifoFd == -1) {
    perror("Error opening REG_FIFO");
    exit(1);
  }
  LoginFifoFd = open(login_fifo, O_RDONLY | O_NONBLOCK);
  if (LoginFifoFd == -1) {
    perror("Error opening LOGIN_FIFO");
    exit(1);
  }
  MsgFifoFd = open(msg_fifo, O_RDONLY | O_NONBLOCK);
  if (MsgFifoFd == -1) {
    perror("Error opening MSG_FIFO");
    exit(1);
  }
  LogoutFifoFd = open(logout_fifo, O_RDONLY | O_NONBLOCK);
  if (LogoutFifoFd == -1) {
    perror("Error opening LOGOUT_FIFO");
    exit(1);
  }

  // open FIFOs for writing
  if (open(reg_fifo, O_WRONLY) == -1) {
    perror("Error opening REG_FIFO for writing");
    exit(1);
  }
  if (open(login_fifo, O_WRONLY) == -1) {
    perror("Error opening LOGIN_FIFO for writing");
    exit(1);
  }
  if (open(msg_fifo, O_WRONLY) == -1) {
    perror("Error opening MSG_FIFO for writing");
    exit(1);
  }
  if (open(logout_fifo, O_WRONLY) == -1) {
    perror("Error opening LOGOUT_FIFO for writing");
    exit(1);
  }

  // reset to blocking mode
  if (fcntl(RegFifoFd, F_SETFL, fcntl(RegFifoFd, F_GETFL) & ~O_NONBLOCK) ==
      -1) {
    perror("Error setting REG_FIFO to blocking mode");
    exit(1);
  }
  if (fcntl(LoginFifoFd, F_SETFL, fcntl(LoginFifoFd, F_GETFL) & ~O_NONBLOCK) ==
      -1) {
    perror("Error setting LOGIN_FIFO to blocking mode");
    exit(1);
  }
  if (fcntl(MsgFifoFd, F_SETFL, fcntl(MsgFifoFd, F_GETFL) & ~O_NONBLOCK) ==
      -1) {
    perror("Error setting MSG_FIFO to blocking mode");
    exit(1);
  }
  if (fcntl(LogoutFifoFd, F_SETFL,
            fcntl(LogoutFifoFd, F_GETFL) & ~O_NONBLOCK) == -1) {
    perror("Error setting LOGOUT_FIFO to blocking mode");
    exit(1);
  }

  LOG("Server started\n");
  LOG("Server name: %s\n", serverName);
  LOG("Version: %s\n", version);
}

void destructer() {
  LOG("Server exit\n");
  close(RegFifoFd);
  close(LoginFifoFd);
  close(MsgFifoFd);
  close(LogoutFifoFd);
  unlink(reg_fifo);
  unlink(login_fifo);
  unlink(msg_fifo);
  unlink(logout_fifo);
  fclose(server_log);
  // set log file 000 mode
  chmod(server_log_file, 0);
  for (ListNode *node = user_list->next; node != user_list; node = node->next) {
    UserInfo *user = list_entry(node, UserInfo, list);
    fclose(user->log_file);
    fclose(user->fail_log_file);
    chmod(user->log_file_name, 0);
    chmod(user->fail_log_file_name, 0);
    close(user->msg_fifo_fd);
    free(user);
  }
}

void initUserList() {
  user_list = malloc(sizeof(ListNode));
  INIT_LIST_HEAD(user_list);
  pthread_rwlock_init(&user_list_lock, NULL);
}

void startListening() {
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(RegFifoFd, &read_fds);
  FD_SET(LoginFifoFd, &read_fds);
  FD_SET(MsgFifoFd, &read_fds);
  FD_SET(LogoutFifoFd, &read_fds);

  ListNode *thread_list = malloc(sizeof(ListNode));
  INIT_LIST_HEAD(thread_list);

  while (true) {
    fd_set tmp_fds = read_fds;
    int nbytes = select(LogoutFifoFd + 1, &tmp_fds, NULL, NULL, NULL);
    if (nbytes == -1) {
      perror("Error selecting");
      exit(1);
    }
    if (FD_ISSET(RegFifoFd, &tmp_fds)) {
      RegistMsg msg;
      int nbytes = read(RegFifoFd, &msg, sizeof(msg));
      if (nbytes == -1) {
        perror("Error reading from REG_FIFO");
        exit(1);
      }
      // create new thread to handle message
      pthread_t tid;
      pthread_create(&tid, NULL, (void *)RegistHandler, (void *)&msg);
      pthread_detach(tid);
    }
    if (FD_ISSET(LoginFifoFd, &tmp_fds)) {
      LoginMsg msg;
      int nbytes = read(LoginFifoFd, &msg, sizeof(msg));
      if (nbytes == -1) {
        perror("Error reading from LOGIN_FIFO");
        exit(1);
      }
      pthread_t tid;
      pthread_create(&tid, NULL, (void *)LoginHandler, (void *)&msg);
      pthread_detach(tid);
    }
    if (FD_ISSET(MsgFifoFd, &tmp_fds)) {
      SendMsg msg;
      int nbytes = read(MsgFifoFd, &msg, sizeof(msg));
      if (nbytes == -1) {
        perror("Error reading from MSG_FIFO");
        exit(1);
      }
      pthread_t tid;
      pthread_create(&tid, NULL, (void *)SendMsgHandler, (void *)&msg);
      pthread_detach(tid);
    }
    if (FD_ISSET(LogoutFifoFd, &tmp_fds)) {
      LogoutMsg msg;
      int nbytes = read(LogoutFifoFd, &msg, sizeof(msg));
      if (nbytes == -1) {
        perror("Error reading from LOGOUT_FIFO");
        exit(1);
      }
      pthread_t tid;
      pthread_create(&tid, NULL, (void *)LogOutHandler, (void *)&msg);
      pthread_detach(tid);
    }
  }
}

void sigHandler() {
  destructer();
  exit(0);
}

int main() {
  initConfig();
  initLogger();
  initUserList();
  // demon
  initFIFO();
  // set signal handler
  signal(SIGTERM, sigHandler);
  daemon(1, 0);
  startListening();
  destructer();
  return 0;
}