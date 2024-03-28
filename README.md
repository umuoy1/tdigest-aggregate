### API
#### `tdigest_percentile(value double precision, compression, quantile double precision)`
多行 number 聚合，返回最小值、最大值、平均值及与输入平行的分位值
```sql
CREATE TABLE t (a double precision);

INSERT INTO t SELECT random() FROM generate_series(1, 1000000);

SELECT tdigest_percentile(a, 300, 0.5) FROM t;
```
#### `tdigest_serialize(value double precision, compression)`
多行 number 聚合，输出 tdigest bytes
```sql
CREATE TABLE bytes (name TEXT, bytes BYTEA);

INSERT INTO bytes (bytes, name)
  SELECT tdigest_serialize(i, 500), 'test1'
  FROM generate_series(1,10000000) s(i);
```
#### `tdigest_agg_from_bytes(bytes bytea, percentiles double precision[])`
多行 bytea 聚合，返回最小值、最大值、平均值及与输入平行的分位值
```sql
SELECT tdigest_agg_from_bytes(bytes, Array[0.1, 0.2, 0.5])
  FROM bytes
  WHERE name = 'test1' 
    OR name = 'test2';
```