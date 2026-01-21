#include "pin.H"
#include <iostream>
#include <map>
#include <vector>
#include <string>

struct FuncStats {
    UINT64 calls = 0;
    UINT64 stackOps = 0;
    std::string name;
    bool patched = false;
};

std::map<ADDRINT, FuncStats> profile;
const UINT64 HOTSPOT_THRESHOLD = 500;

int ReplacementFunction(CONTEXT *ctxt, AFUNPTR orgFuncptr, int arg) {
    return arg * arg; 
}

VOID OnCall(ADDRINT addr) {
    FuncStats &s = profile[addr];
    s.calls++;
}

VOID OnStackOp(ADDRINT addr) {
    profile[addr].stackOps++;
}

VOID Routine(RTN rtn, VOID *v) {
    RTN_Open(rtn);
    ADDRINT addr = RTN_Address(rtn);
    profile[addr].name = RTN_Name(rtn);

    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)OnCall, IARG_ADDRINT, addr, IARG_END);

    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        if (INS_IsStackWrite(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)OnStackOp, IARG_ADDRINT, addr, IARG_END);
        }
    }

    if (profile[addr].calls >= HOTSPOT_THRESHOLD && !profile[addr].patched) {
        PROTO proto = PROTO_Allocate(PIN_PARG(int), CALLINGSTD_DEFAULT, "proto", PIN_PARG(int), PIN_PARG_END());
       RTN_ReplaceSignature(
    rtn,
    (AFUNPTR)ReplacementFunction,
    IARG_PROTOTYPE, proto,
    IARG_CONTEXT,
    IARG_ORIG_FUNCPTR,
    IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
    IARG_END
);

        profile[addr].patched = true;
    }

    RTN_Close(rtn);
}

VOID Fini(INT32 code, VOID *v) {
    std::cout << "\n[REPORT] WiSe Hack'25 - Analysis\n";
    for (auto const& [addr, s] : profile) {
        if (s.calls > 0) {
            std::cout << "Func: " << s.name << " | Calls: " << s.calls 
                      << " | Stack Ops: " << s.stackOps 
                      << (s.patched ? " [HOT-PATCHED]" : "") << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return 1;
    RTN_AddInstrumentFunction(Routine, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    return 0;
}