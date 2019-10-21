(assert_malformed
  (module binary
    "\00asm" ;; magic
    "\01\00\00\00" ;; version
    "\01\04\01\60\00\00" ;; types
    "\03\02\01\00"              ;; functions
    "\0a\07" "\01\05\00\02\80\0b\0b" ;; code
  )
  "Invalid type for block"
)
