# EOS VM - A Low-Latency, High Performance and Extensible WebAssembly Engine

## ** Developer Preview **
*Not yet licensed as OSS.*

- Extremely Fast Execution (6x faster than WABT)
- Extremely Fast Parsing/Loading (20x faster than WABT)
- Efficient Time Bound Execution
- Deterministic Execution (Soft Float & Hardware Floating Point options)
- Standards Compliant
- Designed for Parallel Execution
- C++ / Header Only
- Simple API for integrating Native Calls

## Motivation
EOS VM is designed from the ground up for the high demands of blockchain applications which require far more from a WebAssembly engine than those designed for web browsers or standards development. In the world of blockchain, any non-deterministic behavior, unbounded computation, or unbounded use of RAM can take down the blockchain for everyone, not just a single user's web browser. Single threaded performance, fast compilation/validation of Wasm, and low-overhead calls to native code are critical to blockchains.

While EOS VM was designed for blockchain, we believe it is ideally suited for any application looking to embed a High Performance WebAssembly engine.

We designed EOS VM to meet the needs of EOSIO blockchains after using three of the most common WebAssembly engines: Binaryen, WABT, and WAVM. These WebAssembly engines were the single largest source of security issues impacting EOSIO blockchains. While WAVM provides extremely fast execution, it is not suited to running a live blockchain because it has extremely long and unpredictable compilation times and the need to recompile all contracts every time the process restarts.  WABT was designed as a toolkit for manipulating WebAssembly first and as an execution engine second.

We considered the WebAssembly engines used by the largest browsers, but they all come with considerable overhead and assumptions which are inappropriate for a reusable library or to be embedded in a blockchain. It is our hope that one day major browsers will opt to switch to EOS VM. 
 
All of the existing libraries incorporate a large code base designed and implemented by engineers not trained in the rigor of blockchain development. This makes the code difficult to audit and keeping up with upstream changes / security breaches difficult. 

With WebAssembly (Wasm) becoming ever more ubiquitous, there is a greater need for a succinct implementation of a Wasm backend.  We implemented __EOS VM__ because all existing backends we evaluated fell short in meeting our needs for a Wasm backend best suited for use in a public blockchain environment. 

## Deterministic Execution
Given that all programs on the blockchain must be deterministic, floating point operations are of particular interest to us.  Because of the non-deterministic nature of rounding modes, NaNs and denormals, special care has to be made to ensure a deterministic environment on all supported platforms.  This comes in the form of "softfloat", a software implementation of IEEE-754 float point arithmetic, which is constrained further to ensure determinism.  If this determinism is not required, hardware based floating point operations are still available through a compile time define.

Any secondary limits/constraints (i.e. stack size, call depth, etc.) can cause consensus failures if these restrictions do not match any previous backend that was in place, __EOS VM__ has all of these constraints user definable through either a compile-time system or run-time based on the use case and data type involved.

## Time Bounded Execution
The ability to ensure that execution doesn't over run the CPU time that is allotted for a given program is a central component of a resource limited blockchain.  This is satisfied by the watchdog timer system (as mentioned below, this mechanism is also useful for general security). EOS VM's implementation is both fast and efficient compared to prior solutions. 

Two mechanisms are available to the user to bound the execution of Wasm:
  1) A simple instruction counter based bounding, this incurs a performance penalty, but doesn't require multi-threading.
  2) A watchdog timer solution that incurs no noticeable overhead during Wasm execution.

## Secure by Design
WebAssembly was designed to run untrusted code in a browser environment where the worst that can happen is a hung browser. Existing libraries such as WABT, WAVM, and Binaryen were designed with assumptions which can lead to unbounded memory allocation, extremely long load times, and stack overflows from recursive descent parsing or execution.  

The fundamental data types that make up __EOS VM__ are built with certain invariants from the onset.  This means that explicit checks and validations, which can be error-prone because of programmer forgetfulness, are not needed as the data types themselves maintain these invariants and kill the execution if violated.  

In addition to these core data types, some of the special purpose allocators utilize the security of the CPU and core OS to satisfy that memory is properly sandboxed (a guard paging mechanism).  

Because of the utilization of guard paging for the memory operations, host function calls that execute natively don't have to explicitly validate pointers that are passed into these functions if access outside of the sandboxed memory occurs, please note special care should be made to ensure that the host function can fail hard, i.e. not call destructors and have no negative impact.

At no point during parsing or evaluation does EOS-VM use unbounded recursion or loops, everything is tightly bound to limit or eliminate the ability for a bad or corrupt Wasm to cause a crash or infinitely hang the machine.

