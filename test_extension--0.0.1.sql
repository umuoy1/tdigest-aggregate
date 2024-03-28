CREATE OR REPLACE FUNCTION tdigest_add(
  internal,
  double precision,
  int,
  double precision
) 
  RETURNS internal 
  AS 'test_extension', 'tdigest_add' 
  LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION tdigest_add(
  internal,
  double precision,
  int
) 
  RETURNS internal 
  AS 'test_extension', 'tdigest_add' 
  LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION tdigest_add(
  internal,
  bytea,
  double precision[]
) 
  RETURNS internal 
  AS 'test_extension', 'tdigest_add' 
  LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION tdigest_percentiles(internal) 
  RETURNS double precision[]
  AS 'test_extension', 'tdigest_percentiles' 
  LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION tdigest_serial(internal) 
  RETURNS bytea 
  AS 'test_extension', 'tdigest_serial' 
  LANGUAGE C IMMUTABLE;


CREATE OR REPLACE FUNCTION tdigest_from_bytes(bytea, double precision[]) 
  RETURNS double precision[] 
  AS 'test_extension', 'tdigest_from_bytes' 
  LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION tdigest_agg(
  internal,
  bytea,
  double precision[]
)
  RETURNS internal 
  AS 'test_extension', 'tdigest_agg' 
  LANGUAGE C IMMUTABLE;

-- 多行 bytea 聚合，返回最小值、最大值、平均值及与输入平行的分位值
CREATE AGGREGATE tdigest_agg_from_bytes(bytea, double precision[]) (
  SFUNC = tdigest_agg,
  STYPE = internal,
  FINALFUNC = tdigest_percentiles
);

-- 多行 number 聚合，返回最小值、最大值、平均值及与输入平行的分位值
CREATE AGGREGATE tdigest_percentile(double precision, int, double precision) (
  SFUNC = tdigest_add,
  STYPE = internal,
  FINALFUNC = tdigest_percentiles
);

-- 多行 number 聚合，输出 tdigest bytes
CREATE AGGREGATE tdigest_serialize(double precision, int) (
  SFUNC = tdigest_add,
  STYPE = internal,
  FINALFUNC = tdigest_serial 
);
