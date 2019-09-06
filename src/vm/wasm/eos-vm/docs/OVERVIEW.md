## Building EOS-VM
To build __EOS-VM__ you will need a fully C++17 compliant toolchain, since **CMake** is not perfect some toolchains will appear to **CMake** as 17 compliant but are not, we will do
our best to special case these as they arise.

Since __EOS-VM__ is designed to be a header only library (with the exception of softfloat, until the new header only softfloat library is finished), building __EOS-VM__ is not necessary
to use __EOS-VM__ in a C++ project. But, if you would like to use the softfloat capabilities, build the example tools or the tests, then all that is needed is to create a build directory
and execute the command `cmake ..` and then `make` after this all tools and tests will be built.

## Using The Example Tools
Once you have built __EOS-VM__ you will notice 3 tools in the directory **build/tools**, you can run your test WASMs by executing the command `eos-vm-interp <path>/<wasm name>.wasm` this
will then run all exported functions within that WASM.  You can also run `bench-interp <path>/<wasm name>.wasm` and get two times in nanoseconds; the time to parse and instaniate your WASM
and the time to execute your WASM.  The last tool is `hello-driver`, this has a prebaked in helloworld WASM and uses user input to bound the number of loops the printing occurs and whether
it should assert, this tool is an example of how to setup a fully integrated solution with host functions.

Both of these are designed to be modified by the end-user and are simply there to show how to easily integrate __EOS-VM__ into your own project.

## Integrating Into Existing CMake Project
By adding __EOS-VM__ as a submodule to your project and adding the subdirectory that contains __EOS-VM__ and adding **eos-vm** to the list of link libraries of your executables/libraries
is all that is required to integrate into your project.  A list of CMake options that can be passed into via command line or with CMake **set** can be found in **CMakeLists.txt** and
**modules/EosVMBuildUtils.cmake** or by running `ccmake ..` instead of `cmake ..`.

### Getting Started
 1) Start by creating a type alias of `eosio::vm::backend` with the host function class type.
 2) Next you can create a `watchdog` timer with a specific duration type, and setting the duration to the time interval you need.  
 3) You can set the callback for this timer to quickly exit the running WASM context (in a future release this will be more tightly integrated).  
 4) You should now read the wasm, the `eosio::vm::backend` class has a method to read from a file or you can use a `std::vector<uint8_t>` that already contains the WASM.  This gets passed to the constructor of `eosio::vm::backend`.
 5) You should register and resolve any host functions, please the **Adding Host Functions** section.
 5) Finally, you can execute a specific export via the `()` operator of `eosio::vm::backend`, this takes the host function instance reference, the module name, the export name and typesafe export arguments. (see **/tools/interp.cpp** and **/tools/hello_driver.cpp** for more details)

### Adding Host Functions
Without any host functions, your WASM execution is going to be very limited (you will not be able to observe side effects or get intermediate values).  

There are three options currently for creating "host functions":
   1) C-style function
   2) static method of a class
   3) proper method of a class

Currently, you are limited to only one type of host class to encapsulate your host functions, this limitation will be removed in a future release.

#### Registered Host Functions Class
Firstly, you should create a type alias to limit the redundant typing needed.
   - `using rhf_t = eosio::vm::registered_host_functions<ClassType>;`, where **ClassType** is either `nullptr_t` if you are only using C-style functions, or the type of the host class.
Then, you will `add` your host functions via `rhf_t::add<ClassType, &function_name, wasm_allocator>("module name", "wasm function name");` for each host function.

The last thing to do is to resolve the imports of your `module`, this is handled by `rhf_t::resolve( <reference to module> );`.

For a more concise example of this, please see **/tools/hello_driver.cpp**.
