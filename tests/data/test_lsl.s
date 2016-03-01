        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT
        MOVEQ #-1,D0
        MOVEQ #0,D1
        MOVEQ #0,D2
        MOVE D2,CCR
        LSR.L #1,D0
        BKPT #1                 ; TEST_HOOK_TEST1
        MOVE D2,CCR
        LSR.L #1,D1
        BKPT #7                 ; TEST_HOOK_EXIT
        
