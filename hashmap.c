#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hashmap.h"

#define HASH_MAP_INIT_SIZE      16  //ハッシュテーブルの初期サイズ
#define HASH_MAP_MAX_CAPACITY   70  //使用率をこれ以下に抑える
#define HASH_MAP_GROW_FACTOR    2   //リハッシュ時に何倍にするか
#define TOMBSTONE ((void *)-1)      //削除済みエントリのキー値

//ハッシュエントリ
typedef struct {
    char *key;              //ハッシュキー（文字列）
    void *data;             //ハッシュデータ（任意のポインタ）
} hash_entry_t;

//ハッシュマップ本体
typedef struct hash_map {
    int num;                //データ数
    int used;               //配列の使用数（データ数+TOMBSTONE数）
    int limit;              //配列の使用数の上限（used>limitになるとrehashする）
    int capacity;           //配列の確保サイズ
    hash_entry_t *buckets;  //配列
} hash_map_t;

static uint32_t fnv1a_hash(const char *str);
static uint32_t fnv1_hash(const char *str);
static uint32_t dbg_hash(const char *str);
static uint32_t calc_hash(const char *str);
static void rehash(hash_map_t *hash_map);
static int match(const char *key1, const char *key2);
static void set_entry(hash_entry_t *entry, const char *key, void *data);

