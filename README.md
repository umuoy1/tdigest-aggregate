```
create table bytes (name TEXT, bytes BYTEA);

INSERT INTO bytes (bytes, name)
  SELECT tdigest_serialize(i, 500), 'test1'
  FROM generate_series(1,10000000) s(i);

SELECT tdigest_percentiles(bytes, ARRAY[0.5, 0.9, 0.99, 0.3, 0.123])
  FROM bytes
  WHERE name = 'test4';
```

#### 已实现功能
1. 百分位聚合计算
2. tdigest 状态序列化存储
3. 反序列化 tidiest 结构计算百分位（返回最小值、最大值、平均值）