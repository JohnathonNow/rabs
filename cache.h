#ifndef CACHE_H
#define CACHE_H

#include <time.h>
#include "sha256.h"
#include "targetset.h"
#include "minilang.h"
#include "target.h"

void cache_open(const char *RootPath);
void cache_close();

void cache_hash_get(target_t *Target, int *LastUpdated, int *LastChecked, time_t *FileTime, BYTE Digest[SHA256_BLOCK_SIZE]);
void cache_hash_set(target_t *Target, time_t FileTime);
void cache_build_hash_get(target_t *Target, BYTE Hash[SHA256_BLOCK_SIZE]);
void cache_build_hash_set(target_t *Target, BYTE BuildHash[SHA256_BLOCK_SIZE]);
void cache_last_check_set(target_t *Target, time_t FileTime);

targetset_t *cache_depends_get(target_t *Target);
void cache_depends_set(target_t *Target, targetset_t *Scans);

targetset_t *cache_scan_get(target_t *Target);
void cache_scan_set(target_t *Target, targetset_t *Scans);

ml_value_t *cache_expr_get(target_t *Target);
void cache_expr_set(target_t *Target, ml_value_t *Value);

extern int CurrentVersion;
void cache_bump_version();

#endif