//ハッシュマップを作成する。
hash_map_t *new_hash_map(void) {
    hash_map_t *hash_map = calloc(1, sizeof(hash_map_t));
    assert(hash_map);
    hash_map->capacity = HASH_MAP_INIT_SIZE;
    hash_map->limit = (hash_map->capacity * HASH_MAP_MAX_CAPACITY) / 100;
    hash_map->buckets = calloc(HASH_MAP_INIT_SIZE, sizeof(hash_entry_t));
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

//https://jonosuke.hatenadiary.org/entry/20100406/p1
//http://www.isthe.com/chongo/tech/comp/fnv/index.html
#define OFFSET_BASIS32      2166136261  //2^24 + 2^8 + 0x93
#define FNV_PRIME32         16777619
static uint32_t fnv1a_hash(const char *str) {
    uint32_t hash = OFFSET_BASIS32;
    for (const char *p=str; *p; p++) {
        hash ^= *p;
        hash *= FNV_PRIME32;
    }
    return hash;
}
static uint32_t fnv1_hash(const char *str) {
    uint32_t hash = OFFSET_BASIS32;
    for (const char *p=str; *p; p++) {
        hash *= FNV_PRIME32;
        hash ^= *p;
    }
    return hash;
}
static uint32_t dbg_hash(const char *str) {
    uint32_t hash = 0;
    for (const char *p=str; *p; p++) {
        hash *= 3;
        hash += *p;
    }
    return hash;
}
hash_map_func_type_t hash_map_func = HASH_MAP_FUNC_FNV_1A;
static uint32_t calc_hash(const char *str) {
    switch (hash_map_func) {
        case HASH_MAP_FUNC_FNV_1A:
            return fnv1a_hash(str);
        case HASH_MAP_FUNC_FNV_1:
            return fnv1_hash(str);
        case HASH_MAP_FUNC_DBG:
            return dbg_hash(str);
        default:
            assert(0);
    }
}

//リハッシュ
static void rehash(hash_map_t *hash_map) {
    hash_map_t new_map = {};
    //dump_hash_map(__func__, hash_map, 0);

    //サイズを拡張した新しいハッシュマップを作成する
    new_map.num  = hash_map->num;
    new_map.used = hash_map->used;
    new_map.capacity = hash_map->capacity * HASH_MAP_GROW_FACTOR;
    new_map.limit = (new_map.capacity * HASH_MAP_MAX_CAPACITY) / 100;
    new_map.buckets = calloc(new_map.capacity, sizeof(hash_entry_t));
    assert(new_map.buckets);

    //すべてのエントリをコピーする
    for (int i=0; i<hash_map->capacity; i++) {
        hash_entry_t *entry = &hash_map->buckets[i];
        if (entry->key && entry->key != TOMBSTONE) {
            int idx = calc_hash(entry->key) % new_map.capacity;
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

//キーの一致チェック
static int match(const char *key1, const char *key2) {
    return key1 != TOMBSTONE && strcmp(key1, key2)==0;
}

//エントリーにキーとデータを設定
static void set_entry(hash_entry_t *entry, const char *key, void *data) {
    if (entry->key==TOMBSTONE) entry->key = NULL;
    entry->key = realloc(entry->key, strlen(key)+1);
    strcpy(entry->key, key);
    entry->data = data;
}

//データ書き込み
//すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。
//キーにNULLは指定できない。dataにNULLを指定できる。
int put_hash_map(hash_map_t *hash_map, const char *key, void *data) {
    assert(hash_map);
    assert(key);
    if (hash_map->used > hash_map->limit) {
        rehash(hash_map);
    }
 
    int idx = calc_hash(key) % hash_map->capacity;
    hash_entry_t *entry_tombstome = NULL;   //新規データを書き込みできる削除済みアイテム
    for (;;) {
        hash_entry_t *entry = &hash_map->buckets[idx];
        if (entry->key==NULL) {
            if (entry_tombstome) {
                set_entry(entry_tombstome, key, data);
            } else {
                set_entry(entry, key, data);
                hash_map->used++;
            }
            hash_map->num++;
            return 1;
        } else if (entry->key==TOMBSTONE) {
            if (entry_tombstome==NULL) entry_tombstome = entry;
        } else if (match(entry->key, key)) {
            entry->data = data;
            return 0;
        }
        if (++idx >= hash_map->capacity) idx = 0;
    }
}

//キーに対応するデータの取得
//存在すればdataに値を設定して1を返す。dataにNULLを指定できる。
//存在しなければ0を返す。
int get_hash_map(hash_map_t *hash_map, const char *key, void **data) {
    assert(hash_map);
    assert(key);
    int idx = calc_hash(key) % hash_map->capacity;

    for (;;) {
        hash_entry_t *entry = &hash_map->buckets[idx];
        if (entry->key==NULL) return 0;
        if (match(entry->key, key)) {
            if (data) *data = entry->data;
            return 1;
        }
        if (++idx >= hash_map->capacity) idx = 0;
    }
}

//データの削除
//キーに対応するデータを削除して1を返す。
//データが存在しない場合は0を返す。
int del_hash_map(hash_map_t *hash_map, const char *key) {
    assert(hash_map);
    assert(key);
    int idx = calc_hash(key) % hash_map->capacity;

    for (;;) {
        hash_entry_t *entry = &hash_map->buckets[idx];
        if (entry->key==NULL) return 0;
        if (match(entry->key, key)) {
            free(entry->key);
            entry->key = TOMBSTONE;
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
int next_iterate(iterator_t* iterator, char **key, void **data) {
    assert(iterator);
    hash_map_t *hash_map = iterator->hash_map;
    for (; iterator->next_idx < hash_map->capacity; iterator->next_idx++) {
        hash_entry_t *hash_entry = &hash_map->buckets[iterator->next_idx];
        if (hash_entry->key && hash_entry->key!=TOMBSTONE) {
            if (key)  *key  = hash_entry->key;
            if (data) *data = hash_entry->data;
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
            int idx = calc_hash(entry->key) % hash_map->capacity;
            if (idx != i) n_col++;
            len += strlen(entry->key);
        }
    }
    fprintf(stderr, "= %s: num=%d,\tused=%d,\tcapacity=%d(%d%%),\tcollision=%d%%\tkey_len=%lld\n", 
        str, hash_map->num, hash_map->used, hash_map->capacity, hash_map->num*100/hash_map->capacity,
        n_col*100/hash_map->capacity, len/hash_map->num);
    if (level>0) {
        for (int i=0; i<hash_map->capacity; i++) {
            hash_entry_t *entry = &hash_map->buckets[i];
            if (!entry->key) continue;
            if (entry->key!=TOMBSTONE) {
                fprintf(stderr, "%02d: \"%s\", %p\n", i, entry->key, entry->data);
            } else if (level>1) {
                fprintf(stderr, "%02d: \"TOMBSTONE\", %p\n", i, entry->data);
            }
        }
    }
}
