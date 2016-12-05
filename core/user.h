#ifndef PF_USER_H
#define PF_USER_H

#define PEER_NAME_MAX 32
#define USER_SALT_LEN 32

// user data
typedef struct {
  unsigned char len;
  unsigned char salt[32];
  unsigned char self_destruct[32];
  unsigned char name[1]; // dummy length defined by len
} UserRecord;

#define USER_META_SIZE (sizeof(UserRecord)-1)

int new_user(UserRecord* rec, unsigned char* name, unsigned char name_len);
int get_user(UserRecord* userrec);

#endif // PF_USER_H
