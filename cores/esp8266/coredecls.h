
#ifndef __COREDECLS_H
#define __COREDECLS_H

#ifdef __cplusplus
extern "C" {
#endif

extern bool s_bootTimeSet;

// TODO: put declarations here, get rid of -Wno-implicit-function-declaration

void sntp_force_request (void);
void settimeofday_cb (void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif // __COREDECLS_H
