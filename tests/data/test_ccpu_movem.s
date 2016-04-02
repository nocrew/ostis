        DC.L $FA52235F          ; Cartridge header
        BKPT #0                 ; TEST_HOOK_INIT

        BKPT #6                 ; Setup Test 1
        MOVEM.L D0-D3,(A0)
        BKPT #1                 ; Verify Test 1

        BKPT #6                 ; Setup Test 2
        MOVEM.L D0-D3,-(A0)
        BKPT #1                 ; Verify Test 2

        BKPT #6                 ; Setup Test 3
        MOVEM.L D0-D7,4(A0)
        BKPT #1                 ; Verify Test 3

        BKPT #6                 ; Setup Test 4
        MOVEM.W D0-D3,-(A7)
        BKPT #1                 ; Verify Test 4

        BKPT #6                 ; Setup Test 5
        DC.W $4CBA
        DC.W $000F
        DC.W $0016
        BKPT #1                 ; Verify Test 5

        BKPT #6                 ; Setup Test 6
        MOVEM.W (A0)+,D4-D7
        BKPT #1                 ; Verify Test 6

        BKPT #6                 ; Setup Test 7
        MOVEM.L (A7)+,D0-D7/A0-A7
        BKPT #1                 ; Verify Test 7

        BKPT #7                 ; TEST_HOOK_EXIT

data    DC.W $1234
        DC.W $55AA
        DC.W $FF00
        DC.W $AA55
