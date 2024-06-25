#pragma once
#define USER_NAME_LEN 64
#define PASSWORD_LEN 64
#define USER_FIFO_NAME_LEN 128
#define MAX_MSG_LEN 256
#define MAX_REPLY_MSG_LEN 256 * 2

enum MsgType { REGIST = 1, LOGIN, LOGOUT, SEND, Reply };
struct Msg {
  enum MsgType type;
  char username[USER_NAME_LEN];
  char password[PASSWORD_LEN];
  char fifo_name[USER_FIFO_NAME_LEN];
};

#define INIT_MSG_HEADER(MSG, TYPE, USERNAME, PASSWORD, FIFO_NAME) \
  do {                                                            \
    (MSG).msg_header.type = TYPE;                                 \
    strcpy((MSG).msg_header.username, USERNAME);                  \
    strcpy((MSG).msg_header.password, PASSWORD);                  \
    strcpy((MSG).msg_header.fifo_name, FIFO_NAME);                \
  } while (0)

typedef struct RegistMsg {
  struct Msg msg_header;
} RegistMsg;

typedef struct LoginMsg {
  struct Msg msg_header;
} LoginMsg;

typedef struct LogoutMsg {
  struct Msg msg_header;
} LogoutMsg;

typedef struct SendMsg {
  struct Msg msg_header;
  char target_user[USER_NAME_LEN];
  char msg[MAX_MSG_LEN];
} SendMsg;

typedef struct ReplyMsg {
  struct Msg msg_header;
  char msg[MAX_REPLY_MSG_LEN];
} ReplyMsg;