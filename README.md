# ハッシュマップ（オープンアドレス法）
C言語で実装したオープンアドレス法のハッシュマップです。

## 機能
- キー：バイト列、データ：任意のポインタ
- 追加・削除・検索・イテレート
- 1エントリ当たり1回のmalloc（キーの保存）
- ハッシュ関数は[FNV-1a、FNV-1](http://www.isthe.com/chongo/tech/comp/fnv/index.html)、[CRC32](https://ja.wikipedia.org/wiki/%E5%B7%A1%E5%9B%9E%E5%86%97%E9%95%B7%E6%A4%9C%E6%9F%BB#CRC-32)

## 参考
- https://jonosuke.hatenadiary.org/entry/20100406/p1
- [Wikipedia - ハッシュテーブル](https://ja.wikipedia.org/wiki/%E3%83%8F%E3%83%83%E3%82%B7%E3%83%A5%E3%83%86%E3%83%BC%E3%83%96%E3%83%AB)
- [Wikipedia - 巡回冗長検査](https://ja.wikipedia.org/wiki/%E5%B7%A1%E5%9B%9E%E5%86%97%E9%95%B7%E6%A4%9C%E6%9F%BB)
- [chibicc: A Small C Compiler](https://github.com/rui314/chibicc)
