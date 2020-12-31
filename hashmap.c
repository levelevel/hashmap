#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hashmap.h"

#define HASH_MAP_INIT_SIZE      16  //ハッシュテーブルの初期サイズ
#define HASH_MAP_MAX_CAPACITY   50  //使用率をこれ以下に抑える
#define HASH_MAP_GROW_FACTOR    2   //リハッシュ時に何倍にするか
#define TOMBSTONE ((void *)-1)      //削除済みエントリのキー値

static uint32_t fnv1_hash(const char *str);
static uint32_t fnv1a_hash(const char *str);
static uint32_t calc_hash(const char *str);
static void rehash(HASH_MAP_t *hash_map);
static int match(const char *key1, const char *key2);
static void set_entry(HASH_ENTRY_t *entry, const char *key, void *data);

//ハッシュマップを作成する。
HASH_MAP_t *new_hash_map(void) {
    HASH_MAP_t *hash_map = calloc(1, sizeof(HASH_MAP_t));
    hash_map->capacity = HASH_MAP_INIT_SIZE;
    hash_map->array = calloc(HASH_MAP_INIT_SIZE, sizeof(HASH_ENTRY_t));
    return hash_map;
}

//ハッシュマップをフリーする。
void free_hash_map(HASH_MAP_t *hash_map) {
    if (hash_map) {
        for (int i=0; i<hash_map->capacity; i++) {
            HASH_ENTRY_t *entry = &hash_map->array[i];
            if (entry->key && entry->key!=TOMBSTONE) free(entry->key);
        }
    }
    free(hash_map->array);
    free(hash_map);
}

//https://jonosuke.hatenadiary.org/entry/20100406/p1
//http://www.isthe.com/chongo/tech/comp/fnv/index.html
#define FNV_OFFSET_BASIS32  2166136261  //2^24 + 2^8 + 0x93
#define FNV_PRIME32         16777619
static uint32_t fnv1_hash(const char *str) {
    uint32_t hash = FNV_OFFSET_BASIS32;
    for (const char *p=str; *p; p++) {
        hash *= FNV_PRIME32;
        hash ^= *p;
    }
    return hash;
}
static uint32_t fnv1a_hash(const char *str) {
    uint32_t hash = FNV_OFFSET_BASIS32;
    for (const char *p=str; *p; p++) {
        hash ^= *p;
        hash *= FNV_PRIME32;
    }
    return hash;
}
static uint32_t dbg_hash(const char *str) {
    uint32_t hash = 11;
    for (const char *p=str; *p; p++) {
        hash *= 11;
        hash += *p;
    }
    return hash;
}
HASH_MAP_FUNC_TYPE_t hash_map_func = HASH_MAP_FUNC_FNV_1A;
static uint32_t calc_hash(const char *str) {
    switch (hash_map_func) {
        case HASH_MAP_FUNC_FNV_1:
            return fnv1_hash(str);
        case HASH_MAP_FUNC_FNV_1A:
            return fnv1a_hash(str);
        case HASH_MAP_FUNC_DBG:
            return dbg_hash(str);
        default:
            assert(0);
    }
}

//リハッシュ
static void rehash(HASH_MAP_t *hash_map) {
    HASH_MAP_t new_map = {};
    //dump_hash_map(__func__, hash_map, 0);

    //サイズを拡張した新しいハッシュマップを作成する
    new_map.capacity = hash_map->capacity * HASH_MAP_GROW_FACTOR;
    new_map.array = calloc(new_map.capacity, sizeof(HASH_ENTRY_t));

    //すべてのエントリをコピーする
    for (int i=0; i<hash_map->capacity; i++) {
        HASH_ENTRY_t *entry = &hash_map->array[i];
        if (entry->key && entry->key != TOMBSTONE) {
            put_hash_map(&new_map, entry->key, entry->data);
        }
    }
    assert(hash_map->num==new_map.num);
    free(hash_map->array);
    *hash_map = new_map;
}

//キーの一致チェック
static int match(const char *key1, const char *key2) {
    assert(key1);
    assert(key2);
    return key1 != TOMBSTONE &&
        strcmp(key1, key2)==0;
}

