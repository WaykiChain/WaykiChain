#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

struct type_converter32 {
   union {
      uint32_t ui;
      float    f;
   } _data;
   type_converter32(uint32_t n) { _data.ui = n; }
   uint32_t to_ui() const { return _data.ui; }
   float    to_f() const { return _data.f; }
};

struct type_converter64 {
   union {
      uint64_t ui;
      double   f;
   } _data;
   type_converter64(uint64_t n) {
      _data.ui = n;
   }
   uint64_t to_ui() const { return _data.ui; }
   double   to_f() const {
      return _data.f;
   }
};

// C++20: using std::bit_cast;
template<typename T, typename U>
T bit_cast(const U& u) {
   static_assert(sizeof(T) == sizeof(U), "bitcast requires identical sizes.");
   T result;
   std::memcpy(&result, &u, sizeof(T));
   return result;
}

inline std::vector<uint8_t> read_wasm(const std::string& fname) {
   std::ifstream wasm_file(fname, std::ios::binary);
   if (!wasm_file.is_open())
      throw std::runtime_error("wasm file cannot be found");
   wasm_file.seekg(0, std::ios::end);
   std::vector<uint8_t> wasm;
   int                  len = wasm_file.tellg();
   if (len < 0)
      throw std::runtime_error("wasm file length is -1");
   wasm.resize(len);
   wasm_file.seekg(0, std::ios::beg);
   wasm_file.read((char*)wasm.data(), wasm.size());
   wasm_file.close();
   return wasm;
}
