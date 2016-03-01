        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT
        MOVEQ #-1,D0
        MOVEQ #0,D1
        MOVE.L #$40000000,D2
        MOVE D2,CCR
        LSR.L #1,D0
        ROXL.L #1,D2
        MOVE D2,CCR
        LSR.L #1,D1
        ROXL.L #1,D2
        BKPT #1                 ; TEST_HOOK_TEST1
        MOVE.L #$10000,A0
        MOVE.W #$8000,(A0)
        ROXL (A0)
        BKPT #7                 ; TEST_HOOK_EXIT
        
