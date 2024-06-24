#include "config.h"
#include "msg.h"
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int regFd, loginFd, msgFd, logoutFd;

int priFd;

char username[USER_NAME_LEN];
char password[PASSWORD_LEN];
char fifo_name[USER_FIFO_NAME_LEN];

void sigHandler();

void init_client_conf() {
  FILE *fp;
  char line[LOG_NAME_LEN];
  char *key, *value;
  fp = fopen(config_file, "r");
  if (fp == NULL) {
    perror("Error opening config file");
    exit(1);
  }
  while (fgets(line, sizeof(line), fp) != NULL) {
    key = strtok(line, "=");
    value = strtok(NULL, "=");
    // remove newline character
    value[strcspn(value, "\n")] = 0;
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
    }
  }
  fclose(fp);
}

void init_global_fifo() {
  regFd = open(reg_fifo, O_WRONLY | O_NONBLOCK);
  if (regFd == -1) {
    perror("Error opening reg fifo");
    exit(1);
  }
  loginFd = open(login_fifo, O_WRONLY | O_NONBLOCK);
  if (loginFd == -1) {
    perror("Error opening login fifo");
    exit(1);
  }
  msgFd = open(msg_fifo, O_WRONLY | O_NONBLOCK);
  if (msgFd == -1) {
    perror("Error opening msg fifo");
    exit(1);
  }
  logoutFd = open(logout_fifo, O_WRONLY | O_NONBLOCK);
  if (logoutFd == -1) {
    perror("Error opening logout fifo");
    exit(1);
  }
}

void parse_cmd(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s -u username -p password\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // ./client -u username -p password
  // use getopt to parse command line arguments
  int opt;
  while ((opt = getopt(argc, argv, "u:p:")) != -1) {
    switch (opt) {
    case 'u':
      strcpy(username, optarg);
      break;
    case 'p':
      strcpy(password, optarg);
      break;
    default:
      fprintf(stderr, "Usage: %s -u username -p password\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}

void open_private_fifo() {
  sprintf(fifo_name, "%s_fifo", username);
  if (access(fifo_name, F_OK) == -1) {
    // create private fifo
    int res = mkfifo(fifo_name, 0666);
    if (res == -1) {
      perror("Error making private fifo");
      exit(1);
    }
  }

  priFd = open(fifo_name, O_RDONLY | O_NONBLOCK);
  if (priFd == -1) {
    perror("Error opening private fifo");
    exit(1);
  }

  if (open(fifo_name, O_WRONLY) == -1) {
    perror("Error opening private fifo");
    exit(1);
  }

  // reset fifo to block mode
  if (fcntl(priFd, F_SETFL, fcntl(priFd, F_GETFL) & ~O_NONBLOCK) == -1) {
    perror("Error setting private fifo to block mode");
    exit(1);
  }
}

void handle_input(char *buf) {
  // ex: send -u user1 user2 user3 -m "hello"
  // ex: logout login regist

  char *cmd = strtok(buf, " ");
  if (cmd == NULL) {
    printf("Invalid command\n");
    return;
  }

  if (strcmp(cmd, "send") == 0) {
    // extract target users
    // read -u
    char *opt = strtok(NULL, " ");
    if (opt == NULL || strcmp(opt, "-u") != 0) {
      printf("Invalid command\n");
      return;
    }
    char users[USER_NAME_LEN][USER_NAME_LEN];
    char *msg_buf = NULL;
    int user_count = 0;
    while (1) {
      char *user = strtok(NULL, " ");
      if (user == NULL) {
        printf("Invalid command\n");
        return;
      }
      if (strcmp(user, "-m") == 0) {
        msg_buf = user + 3;
        break;
      }
      strcpy(users[user_count++], user);
    }
    if (user_count == 0) {
      printf("Invalid command\n");
      return;
    }
    // extract message after -m
    if (msg_buf == NULL) {
      printf("Invalid command\n");
      return;
    }
    if (strlen(msg_buf) > MAX_MSG_LEN) {
      printf("Message too long\n");
      return;
    }

    for (int i = 0; i < user_count; i++) {
      SendMsg msg;
      memset(&msg, 0, sizeof(msg));
      INIT_MSG_HEADER(msg, SEND, username, password, fifo_name);
      strcpy(msg.target_user, users[i]);
      strcpy(msg.msg, msg_buf);
      int res = write(msgFd, &msg, sizeof(msg));
      if (res == -1) {
        perror("Error writing to msg fifo");
        exit(1);
      }
    }

  } else if (strcmp(cmd, "logout") == 0) {
    LogoutMsg msg;
    INIT_MSG_HEADER(msg, LOGOUT, username, password, fifo_name);
    int res = write(logoutFd, &msg, sizeof(msg));
    if (res == -1) {
      perror("Error writing to logout fifo");
      exit(1);
    }
    sigHandler();
  } else if (strcmp(cmd, "login") == 0) {
    LoginMsg msg;
    INIT_MSG_HEADER(msg, LOGIN, username, password, fifo_name);
    int res = write(loginFd, &msg, sizeof(msg));
    if (res == -1) {
      perror("Error writing to login fifo");
      exit(1);
    }

  } else if (strcmp(cmd, "regist") == 0) {
    RegistMsg msg;
    INIT_MSG_HEADER(msg, REGIST, username, password, fifo_name);
    int res = write(regFd, &msg, sizeof(msg));
    if (res == -1) {
      perror("Error writing to reg fifo");
      exit(1);
    }
  } else {
    printf("Invalid command\n");
  }
}

void mainloop() {
  char buf[MAX_MSG_LEN];
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(STDIN_FILENO, &rfds);
  FD_SET(priFd, &rfds);
  while (1) {
    fd_set tmp = rfds;
    printf("Cli > ");
    fflush(stdout);
    select(priFd + 1, &tmp, NULL, NULL, NULL);
    if (FD_ISSET(STDIN_FILENO, &tmp)) {
      // read from stdin
      fgets(buf, sizeof(buf), stdin);
      // remove newline character replace with null character
      buf[strcspn(buf, "\n")] = 0;
      // send to server
      handle_input(buf);
    }
    if (FD_ISSET(priFd, &tmp)) {
      // read from private fifo
      ReplyMsg msg;
      int res = read(priFd, &msg, sizeof(msg));
      if (res == -1) {
        perror("Error reading from private fifo");
        exit(1);
      }
      printf("%s\n", msg.msg);
    }
  }
}

void sigHandler() {
  close(regFd);
  close(loginFd);
  close(msgFd);
  close(logoutFd);
  close(priFd);
  exit(0);
}

int main(int argc, char *argv[]) {
  parse_cmd(argc, argv);
  init_client_conf();
  init_global_fifo();
  open_private_fifo();
  signal(SIGINT, sigHandler);
  mainloop();
  return 0;
}