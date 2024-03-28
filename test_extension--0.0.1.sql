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


CREATE OR REPLACE FUNCTION tdigest_percentiles(internal) 
  RETURNS double precision 
  AS 'test_extension', 'tdigest_percentiles' 
  LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION tdigest_percentiles(bytea, double precision[]) 
  RETURNS double precision[] 
  AS 'test_extension', 'tdigest_percentiles_from_bytes' 
  LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION tdigest_serial(internal) 
  RETURNS bytea 
  AS 'test_extension', 'tdigest_serial' 
  LANGUAGE C IMMUTABLE;
  
CREATE AGGREGATE tdigest_percentile(double precision, int, double precision) (
  SFUNC = tdigest_add,
  STYPE = internal,
  FINALFUNC = tdigest_percentiles
);

CREATE AGGREGATE tdigest_serialize(double precision, int) (
  SFUNC = tdigest_add,
  STYPE = internal,
  FINALFUNC = tdigest_serial 
);
