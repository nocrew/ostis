        ORG $FA0000
        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT

        LEA smccode,A0
        LEA $10000,A1
        MOVEQ #codeend-smccode,D1
loop:   MOVE.B (A0)+,(A1)+
        DBRA D1,loop
        JMP $10000
smccode:
        MOVEQ #0,D0
        LEA wcode-smccode+$10000,A0
        MOVE.L #$4E714E71,(A0)
wcode:  
        ADDQ #1,D0
        ADDQ #1,D0
        BKPT #7                 ; TEST_HOOK_EXIT
codeend:
        
