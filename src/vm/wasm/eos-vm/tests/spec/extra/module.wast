(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\0a\01\00" ;; code
    "\03\01\00" ;; function
  )
  "out of order sections"
)

(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\03\01\00" ;; function
    "\03\01\00" ;; function
  )
  "duplicate section"
)

(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\01\01\00"  ;; type section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\02\01\00"  ;; import section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\03\01\00"  ;; function section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\04\01\00"  ;; table section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\05\01\00"  ;; memory section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\06\01\00"  ;; global section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\07\01\00"  ;; export section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\09\01\00"  ;; element section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\0a\01\00"  ;; code section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\0b\01\00"  ;; data section
    "\00\0e\06" "custom" "payload"
    "\00\0e\06" "custom" "payload"
    "\34"
  )
  "Invalid section id at end of module"
)

(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\03\02\00" ;; function
    "\0a\01\00" ;; code
  )
  "incorrect section size"
)
