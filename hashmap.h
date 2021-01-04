//ハッシュマップ（オープンアドレス法）
//
//## 機能
//- キー：文字列、データ：任意のポインタ
//- 追加・削除・検索・イテレート
//- 1エントリ当たり1回のmalloc（キーの保存）
//- ハッシュ関数は[FNV-1aまたはFNV-1](http://www.isthe.com/chongo/tech/comp/fnv/index.html)
//
//## 参考
//- https://jonosuke.hatenadiary.org/entry/20100406/p1
//- https://ja.wikipedia.org/wiki/%E3%83%8F%E3%83%83%E3%82%B7%E3%83%A5%E3%83%86%E3%83%BC%E3%83%96%E3%83%AB
//- https://github.com/rui314/chibicc

//ハッシュ関数の種類
typedef enum {
    HASH_MAP_FUNC_FNV_1A,   //FNV-1a hash (Default)
    HASH_MAP_FUNC_FNV_1,    //FNV-1 hash
    HASH_MAP_FUNC_DBG,      //DEBUG
} hash_map_func_type_t;

//ハッシュ関数の種類を設定するグローバル変数
extern hash_map_func_type_t hash_map_func;

//ハッシュマップ
typedef struct hash_map hash_map_t;

//ハッシュマップを作成する。
hash_map_t *new_hash_map(void);

//ハッシュマップをフリーする。
void free_hash_map(hash_map_t *hash_map);

//データ書き込み
//すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。
//キーにNULLは指定できない。dataにNULLを指定できる。
int put_hash_map(hash_map_t *hash_map, const char *key, void *data);

//キーに対応するデータの取得
//存在すればdataに値を設定して1を返す。dataにNULLを指定できる。
//存在しなければ0を返す。
int get_hash_map(hash_map_t *hash_map, const char *key, void **data);

//データの削除
//キーに対応するデータを削除して1を返す。
//データが存在しない場合は0を返す。
int del_hash_map(hash_map_t *hash_map, const char *key);

//ハッシュマップのデータ数
int num_hash_map(hash_map_t *hash_map);

//イテレータ
typedef struct iterator iterator_t;

//ハッシュマップのイテレータを生成する。
iterator_t *iterate_hash_map(hash_map_t *hash_map);

//次のデータをkey,dataに設定して1を返す。key、dataにNULL指定可能。
//次のデータがない場合は0を返す。
//イテレートする順番はランダム。
int next_iterate(iterator_t* iterator, char **key, void **data);

//ハッシュマップのイテレータを解放する。
void end_iterate(iterator_t* iterator);

//ハッシュマップをダンプする
//level=0: 基本情報のみ
//level=1: 有効なキーすべて
//level=2: 削除済みも含める
void dump_hash_map(const char *str, hash_map_t *hash_map, int level);
