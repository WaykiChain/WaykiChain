## Changes
- memory_size.0:
    - Change to start with one page of memory
        - This is necessary because the error messages for the `eosio::check` calls get put in the `data` segment so we must have a least one page of memory.
    - Adjust memory page counts accordingly in tests
- memory_size.2:
    - Change to start with one page of memory
        - This is necessary because the error messages for the `eosio::check` calls get put in the `data` segment so we must have a least one page of memory.
    - Adjust memory page counts accordingly in tests
