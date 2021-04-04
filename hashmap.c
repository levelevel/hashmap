#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "hashmap.h"

#define HASH_MAP_INIT_SIZE      16  //ハッシュテーブルの初期サイズ
#define HASH_MAP_MAX_CAPACITY   70  //使用率をこれ以下に抑える
#define HASH_MAP_LOW_CAPACITY   50  //リハッシュ時に使用率をこれ以上に保つ
#define HASH_MAP_GROW_FACTOR    2   //リハッシュ時に何倍にするか
#define TOMBSTONE ((void *)-1)      //削除済みエントリのキー値

//ハッシュエントリ
typedef struct {
    char *key;              //ハッシュキー（バイト列）
    int keylen;             //ハッシュキーの長さ
    void *data;             //ハッシュデータ（任意のポインタ）
} hash_entry_t;

//ハッシュマップ本体
typedef struct hash_map {
    int num;                //データ数
    int used;               //配列の使用数（データ数+TOMBSTONE数）
    int limit;              //配列の使用数の上限（used>limitになるとrehashする）
    int capacity;           //配列の確保サイズ
    hash_entry_t *buckets;  //配列
    hash_func_t hash_func;  //ハッシュ関数
} hash_map_t;

static void make_crc_table(void);
static void rehash(hash_map_t *hash_map);
static void set_entry(hash_entry_t *entry, const char *keykey, int len, void *data);
void fprint_key(FILE *fp, const unsigned char *key, int keylen);

//ハッシュマップを作成する。
hash_map_t *new_hash_map(size_t init_size, hash_func_t hash_func) {
    hash_map_t *hash_map = calloc(1, sizeof(hash_map_t));
    assert(hash_map);
    hash_map->capacity = init_size?init_size:HASH_MAP_INIT_SIZE;
    hash_map->limit = (hash_map->capacity * HASH_MAP_MAX_CAPACITY) / 100;
    hash_map->buckets = calloc(HASH_MAP_INIT_SIZE, sizeof(hash_entry_t));
    hash_map->hash_func = hash_func?hash_func:fnv1a_hash;
    assert(hash_map->buckets);
    return hash_map;
}

//ハッシュマップをフリーする。
void free_hash_map(hash_map_t *hash_map) {
    if (hash_map) {
        for (int i=0; i<hash_map->capacity; i++) {
            hash_entry_t *entry = &hash_map->buckets[i];
            if (entry->key && entry->key!=TOMBSTONE) free(entry->key);
        }
        free(hash_map->buckets);
    }
    free(hash_map);
}

//FNV-1A(default), FNV-1
//https://jonosuke.hatenadiary.org/entry/20100406/p1
//http://www.isthe.com/chongo/tech/comp/fnv/index.html
#define OFFSET_BASIS32      2166136261  //2^24 + 2^8 + 0x93
#define FNV_PRIME32         16777619
uint32_t fnv1a_hash(const char *s, int len) {
    uint32_t hash = OFFSET_BASIS32;
    for (int i=0; i<len; i++) {
        hash ^= s[i];
        hash *= FNV_PRIME32;
    }
    return hash;
}
uint32_t fnv1_hash(const char *s, int len) {
    uint32_t hash = OFFSET_BASIS32;
    for (int i=0; i<len; i++) {
        hash *= FNV_PRIME32;
        hash ^= s[i];
    }
    return hash;
}

