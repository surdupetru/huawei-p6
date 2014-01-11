typedef struct _StateTableEntry_t
{
        TxPowerStateEvent_e   event;
        TxPowerState_e        newState;
        void (*pfnTransitionHandler)(void);  // note that the size of this structure, on an 8051, should be exactly 4 bytes.
} StateTableEntry_t,*PStateTableEntry_t;

typedef struct _StateTableRowHeader_t
{

        void (*pfnEventGatherer)(void);  // note that the size of this structure, on an 8051, should be exactly 4 bytes.
        PStateTableEntry_t  pStateRow;
} StateTableRowHeader_t,*PStateTableRowHeader_t;

