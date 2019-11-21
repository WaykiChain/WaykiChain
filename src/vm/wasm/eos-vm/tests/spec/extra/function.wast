(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00" "\01\04\01\60" "\00\00\03\02"
    "\01\00\0a\07" "\01\05\00\0b" "\02\40\0b"
  )
  "extra code after end of function"
)

(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00" "\01\04\01\60" "\00\00\03\02"
    "\01\00\0a\06" "\01\04\00\02\40\0b"
  )
  "function body too long"
)

(assert_invalid
  (module binary
    "\00asm" ;; magic
    "\01\00\00\00" ;; version
    "\01\05\01\60\00\01\40" ;; types
    "\03\02\01\00"              ;; functions
    "\0a\04" "\01\02\00\0b" ;; code
  )
  "incorrect return type"
)
