#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "hashmap.h"

//CPU時間とメモリを表示
#include <sys/time.h>
#include <sys/resource.h>
void print_usage(struct rusage *ru0) {
#ifdef __linux__
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    time_t      u_sec  = ru.ru_utime.tv_sec  - ru0->ru_utime.tv_sec;
    time_t      s_sec  = ru.ru_stime.tv_sec  - ru0->ru_stime.tv_sec;
    suseconds_t u_usec = ru.ru_utime.tv_usec - ru0->ru_utime.tv_usec;
    suseconds_t s_usec = ru.ru_stime.tv_usec - ru0->ru_stime.tv_usec;
    long memdif = ru.ru_maxrss - ru0->ru_maxrss;
    if (u_usec<0) { u_sec--; u_usec += 1000*1000; }
    if (s_usec<0) { s_sec--; s_usec += 1000*1000; }
    printf("User:   %ld.%03ld sec\n", u_sec, u_usec/1000);
    printf("Sys:    %ld.%03ld sec\n", s_sec, s_usec/1000);
    printf("Memory: %ld(+%ld) MB\n", ru.ru_maxrss/1024, memdif/1024);
#endif
}

static char *sa[]={
    "", "127637hga", "C Language", "ハッシュ関数", 
    "すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。",
    };
#define MAKE_KEY(key, n) sprintf(key, "%d_%x_%s",n,n,sa[n%(sizeof(sa)/sizeof(char*))]);
#define MAKE_DATA(n)     ((void*)0+n)

void test_dump() {
    hash_map_t *hash_map = new_hash_map(0, NULL);
    put_hash_map(hash_map, "abc", 3, 0);
    put_hash_map(hash_map, "abc", 4, 0);
    put_hash_map(hash_map, "\0\t\r\n\xff", 5, 0);
    dump_hash_map(__func__, hash_map, 2);
    free_hash_map(hash_map);
}

void test_iterate(hash_map_t *hash_map) {
    iterator_t *iterator = iterate_hash_map(hash_map);
    char *key  = NULL;
    int keylen = 0;
    void *data = NULL;
    int cnt = 0;
    while (next_iterate(iterator, &key, &keylen, &data)) {
        assert(key);
        assert(keylen);
        cnt++;
        key = NULL;
        keylen = 0;
        data = NULL;
    }
    assert(cnt==num_hash_map(hash_map));
    end_iterate(iterator);
}

//size: データ数
void test_hash_map(int size, hash_func_t hash_func, const char *func_name) {
    fprintf(stderr, "=== %s: size=%d * 10, hash_map_func = %s\n",  __func__, size, func_name);

    int ret;
    char key[128];
    void *data;
    hash_map_t *hash_map = new_hash_map(0, hash_func);
    test_iterate(hash_map);

    // 0123456789
    // ++++++         put
    //   ---          del
    //    +           put
    //      ++++      put
    // @@ @ @@@@        
    // 0123456789

    //新規追加
    for (int i=0; i<6*size; i++) {
        MAKE_KEY(key, i);
        ret = put_hash_map(hash_map, key, strlen(key), MAKE_DATA(i));
        assert(ret==1);
    }
    assert(num_hash_map(hash_map)==6*size);

    //既存削除
    for (int i=2*size; i<5*size; i++) {
        MAKE_KEY(key, i);
        ret = del_hash_map(hash_map, key, strlen(key));
        assert(ret==1);
    }
    dump_hash_map(__func__, hash_map, 0);

    //削除後追加
    for (int i=3*size; i<4*size; i++) {
        MAKE_KEY(key, i);
        ret = put_hash_map(hash_map, key, strlen(key), MAKE_DATA(i));
        assert(ret==1);
    }

    //上書き+新規追加
    for (int i=5*size; i<9*size; i++) {
        MAKE_KEY(key, i);
        ret = put_hash_map(hash_map, key, strlen(key), MAKE_DATA(i));
        assert(ret==(i<6*size?0:1));
    }
    dump_hash_map(__func__, hash_map, 0);

    //取得
    for (int i=0*size; i<2*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, strlen(key), &data);
        assert(ret==1);
        assert(data==MAKE_DATA(i));
        ret = get_hash_map(hash_map, key, strlen(key), NULL);
        assert(ret==1);
    }
    for (int i=2*size; i<3*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, strlen(key), &data);
        assert(ret==0);
    }
    for (int i=3*size; i<4*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, strlen(key), &data);
        assert(ret==1);
        assert(data==MAKE_DATA(i));
    }
    for (int i=4*size; i<5*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, strlen(key), &data);
        assert(ret==0);
    }
    for (int i=5*size; i<9*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, strlen(key), &data);
        assert(ret==1);
        assert(data==MAKE_DATA(i));
    }
    for (int i=9*size; i<10*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, strlen(key), &data);
        assert(ret==0);
    }

    //イテレート
    test_iterate(hash_map);

    free_hash_map(hash_map);
    free_hash_map(NULL);
}

//Speed Test
//size: データ数
void test_speed(long size, hash_func_t hash_func, const char *func_name) {
    printf("== Speed Test: n=%ld\n", size);

    struct rusage ru0;
    getrusage(RUSAGE_SELF, &ru0);

    test_hash_map(size, hash_func, func_name);

    //結果表示
    print_usage(&ru0);
}

void test_func(void) {
    int size = 10000;
    test_hash_map(size, NULL, "FNV-1A(Default)");

    test_hash_map(size, fnv1_hash, "FNV1-1");

    test_hash_map(size, crc32_hash, "CRC32");

    printf("== Functional Test: OK\n");
}

int main(int argc, char **argv) {
    fprintf(stderr, "Start Test\n");

    test_dump();

    test_func();

    test_speed(100*10000, fnv1a_hash, "FNV-1A");
    test_speed(100*10000, crc32_hash, "CRC32");

    return 0;
}
