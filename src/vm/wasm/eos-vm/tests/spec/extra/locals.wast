(module
  (func $fail (result i64)
    (local i64)
    (local.get 0)
  )
  (func (export "local-zero-init") (result i64)
    (drop (i64.const 374625143947436)) ;; jit specific -- the top level function is called with RAX=0, so we need a layer of indirection to trigger a failure.
    (call $fail)
  )
)

(assert_return (invoke "local-zero-init") (i64.const 0))

;; 15-17 locals broke jit due to incorrect allocation size.
(module
  (func $16-locals (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64))
)

(assert_invalid
  (module binary
    "\00asm"                    ;; magic
    "\01\00\00\00"              ;; version
    "\01\04\01\60\00\00"        ;; types
    "\03\02\01\00"              ;; functions
    "\0a\06" "\01\04\01\01\32\0b" ;; code
  )
  "incorrect local type"
)

(assert_malformed
  (module binary
    "\00asm"                    ;; magic
    "\01\00\00\00"              ;; version
    "\01\05\01\60\01\7f\00"     ;; types [i32]->[]
    "\03\02\01\00"              ;; functions
    "\0a\0c\01\0a\02"           ;; code
    "\fe\ff\ff\ff\0f\7f"        ;; 0xFFFFFFFE i32
    "\01\7e"                    ;; 0x00000001 i64
    "\0b"                       ;; end
  )
  "Too many locals"
)
