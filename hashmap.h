//ハッシュ関数（キー：文字列、データ：任意のポインタ）

//ハッシュ関数の種類
typedef enum {
    //https://jonosuke.hatenadiary.org/entry/20100406/p1
    //http://www.isthe.com/chongo/tech/comp/fnv/index.html
    HASH_MAP_FUNC_FNV_1A,  //FNV-1a hash (Default)
    HASH_MAP_FUNC_FNV_1,   //FNV-1 hash
    HASH_MAP_FUNC_FNV_DBG, //DEBUG
} HASH_MAP_FUNC_TYPE_t;

//ハッシュ関数の種類を設定するグローバル変数
extern HASH_MAP_FUNC_TYPE_t hash_map_func;

//ハッシュエントリ
typedef struct {
    char *key;              //ハッシュキー（文字列）
    void *data;             //ハッシュデータ（任意のポインタ）
} HASH_ENTRY_t;

//ハッシュマップ本体
typedef struct {
    int num;                //データ数
    int used;               //配列の使用数（データ数+TOMBSTONE数）
    int capacity;           //配列の最大数
    HASH_ENTRY_t *array;    //配列
} HASH_MAP_t;

//ハッシュマップを作成する。
HASH_MAP_t *new_hash_map(void);

//ハッシュマップを開放する。
void free_hash_map(HASH_MAP_t *hash_map);

//データ書き込み
//すでにデータが存在する場合は上書きし0を返す。新規データ時は1を返す。
//キーにNULLは指定できない。dataにNULLを指定できる。
int put_hash_map(HASH_MAP_t *hash_map, const char *key, void *data);

//キーに対応するデータの取得
//存在すればdataに値を設定して1を返す。
//存在しなければ0を返す。
int get_hash_map(HASH_MAP_t *hash_map, const char *key, void **data);

//データの削除
//キーに対応するデータを削除して1を返す。
//データが存在しない場合は0を返す。
int del_hash_map(HASH_MAP_t *hash_map, const char *key);

void dump_hash_map(const char *str, HASH_MAP_t *hash_map, int level);
