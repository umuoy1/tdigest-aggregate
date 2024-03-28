### API
#### `tdigest_percentile(value double precision, compression, quantile double precision)`
百分位聚合计算
```sql
CREATE TABLE t (a double precision);

INSERT INTO t SELECT random() FROM generate_series(1, 1000000);

SELECT tdigest_percentile(a, 300, 0.5) FROM t;
```
#### `tdigest_serialize(value double precision, compression)`
tdigest 状态序列化存储
```sql
CREATE TABLE bytes (name TEXT, bytes BYTEA);

INSERT INTO bytes (bytes, name)
  SELECT tdigest_serialize(i, 500), 'test1'
  FROM generate_series(1,10000000) s(i);
```
#### `tdigest_agg_from_bytes(bytes bytea, quantiles double precision[])`
反序列化 tidiest 结构计算百分位（返回最小值、最大值、平均值、中分数）
```sql
SELECT tdigest_agg_from_bytes(bytes, ARRAY[0.5, 0.9, 0.99, 0.3, 0.123])
  FROM bytes
  WHERE name = 'test1';
```