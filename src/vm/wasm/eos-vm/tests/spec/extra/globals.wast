(assert_invalid
  (module
    (global i32 (i64.const -1))
  )
  "wrong global type"
)

(assert_malformed
  (module binary
    "\00asm"       ;; magic
    "\01\00\00\00" ;; version
    "\06\06"       ;; section globals size 6
    "\01"          ;; size 1
    "\7f"          ;; type i32
    "\FF"          ;; mutability
    "\41\00\0b"    ;; (i32.const 0)
  )
  "invalid mutablity"
)