//CRC32
//https://ja.wikipedia.org/wiki/%E5%B7%A1%E5%9B%9E%E5%86%97%E9%95%B7%E6%A4%9C%E6%9F%BB#CRC-32
static uint32_t crc_table[256];
static int crc_table_computed = 0;
static void make_crc_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        crc_table[i] = c;
    }
    crc_table_computed = 1;
}
uint32_t crc32_hash(const char *s, int len) {
    if (!crc_table_computed) make_crc_table();

    uint32_t c = 0xFFFFFFFF;
    for (int i=0; i<len; i++) {
        c = crc_table[(c ^ s[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}

//リハッシュ
static void rehash(hash_map_t *hash_map) {
    hash_map_t new_map = {};
    //dump_hash_map(__func__, hash_map, 0);

    //サイズを拡張した新しいハッシュマップを作成する
    new_map = *hash_map;
    if (new_map.num >= (hash_map->capacity * HASH_MAP_LOW_CAPACITY) / 100) {
        new_map.capacity = hash_map->capacity * HASH_MAP_GROW_FACTOR;
        new_map.limit = (new_map.capacity * HASH_MAP_MAX_CAPACITY) / 100;
    }
    new_map.buckets = calloc(new_map.capacity, sizeof(hash_entry_t));
    assert(new_map.buckets);

    //すべてのエントリをコピーする
    for (int i=0; i<hash_map->capacity; i++) {
        hash_entry_t *entry = &hash_map->buckets[i];
        if (entry->key && entry->key != TOMBSTONE) {
            int idx = hash_map->hash_func(entry->key, entry->keylen) % new_map.capacity;
            hash_entry_t *new_entry = &new_map.buckets[idx];
            while (new_entry->key) {
                if (++idx >= new_map.capacity) idx = 0;
                new_entry = &new_map.buckets[idx];
            }
            *new_entry = *entry;
        }
    }
    free(hash_map->buckets);
    *hash_map = new_map;
}

//エントリーにキーとデータを設定
static void set_entry(hash_entry_t *entry, const char *key, int keylen, void *data) {
    if (entry->key==TOMBSTONE) entry->key = NULL;
    entry->key = realloc(entry->key, keylen);
    assert(entry->key);
    entry->keylen = keylen;
    memcpy(entry->key, key, keylen);
    entry->data = data;
}

//データ書き込み
//すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。
//キーにNULLは指定できない。dataにNULLを指定できる。
int put_hash_map(hash_map_t *hash_map, const char *key, int keylen, void *data) {
    assert(hash_map);
    assert(key);
    if (hash_map->used > hash_map->limit) {
        rehash(hash_map);
    }
 
    int idx = hash_map->hash_func(key, keylen) % hash_map->capacity;
    hash_entry_t *entry_tombstome = NULL;   //新規データを書き込みできる削除済みアイテム
    for (;;) {
        hash_entry_t *entry = &hash_map->buckets[idx];
        if (entry->key==NULL) {
            if (entry_tombstome) {
                set_entry(entry_tombstome, key, keylen, data);
            } else {
                set_entry(entry, key, keylen, data);
                hash_map->used++;
            }
            hash_map->num++;
            return 1;
        } else if (entry->key==TOMBSTONE) {
            if (entry_tombstome==NULL) entry_tombstome = entry;
        } else if (entry->keylen==keylen && memcmp(entry->key, key, keylen)==0) {
            entry->data = data;
            return 0;
        }
        if (++idx >= hash_map->capacity) idx = 0;
    }
}

//キーに対応するデータの取得
//存在すればdataに値を設定して1を返す。dataにNULLを指定できる。
//存在しなければ0を返す。
int get_hash_map(hash_map_t *hash_map, const char *key, int keylen, void **data) {
    assert(hash_map);
    assert(key);
    int idx = hash_map->hash_func(key, keylen) % hash_map->capacity;

    for (;;) {
        hash_entry_t *entry = &hash_map->buckets[idx];
        if (entry->key==NULL) return 0;
        if (entry->keylen==keylen && memcmp(entry->key, key, keylen)==0) {
            if (data) *data = entry->data;
            return 1;
        }
        if (++idx >= hash_map->capacity) idx = 0;
    }
}

//データの削除
//キーに対応するデータを削除して1を返す。
//データが存在しない場合は0を返す。
int del_hash_map(hash_map_t *hash_map, const char *key, int keylen) {
    assert(hash_map);
    assert(key);
    int idx = hash_map->hash_func(key, keylen) % hash_map->capacity;

    for (;;) {
        hash_entry_t *entry = &hash_map->buckets[idx];
        if (entry->key==NULL) return 0;
        if (entry->keylen==keylen && memcmp(entry->key, key, keylen)==0) {
            free(entry->key);
            entry->key = TOMBSTONE;
            entry->keylen = 0;
            entry->data = NULL;
            hash_map->num--;
            return 1;
        }
        if (++idx >= hash_map->capacity) idx = 0;
    }
}

//ハッシュマップのデータ数
int num_hash_map(hash_map_t *hash_map) {
    return hash_map->num;
}

//イテレータ
typedef struct iterator {
    int         next_idx;
    hash_map_t  *hash_map;
} iterator_t;

//ハッシュマップのイテレータを生成する。
iterator_t *iterate_hash_map(hash_map_t *hash_map) {
    assert(hash_map);
    iterator_t *iterator = calloc(1, sizeof(iterator_t));
    assert(iterator);
    iterator->hash_map = hash_map;
    return iterator;
}

//次のデータをkey,dataに設定して1を返す。key、dataにNULL指定可能。
//次のデータがない場合は0を返す。
//イテレートする順番はランダム。
int next_iterate(iterator_t* iterator, char **key, int *keylen, void **data) {
    assert(iterator);
    hash_map_t *hash_map = iterator->hash_map;
    for (; iterator->next_idx < hash_map->capacity; iterator->next_idx++) {
        hash_entry_t *hash_entry = &hash_map->buckets[iterator->next_idx];
        if (hash_entry->key && hash_entry->key!=TOMBSTONE) {
            if (key)    *key    = hash_entry->key;
            if (keylen) *keylen = hash_entry->keylen;
            if (data)   *data   = hash_entry->data;
            iterator->next_idx++;
            return 1;
        }
    }
    return 0;
}

//ハッシュマップのイテレータを解放する。
void end_iterate(iterator_t* iterator) {
    free(iterator);
}

//ハッシュマップをダンプする
//level=0: 基本情報のみ
//level=1: 有効なキーすべて
//level=2: 削除済みも含める
void dump_hash_map(const char *str, hash_map_t *hash_map, int level) {
    int n_col = 0;
    long long len = 0;
    for (int i=0; i<hash_map->capacity; i++) {
        hash_entry_t *entry = &hash_map->buckets[i];
        if (entry->key && entry->key!=TOMBSTONE) {
            int idx = hash_map->hash_func(entry->key, entry->keylen) % hash_map->capacity;
            if (idx != i) n_col++;
            len += entry->keylen;
        }
    }
    fprintf(stderr, "= %s: num=%d,\tused=%d,\tcapacity=%d(%.1f%%),\tcollision=%.1f%%\tkey_len=%lld\n", 
        str, hash_map->num, hash_map->used, hash_map->capacity, hash_map->num*100.0/hash_map->capacity,
        n_col*100.0/hash_map->capacity, len/hash_map->num);
    if (level>0) {
        for (int i=0; i<hash_map->capacity; i++) {
            hash_entry_t *entry = &hash_map->buckets[i];
            if (!entry->key) continue;
            if (entry->key!=TOMBSTONE) {
                fprintf(stderr, "%02d: \"", i);
                fprint_key(stderr, entry->key, entry->keylen);
                fprintf(stderr, "\", %p\n", entry->data);
            } else if (level>1) {
                fprintf(stderr, "%02d: \"TOMBSTONE\", %p\n", i, entry->data);
            }
        }
    }
}
//キーをプリントする
void fprint_key(FILE *fp, const unsigned char *key, int keylen) {
    for (int i=0; i<keylen; i++) {
        if (isprint(key[i])) {
            fputc(key[i], fp);
        } else {
            //fprintf(fp, "\\%03o", key[i]);
            fprintf(fp, "\\x%02x", key[i]);
        }
    }
}
