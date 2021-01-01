# ハッシュマップ（オープンアドレス法）
C言語で実装したオープンアドレス法のハッシュマップです。

## 機能
- キー：文字列、データ：任意のポインタ
- 追加・削除・検索
- 1エントリ当たり1回のmalloc（キーの保存）
- ハッシュ関数は[FNV-1aまたはFNV-1](http://www.isthe.com/chongo/tech/comp/fnv/index.html)

## 参考
- https://jonosuke.hatenadiary.org/entry/20100406/p1
- https://ja.wikipedia.org/wiki/%E3%83%8F%E3%83%83%E3%82%B7%E3%83%A5%E3%83%86%E3%83%BC%E3%83%96%E3%83%AB
- https://github.com/rui314/chibicc
