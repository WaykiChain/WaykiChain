## How tests are generated
1. The JSON file for a spec test suite is read.
2. For each spec test wasm defined in the JSON:
    - All the spec tests are created in a C++ file to match the function declaration as interpreted from the JSON.
    - Test are categorized into `assert_trap`/`assert_exhaustion` and `assert_return`.
    - Tests are split up into `sub_apply` functions based on the rules defined below.
    - An `apply` function is written that switches on the third parameter to decide which `sub_apply` function to run.
    - A map of the function name and the index in order is created to be used for merging.
3. Unit Tests are generated based on the rules below.
4. The generated C++ files are compiled and linked, without optimizations to prevent the empty functions from being optimized out.
5. The generated WASM is combined with the original test wasm.
    - The imports and apply functions (and any helper functions) from the generated wasm are combined with the test function definitions from the spec test wasm.
    - Any necessary shifting of type/import/function/call/exports numbers is done.
        - This is where the generated map from above is used.
6. The newly created merged wasms and unit test C++ files are copied into the appropriate directory in the eos repo.

## How tests are split up
- Within a spec test suite, each `assert_trap` and `assert_exhaustion` test case is given a unique `sub_apply` function.
    - All tests in a suite are in the same WASM file, so the test that is run is based on the `test.name` passed in to `apply` (which calls the correct `sub_apply`).
- Within a test suite, `assert_return` tests are grouped into sets of 100.
    - This is due to the limit on 1024 locals and 1024 func defs built into nodeos. Some spec tests had too many functions to have a `sub_apply` per test, and some had too many variables to be put all into one `sub_apply`.
    - 100 was found to be the number that did not exceed this maximum for all the tests.
    - The tests also have some reliance on ordering (a store may need to be called before a load for example).
    - 100 also works out to make sure the right ordering is achieved.

- The unit tests are split into 2 groups. All of the `assert_trap` tests are grouped into one `BOOST_DATA_TEST_CASE` and all the `assert_return` tests are grouped into a second `BOOST_DATA_TEST_CASE`
- The unit test files are grouped by test suite (all `address` tests are together, all `call` tests together, etc.)

## How to generate tests
- Run the `setup_eosio_tests.py` script with no options to see the help text.
