(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\04\05\01\70\02\00\00" ;; table
  )
  "invalid limits"
)

(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\04\05\01\70\01\01\00" ;; table
  )
  "invalid limits"
)

(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\04\05\01\60\01\01\00" ;; table
  )
  "invalid type"
)

;; Try to cause overflow.  Unfortunately, this fails on allocation first.
;; (module (table 0x80000000 funcref))
