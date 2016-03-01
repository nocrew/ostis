        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT
        MOVEQ #98,D0
        BKPT #7                 ; TEST_HOOK_EXIT
        
