#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hashmap.h"

static char *sa[]={
    "", "127637hga", "C Language", "ハッシュ関数", 
    "すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。",
    };
#define MAKE_KEY(key, n) sprintf(key, "%d_%x_%s",n,n,sa[n%(sizeof(sa)/sizeof(char*))]);
#define MAKE_DATA(n)     ((void*)0+n)

void test_iterate(hash_map_t *hash_map) {
    iterator_t *iterator = iterate_hash_map(hash_map);
    char *key  = NULL;
    void *data = NULL;
    int cnt = 0;
    while (next_iterate(iterator, &key, &data)) {
        assert(key);
        cnt++;
        key = NULL;
        data = NULL;
    }
    assert(cnt==num_hash_map(hash_map));
    end_iterate(iterator);
}

void test_hash_map(int size) {
    fprintf(stderr, "=== %s: size=%d * 10\n", __func__, size);

    int ret;
    char key[128];
    void *data;
    hash_map_t *hash_map = new_hash_map();
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
        ret = put_hash_map(hash_map, key, MAKE_DATA(i));
        assert(ret==1);
    }
    assert(num_hash_map(hash_map)==6*size);

    //既存削除
    for (int i=2*size; i<5*size; i++) {
        MAKE_KEY(key, i);
        ret = del_hash_map(hash_map, key);
        assert(ret==1);
    }
    dump_hash_map(__func__, hash_map, 0);

    //削除後追加
    for (int i=3*size; i<4*size; i++) {
        MAKE_KEY(key, i);
        ret = put_hash_map(hash_map, key, MAKE_DATA(i));
        assert(ret==1);
    }

    //上書き+新規追加
    for (int i=5*size; i<9*size; i++) {
        MAKE_KEY(key, i);
        ret = put_hash_map(hash_map, key, MAKE_DATA(i));
        assert(ret==(i<6*size?0:1));
    }
    dump_hash_map(__func__, hash_map, 0);

    //取得
    for (int i=0*size; i<2*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, &data);
        assert(ret==1);
        assert(data==MAKE_DATA(i));
        ret = get_hash_map(hash_map, key, NULL);
        assert(ret==1);
    }
    for (int i=2*size; i<3*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, &data);
        assert(ret==0);
    }
    for (int i=3*size; i<4*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, &data);
        assert(ret==1);
        assert(data==MAKE_DATA(i));
    }
    for (int i=4*size; i<5*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, &data);
        assert(ret==0);
    }
    for (int i=5*size; i<9*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, &data);
        assert(ret==1);
        assert(data==MAKE_DATA(i));
    }
    for (int i=9*size; i<10*size; i++) {
        MAKE_KEY(key, i);
        ret = get_hash_map(hash_map, key, &data);
        assert(ret==0);
    }

    //イテレート
    test_iterate(hash_map);

    free_hash_map(hash_map);
    free_hash_map(NULL);
}

void test_main(void) {
    fprintf(stderr, "===== hash_map_func=%d\n", hash_map_func);
    test_hash_map(10);
    test_hash_map(100);
    test_hash_map(1000);
    test_hash_map(10000);
    test_hash_map(100000);
    //test_hash_map(1000000);
}

void test_speed(int size) {
    int ret;
    char key[128];
    void *data;
    hash_map_t *hash_map = new_hash_map();

    //新規追加
    for (int i=0; i<size; i++) {
        MAKE_KEY(key, i);
        ret = put_hash_map(hash_map, key, MAKE_DATA(i));
        assert(ret==1);
    }
    dump_hash_map(__func__, hash_map, 0);
    assert(num_hash_map(hash_map)==size);
}

int main(int argc, char **argv) {
    //hash_map_func = HASH_MAP_FUNC_FNV_1A;
    //test_main();

    hash_map_func = HASH_MAP_FUNC_FNV_1;
    //test_main();

    hash_map_func = HASH_MAP_FUNC_DBG;
    //test_main();

    test_speed(1000*10000);
    return 0;
}
