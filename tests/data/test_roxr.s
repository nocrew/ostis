        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT
        MOVEQ #0,D0
        MOVE.L #$10000,A0
        MOVE.W #1,(A0)
        MOVE.W #0,2(A0)
        MOVE D0,CCR
        ROXR (A0)               ; 0 + 0001 => 0000 + 1
        ROXR 2(A0)              ; 1 + 0000 => 8000 + 0
        MOVE 2(A0),D0           ; D0 == 8000
        BKPT #7                 ; TEST_HOOK_EXIT
        
