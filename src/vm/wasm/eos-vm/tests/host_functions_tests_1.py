#!/usr/bin/python

from __future__ import print_function

types = [("b", "i32"), ("i32", "i32"), ("ui32", "i32"), ('i64', 'i64'), ('ui64', 'i64'), ('f32', 'f32'), ('f64', 'f64'),
         ('ptr', 'i32'), ('cptr', 'i32'), ('vptr', 'i32'), ('cvptr', 'i32'),
         ('ref', 'i32'), ('cref', 'i32'), ('vref', 'i32'), ('cvref', 'i32')]

print("(module", end='')

for (name, wasmtype) in types:
    print('''
   (func $put_%(name)s (export "put_%(name)s") (import "env" "put_%(name)s") (param %(type)s))
   (func $get_%(name)s (export "get_%(name)s") (import "env" "get_%(name)s") (result %(type)s))''' % {"name":name, "type":wasmtype}, end='')

print('\n\n   (table anyfunc (elem', end='')
for (name, wasmtype) in types:
    print(' $put_%(name)s $get_%(name)s' % {'name':name}, end='')
print('))', end='')

idx = 0
for (name, wasmtype) in types:
    print('''

   (func (export "call.put_%(name)s") (param %(type)s) (get_local 0) (call $put_%(name)s))
   (func (export "call_indirect.put_%(name)s") (param %(type)s)
      (get_local 0)
      (i32.const %(idx)d) ;; $put_%(name)s
      (call_indirect (param %(type)s)))''' % {"name":name, "type":wasmtype, "idx":idx}, end='')
    idx += 1
    print('''
   (func (export "call.get_%(name)s") (result %(type)s) (call $get_%(name)s))
   (func (export "call_indirect.get_%(name)s") (result %(type)s)
      (i32.const %(idx)d) ;; $get_%(name)s
      (call_indirect (result %(type)s)))''' % {"name":name, "type":wasmtype, "idx":idx}, end='')
    idx += 1

print('\n)')
