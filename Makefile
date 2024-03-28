MODULES = test_extension
EXTENSION = test_extension        # the extensions name
DATA = test_extension--0.0.1.sql  # script files to install

# postgres build stuff
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)