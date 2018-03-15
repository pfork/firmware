#ifndef sphinx_ops_h
#define sphinx_ops_h
#include <stdint.h>

int pf_sphinx_respond(const uint8_t *id, const uint8_t *challenge);
int pf_sphinx_create(const uint8_t *id, const uint8_t *challenge);
int pf_sphinx_change(const uint8_t *id, const uint8_t *challenge);
int pf_sphinx_commit(const uint8_t *id);
int pf_sphinx_delete(const uint8_t *id);

#endif // sphinx_ops_h
