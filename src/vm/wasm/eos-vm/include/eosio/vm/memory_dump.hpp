#pragma once

#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/variant.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace eosio { namespace vm {

#define MEMORY_DUMP_OP_VISIT(name, code) \
   void operator()(const EOS_VM_OPCODE_T(name)& op) { \
      stream << #name << "\n"; \
   }

#define MEMORY_DUMP_CONTROL_FLOW_VISIT(name, code) \
   void operator()(const EOS_VM_OPCODE_T(name)& op) { \
      stream << #name << " : { " << op.data << ", " << op.pc << ", " << op.index << ", " << op.op_index << " }\n"; \
   }

#define MEMORY_DUMP_BR_TABLE_VISIT(name, code) \
   void operator()(const EOS_VM_OPCODE_T(name)& op) { \
      stream << #name << " : { [ "; \
      for (uint32_t i=0; i < op.size; i++) { \
         stream << op.table[i].pc; \
         if (i < op.size-1) { \
            stream << ", "; \
         } \
      } \
      stream << " ] }\n"; \
      index += op.offset; \
   }

#define MEMORY_DUMP_CALL_VISIT(name, code) \
   void operator()(const name##_t& op) { \
      stream << #name << " : { " << op.index << " }\n"; \
   }

#define MEMORY_DUMP_VARIABLE_ACCESS_VISIT(name, code) \
   void operator()(const name##_t& op) { \
      stream << #name << " : { " << op.index << " }\n"; \
   }

#define MEMORY_DUMP_MEMORY_VISIT(name, code) \
   void operator()(const name##_t& op) { \
      stream << #name << " : { " << op.flags_align << ", " << op.offset << " }\n"; \
   }

#define MEMORY_DUMP_CONST_VISIT(name, code) \
   void operator()(const name##_t& op) { \
      stream << #name << " : { " << op.data.ui << " }\n"; \
   }
   
   template <typename Stream>
   struct memory_dump_visitor {
      memory_dump_visitor(Stream&& stream, size_t& i) : stream(stream), index(i) {}
      EOS_VM_CONTROL_FLOW_OPS(MEMORY_DUMP_CONTROL_FLOW_VISIT)
      EOS_VM_BR_TABLE_OP(MEMORY_DUMP_BR_TABLE_VISIT)
      EOS_VM_RETURN_OP(MEMORY_DUMP_OP_VISIT)
      EOS_VM_CALL_OPS(MEMORY_DUMP_CALL_VISIT)
      EOS_VM_CALL_IMM_OPS(MEMORY_DUMP_CALL_VISIT)
      EOS_VM_PARAMETRIC_OPS(MEMORY_DUMP_OP_VISIT)
      EOS_VM_VARIABLE_ACCESS_OPS(MEMORY_DUMP_VARIABLE_ACCESS_VISIT)
      EOS_VM_I32_CONSTANT_OPS(MEMORY_DUMP_CONST_VISIT)
      EOS_VM_I64_CONSTANT_OPS(MEMORY_DUMP_CONST_VISIT)
      EOS_VM_F32_CONSTANT_OPS(MEMORY_DUMP_CONST_VISIT)
      EOS_VM_F64_CONSTANT_OPS(MEMORY_DUMP_CONST_VISIT)
      EOS_VM_COMPARISON_OPS(MEMORY_DUMP_OP_VISIT)
      EOS_VM_NUMERIC_OPS(MEMORY_DUMP_OP_VISIT)
      EOS_VM_CONVERSION_OPS(MEMORY_DUMP_OP_VISIT)
      EOS_VM_EXIT_OP(MEMORY_DUMP_OP_VISIT)
      EOS_VM_EMPTY_OPS(MEMORY_DUMP_OP_VISIT)
      EOS_VM_ERROR_OPS(MEMORY_DUMP_OP_VISIT)
      template <typename T>
      inline void operator()(T) { stream << "invalid opcode\n"; }
      Stream& stream;
      size_t& index;
   };

   template <typename Opcode>
   class memory_dump {
      public:
         memory_dump(Opcode* opcodes, size_t size) : _opcodes(opcodes), _size(size) {}

         template <typename Stream>
         void write(Stream&& stream) {
            size_t index=0;
            memory_dump_visitor<Stream> md(std::forward<Stream>(stream), index);
            for (; index < _size; index++) {
               eosio::vm::visit(std::move(md), std::move(_opcodes[index]));
            }
         }
      private:
         Opcode* _opcodes = nullptr;
         size_t  _size    = 0;
   };
}} // ns eosio::vm