All of these solutions are transparent to the developer and allow for more succinct functions that are not cluttered with external checks and only the core logic is needed in most places.  

## High-Performance Execution
Host functions are callable through a thin layer that doesn't incur heavy performance penalties.

Because of the utilization of native paging mechanisms, almost all memory operations are very close to native if not at parity with native memory operations.

Because of the high compaction and linear nature of the builtin allocators, this allows for a very cache friendly environment and further allows for high performance execution.

Certain design decisions were made to maximize the performance of interpreter implementation.  As mentioned above, __EOS VM__ has custom allocators and memory management that fits the needs and use cases for different access patterns and allocation requirements.  These allocators are used to back the core data types (fast vector, Wasm stack, fast variant, Wasm module), and as such do not "own" the memory that they use for their operations.  These non-owning data structures allow for the ability to use the memory cleanly and not have to concern the data type with destructing when going out of scope, which can increase the performance for certain areas of EOS VM without loss of generality for the developer.  Since the data is held by these allocators and have lifetimes that match that of a Wasm module, no copies of these heavyweight data types are ever needed.  Once an element in an EOS VM is constructed, that is its home and final resting place for the lifetime of the Wasm module.  

A fast `variant` or discriminating union type is the fundamental data type that represents a Wasm opcode or a Wasm stack element.  This allows for a clean interface to "visit" each Wasm opcode without any loss of performance.  This visiting is statically derivable and not dynamically dispatched like more classical approaches that use the object-oriented visitor pattern.  In addition to a `visit` function that acts similar to `std::visit`, a custom dispatcher is defined that allows for a similar interface but with __EOS VM__ specific optimizations and assumptions.


## Effortless Integration
With the exception of the softfloat library, which is an external dependency, __EOS VM__ is a header only implementation.

Given the needs of the end user, integration can be as simple as pointing to the include directory.

__EOS-VM__ utilizes __CMake__ which allows integration into a project to be as little as adding `eos-vm` to the list of targets in the `target_link_libraries`.

If the need is only single-threaded a self-contained backend type is defined for the user to encapsulate all the components needed, which allows for source code integration to be constructing an instance of that type and adding "host functions" to the `registered_host_functions`.  Registering the host functions is as easy as calling a function with the function/member pointer and supplying the Wasm module name and function name.

If multi-threaded execution is needed (i.e. multiple backends running at once), then the above integration is needed and the user will have to also construct thread specific watchdog timers and linear memory allocators.  These are also designed to be effortlessly registered to a particular Wasm backend.  

## Highly Extensible Design
Given the __EOS-VM__ variant type and visitor system, new backends with custom logic can be easily defined and allows the same level of flexibility and code reuse as a much more heavyweight OOP __Visitor__ or __Listener__ design.

Since the design of __EOS-VM__ is component based, with each component being very self-contained, new backends or tools for Wasm can be crafted from previously defined components while only needing to define the logic for the extended functionality that is needed, with very little, to no, boilerplate needed.

Extensions to Wasm itself can be made by simply defining the new section (aka C++ class field) for the module and the function needed to parse an element of that section.  This will allow for tooling to be constructed at a rapid pace for custom Wasms for a multitude of needs (debugging, profiling, etc.).

## Using EOS-VM
[Quick Overview](./docs/OVERVIEW.md)


## Contributing

[Contributing Guide](./CONTRIBUTING.md)

[Code of Conduct](./CONTRIBUTING.md#conduct)

## Important

Refer to [LICENSE](./LICENSE) for copyright notice.  Block.one makes its contribution on a voluntary basis as a member of the EOSIO community and is not responsible for ensuring the overall performance of the software or any related applications.  We make no representation, warranty, guarantee or undertaking in respect of the software or any related documentation, whether expressed or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall we be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or documentation or the use or other dealings in the software or documentation. Any test results or performance figures are indicative and will not reflect performance under all conditions.  Any reference to any third party or third-party product, service or other resource is not an endorsement or recommendation by Block.one.  We are not responsible, and disclaim any and all responsibility and liability, for your use of or reliance on any of these resources. Third-party resources may be updated, changed or terminated at any time, so the information here may be out of date or inaccurate.  Any person using or offering this software in connection with providing software, goods or services to third parties shall advise such third parties of these license terms, disclaimers and exclusions of liability.  Block.one, EOSIO, EOSIO Labs, EOS, the heptahedron and associated logos are trademarks of Block.one.
