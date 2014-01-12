#ifndef ecdho_h
#define ecdho_h
int start_ecdh(unsigned char* peer,
               unsigned char peer_len,
               unsigned char* pub,      // output
               unsigned char* keyid);   // output
int respond_ecdh(unsigned char* peer,
                 unsigned char peer_len,
                 unsigned char* pub,      // input/output
                 unsigned char* keyid);   // output
int finish_ecdh(unsigned char* peer,
                unsigned char peer_len,
                unsigned char* keyid,
                unsigned char* pub,
                unsigned char* seedid); // output

#endif // ecdho_h
