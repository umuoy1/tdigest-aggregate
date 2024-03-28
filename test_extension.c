#include "tdigest.h"

#include "postgres.h"
#include "fmgr.h"
#include "executor/executor.h"
#include "nodes/execnodes.h"
#include "nodes/plannodes.h"
#include "funcapi.h"
#include "utils/array.h"
#include "utils/lsyscache.h"
#include "tdigest.c"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(tdigest_add);
PG_FUNCTION_INFO_V1(tdigest_percentiles);
PG_FUNCTION_INFO_V1(tdigest_serial);
PG_FUNCTION_INFO_V1(tdigest_percentiles_from_bytes);

Datum tdigest_add(PG_FUNCTION_ARGS);
Datum tdigest_percentiles(PG_FUNCTION_ARGS);
Datum tdigest_serial(PG_FUNCTION_ARGS);
Datum tdigest_percentiles_from_bytes(PG_FUNCTION_ARGS);

typedef struct td_aggregate_state
{
  td_histogram_t *tdigest;
  double precision;
} td_aggregate_state;

/*
 * construct an SQL array from a simple C double array
 */
static Datum
double_to_array(FunctionCallInfo fcinfo, double *d, int len)
{
  ArrayBuildState *astate = NULL;
  int i;

  for (i = 0; i < len; i++)
  {
    /* stash away this field */
    astate = accumArrayResult(astate,
                              Float8GetDatum(d[i]),
                              false,
                              FLOAT8OID,
                              CurrentMemoryContext);
  }

  PG_RETURN_ARRAYTYPE_P(DatumGetPointer(makeArrayResult(astate,
                                                        CurrentMemoryContext)));
}

/*
 * Transform an input FLOAT8 SQL array to a plain double C array.
 *
 * This expects a single-dimensional float8 array, fails otherwise.
 */
static double *
array_to_double(FunctionCallInfo fcinfo, ArrayType *v, int *len)
{
  double *result;
  int nitems,
      *dims,
      ndims;
  Oid element_type;
  int16 typlen;
  bool typbyval;
  char typalign;
  int i;

  /* deconstruct_array */
  Datum *elements;
  bool *nulls;
  int nelements;

  ndims = ARR_NDIM(v);
  dims = ARR_DIMS(v);
  nitems = ArrayGetNItems(ndims, dims);

  /* this is a special-purpose function for single-dimensional arrays */
  if (ndims != 1)
    elog(ERROR, "expected a single-dimensional array (dims = %d)", ndims);

  /*
   * if there are no elements, set the length to 0 and return NULL
   *
   * XXX Can this actually happen? for empty arrays we seem to error out
   * on the preceding check, i.e. ndims = 0.
   */
  if (nitems == 0)
  {
    (*len) = 0;
    return NULL;
  }

  element_type = ARR_ELEMTYPE(v);

  /* XXX not sure if really needed (can it actually happen?) */
  if (element_type != FLOAT8OID)
    elog(ERROR, "array_to_double expects FLOAT8 array");

  /* allocate space for enough elements */
  result = (double *)palloc(nitems * sizeof(double));

  get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);

  deconstruct_array(v, element_type, typlen, typbyval, typalign,
                    &elements, &nulls, &nelements);

  /* we should get the same counts here */
  Assert(nelements == nitems);

  for (i = 0; i < nelements; i++)
  {
    if (nulls[i])
      elog(ERROR, "NULL not allowed as a percentile value");

    result[i] = DatumGetFloat8(elements[i]);
  }

  (*len) = nelements;

  return result;
}

Datum tdigest_add(PG_FUNCTION_ARGS)
{
  td_aggregate_state *state;

  MemoryContext aggcontext;

  if (!AggCheckCallContext(fcinfo, &aggcontext))
    elog(ERROR, "tdigest_add called in non-aggregate context");

  if (PG_ARGISNULL(1))
  {
    if (PG_ARGISNULL(0))
    {
      PG_RETURN_NULL();
    }

    PG_RETURN_DATUM(PG_GETARG_DATUM(0));
  }

  if (PG_ARGISNULL(0))
  {
    int compression = PG_GETARG_INT32(2);
    MemoryContext oldcontext = MemoryContextSwitchTo(aggcontext);

    state = palloc(sizeof(td_aggregate_state));
    state->tdigest = td_new(compression);

    if (PG_NARGS() >= 4)
      state->precision = PG_GETARG_FLOAT8(3);
    MemoryContextSwitchTo(oldcontext);
  }
  else
  {
    state = (td_histogram_t *)PG_GETARG_POINTER(0);
  }

  td_add(state->tdigest, PG_GETARG_FLOAT8(1), 1);
  PG_RETURN_POINTER(state);
}

Datum tdigest_percentiles(PG_FUNCTION_ARGS)
{
  td_aggregate_state *state;
  MemoryContext aggcontext;
  if (!AggCheckCallContext(fcinfo, &aggcontext))
    elog(ERROR, "tdigest_percentiles called in non-aggregate context");

  if (PG_ARGISNULL(0))
  {
    PG_RETURN_NULL();
  }

  state = (td_aggregate_state *)PG_GETARG_POINTER(0);
  double result = td_value_at(state->tdigest, state->precision);
  td_free(state->tdigest);
  PG_RETURN_FLOAT8(result);
}

Datum tdigest_serial(PG_FUNCTION_ARGS)
{
  td_aggregate_state *state;
  MemoryContext aggcontext;
  if (!AggCheckCallContext(fcinfo, &aggcontext))
    elog(ERROR, "tdigest_serial called in non-aggregate context");

  if (PG_ARGISNULL(0))
  {
    PG_RETURN_NULL();
  }

  state = (td_aggregate_state *)PG_GETARG_POINTER(0);
  unsigned char *buffer;
  int size = td_as_bytes(state->tdigest, &buffer);
  td_free(state->tdigest);
  pfree(state);
  if (size == 0)
  {
    PG_RETURN_NULL();
  }
  bytea *result = palloc(size + VARHDRSZ);
  SET_VARSIZE(result, size + VARHDRSZ);
  memcpy(VARDATA(result), buffer, size);
  free(buffer);
  PG_RETURN_BYTEA_P(result);
}

Datum tdigest_percentiles_from_bytes(PG_FUNCTION_ARGS)
{
  bytea *bytes = PG_GETARG_BYTEA_P(0);
  int size = VARSIZE(bytes) - VARHDRSZ;
  unsigned char *buffer = VARDATA(bytes);
  td_histogram_t *td = td_from_bytes(buffer, size);
  if (!td)
  {
    PG_RETURN_NULL();
  }
  double *values;
  int nvalues;
  values = array_to_double(fcinfo, PG_GETARG_ARRAYTYPE_P(1), &nvalues);
  if (values == NULL)
  {
    PG_RETURN_NULL();
  }

  int retn = nvalues + 3;
  double *results = palloc(retn * sizeof(double));
  results[0] = td_min(td);
  results[1] = td_max(td);
  results[2] = td_avg(td);
  for (int i = 3; i < retn; i++)
  {
    results[i] = td_value_at(td, values[i - 3]);
  }
  td_free(td);
  Datum result = double_to_array(fcinfo, results, retn);
  pfree(results);
  pfree(values);
  PG_RETURN_DATUM(result);
}

