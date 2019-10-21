(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\05\04\01\02\01\01" ;; memory
  )
  "invalid limits"
)

(module (memory 65536))

(module binary
  "\00asm" "\01\00\00\00"
  "\05\03\01\04\00" ;; memory
)

(module binary
  "\00asm" "\01\00\00\00"
  "\05\04\01\80\00\00" ;; memory
)
