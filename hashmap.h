//ハッシュマップ（オープンアドレス法）
//
//## 機能
//- キー：バイト列、データ：任意のポインタ
//- 追加・削除・検索・イテレート
//- 1エントリ当たり1回のmalloc（キーの保存）
//- ハッシュ関数は[FNV-1a、FNV-1](http://www.isthe.com/chongo/tech/comp/fnv/index.html)、
//  [CRC32]((https://ja.wikipedia.org/wiki/%E5%B7%A1%E5%9B%9E%E5%86%97%E9%95%B7%E6%A4%9C%E6%9F%BB#CRC-32))
//
//## 参考
//- https://jonosuke.hatenadiary.org/entry/20100406/p1
//- [Wikipedia - ハッシュテーブル](https://ja.wikipedia.org/wiki/%E3%83%8F%E3%83%83%E3%82%B7%E3%83%A5%E3%83%86%E3%83%BC%E3%83%96%E3%83%AB)
//- [Wikipedia - 巡回冗長検査](https://ja.wikipedia.org/wiki/%E5%B7%A1%E5%9B%9E%E5%86%97%E9%95%B7%E6%A4%9C%E6%9F%BB)
//- [chibicc: A Small C Compiler](https://github.com/rui314/chibicc)

//ハッシュ関数の種類
uint32_t fnv1a_hash(const char *s, int len);    //FNV-1a hash (Default)
uint32_t fnv1_hash (const char *s, int len);    //FNV-1 hash
uint32_t crc32_hash(const char *s, int len);    //CRC32 hash

//ハッシュ関数
typedef uint32_t(*hash_func_t)(const char *s, int len);

//ハッシュマップ
typedef struct hash_map hash_map_t;

//ハッシュマップを作成する。
//init_sizeが0の場合はデフォルト値(16)を用いる。
//hash_funcがNULLの場合はfnv1a_hashを用いる。
hash_map_t *new_hash_map(size_t init_size, hash_func_t hash_func);

//ハッシュマップをフリーする。
void free_hash_map(hash_map_t *hash_map);

//データ書き込み
//すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。
//キーにNULLは指定できない。dataにNULLを指定できる。
int put_hash_map(hash_map_t *hash_map, const char *key, int keylen, void *data);

//キーに対応するデータの取得
//存在すればdataに値を設定して1を返す。dataにNULLを指定できる。
//存在しなければ0を返す。
int get_hash_map(hash_map_t *hash_map, const char *key, int keylen, void **data);

//データの削除
//キーに対応するデータを削除して1を返す。
//データが存在しない場合は0を返す。
int del_hash_map(hash_map_t *hash_map, const char *key, int keylen);

//ハッシュマップのデータ数
int num_hash_map(hash_map_t *hash_map);

//イテレータ
typedef struct iterator iterator_t;

//ハッシュマップのイテレータを生成する。
iterator_t *iterate_hash_map(hash_map_t *hash_map);

//次のデータをkey,keylen,dataに設定して1を返す。key,keylen,dataにNULL指定可能。
//次のデータがない場合は0を返す。
//イテレートする順番はランダム。
int next_iterate(iterator_t* iterator, char **key, int *keylen, void **data);

//ハッシュマップのイテレータを解放する。
void end_iterate(iterator_t* iterator);

//ハッシュマップをダンプする
//level=0: 基本情報のみ
//level=1: 有効なキーすべて
//level=2: 削除済みも含める
void dump_hash_map(const char *str, hash_map_t *hash_map, int level);
