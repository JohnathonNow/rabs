#ifndef TARGETCACHE_H
#define TARGETCACHE_H

typedef struct target_t target_t;

void targetcache_init();
struct target_t **targetcache_lookup(const char *Id);

#endif