//エントリーにキーとデータを設定
static void set_entry(HASH_ENTRY_t *entry, const char *key, void *data) {
    char *buf = malloc(strlen(key)+1);
    strcpy(buf, key);
    if (entry->key != TOMBSTONE) free(entry->key);
    entry->key  = buf;
    entry->data = data;
}

//データ書き込み
//すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。
//キーにNULLは指定できない。dataにNULLを指定できる。
int put_hash_map(HASH_MAP_t *hash_map, const char *key, void *data) {
    assert(hash_map);
    assert(key);
    if ((hash_map->used * 100) / hash_map->capacity > HASH_MAP_MAX_CAPACITY ) {
        rehash(hash_map);
    }
 
    uint32_t hash = calc_hash(key);

    for (int i=0; i<hash_map->capacity; i++) {
        int idx = (hash+i) % hash_map->capacity;
        HASH_ENTRY_t *entry = &hash_map->array[idx];
        if (entry->key==NULL) {
            set_entry(entry, key, data);
            hash_map->num++;
            hash_map->used++;
            return 1;
        } else if (match(entry->key, key)) {
            set_entry(entry, key, data);
            return 0;
        }
    }
    assert(0);  //ここに到達することはない
}

//キーに対応するデータの取得
//存在すればdataに値を設定して1を返す。
//存在しなければ0を返す。
int get_hash_map(HASH_MAP_t *hash_map, const char *key, void **data) {
    assert(hash_map);
    assert(key);
    uint32_t hash = calc_hash(key);

    for (int i=0; i<hash_map->capacity; i++) {
        int idx = (hash+i) % hash_map->capacity;
        HASH_ENTRY_t *entry = &hash_map->array[idx];
        if (entry->key==NULL) return 0;
        if (match(entry->key, key)) {
            *data = entry->data;
            return 1;
        }
    }
    assert(0);  //ここに到達することはない
}

//データの削除
//キーに対応するデータを削除して1を返す。
//データが存在しない場合は0を返す。
int del_hash_map(HASH_MAP_t *hash_map, const char *key) {
    assert(hash_map);
    assert(key);
    uint32_t hash = calc_hash(key);

    for (int i=0; i<hash_map->capacity; i++) {
        HASH_ENTRY_t *entry = &hash_map->array[(hash+i) % hash_map->capacity];
        if (entry->key==NULL) return 0;
        if (match(entry->key, key)) {
            free(entry->key);
            entry->key = TOMBSTONE;
            entry->data = NULL;
            hash_map->num--;
            return 1;
        }
    }
    assert(0);  //ここに到達することはない
}

//ハッシュマップをダンプする
//level=0: 基本情報のみ
//level=1: 有効なキーすべて
//level=2: 削除済みも含める
void dump_hash_map(const char *str, HASH_MAP_t *hash_map, int level) {
    int n_col = 0;
    fprintf(stderr, "= %s: num=%d,\tused=%d,\tcapacity=%d(%d%%)", 
        str, hash_map->num, hash_map->used, hash_map->capacity, hash_map->num*100/hash_map->capacity);
    for (int i=0; i<hash_map->capacity; i++) {
        HASH_ENTRY_t *entry = &hash_map->array[i];
        if (entry->key && entry->key!=TOMBSTONE) {
            uint32_t hash = calc_hash(entry->key);
            int idx = hash % hash_map->capacity;
            if (idx != i) n_col++;
        }
    }
    fprintf(stderr, ",\tcollision rate=%d%%", n_col*100/hash_map->capacity);
    fputs("\n", stderr);
    if (level>0) {
        for (int i=0; i<hash_map->capacity; i++) {
            HASH_ENTRY_t *entry = &hash_map->array[i];
            if (!entry->key) continue;
            if (entry->key!=TOMBSTONE) {
                fprintf(stderr, "%02d: \"%s\", %p\n", i, entry->key, entry->data);
            } else if (level>1) {
                fprintf(stderr, "%02d: \"TOMBSTONE\", %p\n", i, entry->data);
            }
        }
    }
}
