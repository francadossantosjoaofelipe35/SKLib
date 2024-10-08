﻿#include <VMExitHandlers.h>
#include <Arch/Registers.h>
#include "Arch/Vmx.h"

#define	CPUIDECX_SSE3	0x00000001	/* streaming SIMD extensions #3 */
#define	CPUIDECX_PCLMUL	0x00000002	/* Carryless Multiplication */
#define	CPUIDECX_DTES64	0x00000004	/* 64bit debug store */
#define	CPUIDECX_MWAIT	0x00000008	/* Monitor/Mwait */
#define	CPUIDECX_DSCPL	0x00000010	/* CPL Qualified Debug Store */
#define	CPUIDECX_VMX	0x00000020	/* Virtual Machine Extensions */
#define	CPUIDECX_SMX	0x00000040	/* Safer Mode Extensions */
#define	CPUIDECX_EST	0x00000080	/* enhanced SpeedStep */
#define	CPUIDECX_TM2	0x00000100	/* thermal monitor 2 */
#define	CPUIDECX_SSSE3	0x00000200	/* Supplemental Streaming SIMD Ext. 3 */
#define	CPUIDECX_CNXTID	0x00000400	/* Context ID */
#define CPUIDECX_SDBG	0x00000800	/* Silicon debug capability */
#define	CPUIDECX_FMA3	0x00001000	/* Fused Multiply Add */
#define	CPUIDECX_CX16	0x00002000	/* has CMPXCHG16B instruction */
#define	CPUIDECX_XTPR	0x00004000	/* xTPR Update Control */
#define	CPUIDECX_PDCM	0x00008000	/* Perfmon and Debug Capability */
#define	CPUIDECX_PCID	0x00020000	/* Process-context ID Capability */
#define	CPUIDECX_DCA	0x00040000	/* Direct Cache Access */
#define	CPUIDECX_SSE41	0x00080000	/* Streaming SIMD Extensions 4.1 */
#define	CPUIDECX_SSE42	0x00100000	/* Streaming SIMD Extensions 4.2 */
#define	CPUIDECX_X2APIC	0x00200000	/* Extended xAPIC Support */
#define	CPUIDECX_MOVBE	0x00400000	/* MOVBE Instruction */
#define	CPUIDECX_POPCNT	0x00800000	/* POPCNT Instruction */
#define	CPUIDECX_DEADLINE	0x01000000	/* APIC one-shot via deadline */
#define	CPUIDECX_AES	0x02000000	/* AES Instruction */
#define	CPUIDECX_XSAVE	0x04000000	/* XSAVE/XSTOR States */
#define	CPUIDECX_OSXSAVE	0x08000000	/* OSXSAVE */
#define	CPUIDECX_AVX	0x10000000	/* Advanced Vector Extensions */
#define	CPUIDECX_F16C	0x20000000	/* 16bit fp conversion  */
#define	CPUIDECX_RDRAND	0x40000000	/* RDRAND instruction  */
#define	CPUIDECX_HV	0x80000000	/* Running on hypervisor */


#define	CPUIDECX_LAHF		0x00000001 /* LAHF and SAHF instructions */
#define	CPUIDECX_CMPLEG		0x00000002 /* Core MP legacy mode */
#define	CPUIDECX_SVM		0x00000004 /* Secure Virtual Machine */
#define	CPUIDECX_EAPICSP	0x00000008 /* Extended APIC space */
#define	CPUIDECX_AMCR8		0x00000010 /* LOCK MOV CR0 means MOV CR8 */
#define	CPUIDECX_ABM		0x00000020 /* LZCNT instruction */
#define	CPUIDECX_SSE4A		0x00000040 /* SSE4-A instruction set */
#define	CPUIDECX_MASSE		0x00000080 /* Misaligned SSE mode */
#define	CPUIDECX_3DNOWP		0x00000100 /* 3DNowPrefetch */
#define	CPUIDECX_OSVW		0x00000200 /* OS visible workaround */
#define	CPUIDECX_IBS		0x00000400 /* Instruction based sampling */
#define	CPUIDECX_XOP		0x00000800 /* Extended operating support */
#define	CPUIDECX_SKINIT		0x00001000 /* SKINIT and STGI are supported */
#define	CPUIDECX_WDT		0x00002000 /* Watchdog timer */
/* Reserved			0x00004000 */
#define	CPUIDECX_LWP		0x00008000 /* Lightweight profiling support */
#define	CPUIDECX_FMA4		0x00010000 /* 4-operand FMA instructions */
#define	CPUIDECX_TCE		0x00020000 /* Translation Cache Extension */
/* Reserved			0x00040000 */
#define	CPUIDECX_NODEID		0x00080000 /* Support for MSRC001C */
/* Reserved			0x00100000 */
#define	CPUIDECX_TBM		0x00200000 /* Trailing bit manipulation instruction */
#define	CPUIDECX_TOPEXT		0x00400000 /* Topology extensions support */
#define	CPUIDECX_CPCTR		0x00800000 /* core performance counter ext */
#define	CPUIDECX_DBKP		0x04000000 /* DataBreakpointExtension */
#define	CPUIDECX_PERFTSC	0x08000000 /* performance time-stamp counter */
#define	CPUIDECX_PCTRL3		0x10000000 /* L3 performance counter ext */
#define	CPUIDECX_MWAITX		0x20000000 /* MWAITX/MONITORX */

#define VMM_CPUIDECX_MASK ~(CPUIDECX_EST | CPUIDECX_TM2 | CPUIDECX_MWAIT | \
    CPUIDECX_PDCM | CPUIDECX_VMX | CPUIDECX_DTES64 | \
    CPUIDECX_DSCPL | CPUIDECX_SMX | CPUIDECX_CNXTID | \
    CPUIDECX_SDBG | CPUIDECX_XTPR | CPUIDECX_PCID | \
    CPUIDECX_DCA | CPUIDECX_X2APIC | CPUIDECX_DEADLINE)
#define VMM_ECPUIDECX_MASK ~(CPUIDECX_SVM | CPUIDECX_MWAITX)
#include <Arch/Cpuid.h>
#include "Vmcall.h"
#include <identity.h>

bool VTx::VMExitHandlers::HandleCPUID(PREGS pContext)
{
    INT32 CpuInfo[4] = { 0 };
    size_t Mode = 0;

    if (vmcall::ValidateCommunicationKey(0) && //Key not set yet
        pContext->rax == 'Hypr' && pContext->rcx == 'Chck') {
        pContext->rax = 'Yass';
        return false;
    }
    else if (vmcall::ValidateCommunicationKey(pContext->rax)
        && vmcall::IsVmcall(pContext->r9)) {
        //Use CPUID as a VMCALL substitute for
        //usermode modules
        pContext->rax = vmcall::HandleVmcall(pContext->rcx, pContext->rdx, pContext->r8, pContext->r9);
        return false;
    }

    //
    // Otherwise, issue the CPUID to the logical processor based on the indexes
    // on the VP's GPRs
    //
    __cpuidex(CpuInfo, (INT32)pContext->rax, (INT32)pContext->rcx);

    if (pContext->rax == PROC_FEATURES_CPUID)
    {
        //
        // Unset the Hypervisor Present-bit in RCX, which Intel and AMD have both
        // reserved for this indication
        //
        CpuInfo[2] |= HYPERV_HYPERVISOR_PRESENT_BIT;
        CpuInfo[2] &= VMM_CPUIDECX_MASK;
    }
    else if (pContext->rax == (unsigned long)Cpuid::Generic::GenericLeaf::ExtendedFeatureInformation)
    {
        //CpuInfo[2] &= VMM_ECPUIDECX_MASK;
    }
    else if (pContext->rax == HYPERV_CPUID_INTERFACE)
    {
        //
        // Return our interface identifier
        //
        //CpuInfo[0] = CPU::chInterfaceID;
    }
    else if (pContext->rax == PROC_MANU_CPUID) {
#ifdef PROCESSOR_MANUFACTURER
        int* values = (int*)PROCESSOR_MANUFACTURER;
        pContext->rbx = values[0];
        pContext->rcx = values[1];
        pContext->rdx = values[2];
#endif
    }

    //
    // Copy the values from the logical processor registers into the VP GPRs
    //
    pContext->rax = CpuInfo[0];
    pContext->rbx = CpuInfo[1];
    pContext->rcx = CpuInfo[2];
    pContext->rdx = CpuInfo[3];

    return FALSE; // Indicates we don't have to turn off VMX
}

#define	CR4_VMXE	0x00002000	/* enable virtual machine operation */

//When returning false inject a #GP
bool VTx::VMExitHandlers::HandleCR(PREGS pContext)
{
    DWORD64 mask = 0;
    size_t ExitQualification = 0;
    size_t guestRIP;

    __vmx_vmread(GUEST_RIP, &guestRIP);
    __vmx_vmread(EXIT_QUALIFICATION, &ExitQualification);

    PMOV_CR_QUALIFICATION data = (PMOV_CR_QUALIFICATION)&ExitQualification;

    PULONG64 RegPtr = (PULONG64)&pContext->rax + data->Fields.Register;

    //
    // Because its RSP and as we didn't save RSP correctly (because of pushes)
    // so we have to make it points to the GUEST_RSP
    //
    if (data->Fields.Register == 4)
    {
        size_t RSP = 0;
        __vmx_vmread(GUEST_RSP, &RSP);
        *RegPtr = RSP;
    }

    switch (data->Fields.AccessType)
    {
    case TYPE_MOV_TO_CR:
    {
        switch (data->Fields.ControlRegister)
        {
        case 0:
        {
            CR0 cr0;
            cr0.Flags = *RegPtr;
            if (cr0.Reserved1 || cr0.Reserved2 || cr0.Reserved3 || cr0.Reserved4)
                return true;
            __vmx_vmwrite(GUEST_CR0, *RegPtr);
            __vmx_vmwrite(CR0_READ_SHADOW, *RegPtr);

            if (!CPU::bCETSupported)
                break;

            if (!cr0.WriteProtect) {
                CR4 cr4 = { 0 };
                __vmx_vmread(GUEST_CR4, &cr4.Flags);
                if (!cr4.CETEnabled) {
                    break;
                }
                vmm::vGuestStates[CPU::GetCPUIndex(true)].bCETNeedsEnabling = true;
                cr4.CETEnabled = false;
                __vmx_vmwrite(GUEST_CR4, cr4.Flags);
                __vmx_vmwrite(CR4_READ_SHADOW, cr4.Flags);
            }
            else {
                if (!vmm::vGuestStates[CPU::GetCPUIndex(true)].bCETNeedsEnabling) {
                    break;
                }
                CR4 cr4 = { 0 };
                __vmx_vmread(GUEST_CR4, &cr4.Flags);
                cr4.CETEnabled = true;
                __vmx_vmwrite(GUEST_CR4, cr4.Flags);
                __vmx_vmwrite(CR4_READ_SHADOW, cr4.Flags);
                vmm::vGuestStates[CPU::GetCPUIndex(true)].bCETNeedsEnabling = false;
            }
            break;
        }
        case 3:
        {
            vmm::UpdateLastValidTsc();

            DWORD dwCore = CPU::GetCPUIndex(true);
            PVM_STATE pState = &vmm::vGuestStates[dwCore];
            CR3 cr3 = { 0 };
            cr3.Flags = *RegPtr;
            if (cr3.Reserved1 || cr3.Reserved2 || cr3.Reserved3)
                return true;

            //CR3 lastBlockedCr3 = { 0 };
            //if (pState->lastExitedCr3)
            //    lastBlockedCr3.Flags = pState->lastExitedCr3;
            //else
            //    lastBlockedCr3 = vmm::GetGuestCR3();
            //
            //if (
            //    !eac::IsNmiBlocked(cr3)
            //    && eac::IsNmiBlocked(lastBlockedCr3)
            //    ) {
            //    if (eac::GetAndDecreaseNmiCount(lastBlockedCr3)) {
            //        IA32_VMX_PROCBASED_CTLS_REGISTER procbased_ctls;
            //        __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &procbased_ctls.Flags);
            //
            //        procbased_ctls.NmiWindowExiting = true;
            //        __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, procbased_ctls.Flags);
            //
            //        pState->lastExitedCr3 = vmm::GetGuestCR3().Flags;
            //    }
            //    else {
            //        pState->lastExitedCr3 = 0;
            //    }
            //}

            pState->lastExitedCr3 = *RegPtr;
            __vmx_vmwrite(GUEST_CR3, *RegPtr);

            INVVPID_DESCRIPTOR desc = { 0 };
            desc.Vpid = dwCore + 1;
            CPU::InvalidateVPID(3, &desc);
            break;
        }
        case 4:
        {
            CR4 cr4;
            cr4.Flags = *RegPtr;

            mask = __readmsr(IA32_VMX_CR4_FIXED1);
            if (cr4.Flags & mask) {
                return true;
            }
            mask = __readmsr(IA32_VMX_CR4_FIXED0);
            if ((cr4.Flags & mask) != mask) {
                return true;
            }

            __vmx_vmwrite(GUEST_CR4, (*RegPtr | CR4_VMXE));
            __vmx_vmwrite(CR4_READ_SHADOW, *RegPtr);
            break;
        }
        default:
        {
            break;
        }
        }
    }
    break;

    case TYPE_MOV_FROM_CR:
    {
        switch (data->Fields.ControlRegister)
        {
        case 0:
            __vmx_vmread(GUEST_CR0, RegPtr);
            break;
        case 3:
            __vmx_vmread(GUEST_CR3, RegPtr);
            break;
        case 4:
            __vmx_vmread(GUEST_CR4, RegPtr);
            *RegPtr |= ~(CR4_VMXE);
            break;
        default:
            break;
        }
    }
    break;

    default:
        break;
    }

    return false;
}

bool VTx::VMExitHandlers::HandleGDTRIDTR(PREGS pContext)
{
    auto dwCore = CPU::GetCPUIndex(true);
    auto pState = &vmm::vGuestStates[dwCore];

    DWORD64 mask = 0;
    size_t ExitInstructionInfo = 0;
    size_t displacement = 0;
    size_t segBase = 0;
    size_t guestRIP;
    Vmx::SegmentAccessRights csAttrib;

    __vmx_vmread(GUEST_RIP, &guestRIP);
    __vmx_vmread(GUEST_CS_AR_BYTES, (size_t*)&csAttrib);
    __vmx_vmread(EXIT_QUALIFICATION, &displacement);
    __vmx_vmread(VMX_INSTRUCTION_INFO, &ExitInstructionInfo);

    Vmx::ExitInstructionInformation* data = (Vmx::ExitInstructionInformation*)&ExitInstructionInfo;

    DWORD64 ptr = 0;

    if (!data->f2.base_invalid) {
        PULONG64 RegPtr = (PULONG64)&pContext->rax + data->f2.base;

        //
        // Because its RSP and as we didn't save RSP correctly (because of pushes)
        // so we have to make it points to the GUEST_RSP
        //
        if (data->f2.base == 4)
        {
            size_t RSP = 0;
            __vmx_vmread(GUEST_RSP, &RSP);
            *RegPtr = RSP;
        }

        ptr = *RegPtr;
    }
    if (!data->f2.index_invalid) {
        ptr += *((PULONG64)&pContext->rax + data->f2.index) << data->f2.scaling;
    }

    __vmx_vmread(GUEST_ES_BASE + (data->f2.segment << 1), &segBase);
    ptr += segBase;

    if (data->f2.address_size == 1)
        ptr &= 0xffffffff;
    else if (data->f2.address_size == 0)
        ptr &= 0xffff;

    auto guestCr3 = vmm::GetGuestCR3();

    USHORT limit = 0;
    DWORD64 base = 0;
    EXIT_ERRORS res;

    if (data->f2.is_loading) {
        res = paging::vmmhost::ReadVirtMemoryEx(&limit, (PVOID)ptr, sizeof(limit), vmm::hostCR3, guestCr3);

        if(csAttrib.fields.L) 
            res = paging::vmmhost::ReadVirtMemoryEx(&base, (PVOID)(ptr + 2), 8, vmm::hostCR3, guestCr3);
        else
            res = paging::vmmhost::ReadVirtMemoryEx(&base, (PVOID)(ptr + 2), 4, vmm::hostCR3, guestCr3);

        if (data->f2.is_idtr_access)
        {
            __vmx_vmwrite(GUEST_IDTR_LIMIT, limit);
            __vmx_vmwrite(GUEST_IDTR_BASE, base);
            //pState->gIdtLimit = limit;
            //pState->gIdtBase = base;
        }
        else
        {
            __vmx_vmwrite(GUEST_GDTR_LIMIT, limit);
            __vmx_vmwrite(GUEST_GDTR_BASE, base);
        }
    }
    else {
        if (data->f2.is_idtr_access)
        {
            __vmx_vmread(GUEST_IDTR_LIMIT, (size_t*)&limit);
            __vmx_vmread(GUEST_IDTR_BASE, &base);
            //limit = pState->gIdtLimit;
            //base = pState->gIdtBase;
        }
        else
        {
            __vmx_vmread(GUEST_GDTR_LIMIT, (size_t*)&limit);
            __vmx_vmread(GUEST_GDTR_BASE, &base);
        }

        res = paging::vmmhost::WriteVirtMemoryEx((PVOID)ptr, &limit, sizeof(limit), guestCr3, vmm::hostCR3);

        if (csAttrib.fields.L)
            res = paging::vmmhost::WriteVirtMemoryEx((PVOID)(ptr + 2), &base, 8, guestCr3, vmm::hostCR3);
        else
            res = paging::vmmhost::WriteVirtMemoryEx((PVOID)(ptr + 2), &base, 4, guestCr3, vmm::hostCR3);
    }

    return false;
}

bool VTx::VMExitHandlers::HandleRDMSR(PREGS pContext)
{
    MSR msr = { 0 };

    __try {
        msr.Content = __readmsr(pContext->rcx);
        if (pContext->rcx == IA32_FEATURE_CONTROL)
        {
            msr.Content |= IA32_FEATURE_CONTROL_LOCK_BIT_FLAG;
            msr.Content &= ~(IA32_FEATURE_CONTROL_ENABLE_VMX_INSIDE_SMX_FLAG);
            msr.Content &= ~(IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX_FLAG);
        }
        pContext->rax = msr.Low;
        pContext->rdx = msr.High;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        VTx::Exceptions::InjectException(
            EXCEPTION_VECTOR::EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT,
            vmm::vGuestStates[CPU::GetCPUIndex(true)].lastErrorCode
        );
        return true;
    }

    return false;
}

bool VTx::VMExitHandlers::HandleWRMSR(PREGS pContext)
{
    MSR msr = { 0 };

    msr.Low = (ULONG)pContext->rax;
    msr.High = (ULONG)pContext->rdx;
    if (pContext->rcx == IA32_FEATURE_CONTROL)
    {
        VTx::Exceptions::InjectException(
            EXCEPTION_VECTOR::EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT
        );
        return true;
    }
    __try {
        __writemsr(pContext->rcx, msr.Content);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        VTx::Exceptions::InjectException(
            EXCEPTION_VECTOR::EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT,
            vmm::vGuestStates[CPU::GetCPUIndex(true)].lastErrorCode
        );
        return true;
    }

    vmm::UpdateLastValidTsc();

    return false;
}

BOOLEAN IsValidXcr0(UINT64 Xcr0)
{
    // FP must be unconditionally set.
    if (!(Xcr0 & XCR0_X87)) {
        return FALSE;
    }

    // YMM depends on SSE.
    if ((Xcr0 & XSTATE_MASK_AVX) && !(Xcr0 & XSTATE_MASK_LEGACY_SSE)) {
        return FALSE;
    }

    // BNDREGS and BNDCSR must be the same.
    if ((Xcr0 & XCR0_BNDCSR) != (Xcr0 & XCR0_BNDREGS)) {
        return FALSE;
    }

    // Validate AVX512 xsave feature bits.
    if (Xcr0 & XSTATE_MASK_AVX512) {

        // OPMASK, ZMM, and HI_ZMM require YMM.
        if (!(Xcr0 & XCR0_YMM)) {
            return FALSE;
        }

        // OPMASK, ZMM, and HI_ZMM must be the same.
        if (~Xcr0 & XCR0_ZMM) {
            return FALSE;
        }
    }

    // XCR0 feature bits are valid!
    return TRUE;
}

DWORD64 shadowBV = 0;
//Return true if a #GP needs to be injected
bool VTx::VMExitHandlers::HandleXSetBv(PREGS pContext)
{
    UINT32 Xcr;
    ULARGE_INTEGER XcrValue;

    Xcr = (UINT32)pContext->rcx;

    // Make sure the guest is not trying to write to a bogus XCR.
    //
    switch (Xcr) {
    case XCR0:
        break;

    default:
        return true;
    }

    XcrValue.u.LowPart = (UINT32)pContext->rax;
    XcrValue.u.HighPart = (UINT32)pContext->rdx;

    // Make sure the guest is not trying to set any unsupported bits.
    //
    if (XcrValue.QuadPart & ~XSTATE_MASK_ALLOWED) {
        return true;
    }

    // Make sure bits being set are architecturally valid.
    //
    if (!IsValidXcr0(XcrValue.QuadPart)) {
        return true;
    }

    // By this point, the XCR value should be accepted by hardware.
    //
    pContext->xCtlRegister = XcrValue.QuadPart;

    return false;
}

bool VTx::VMExitHandlers::HandleInvpcid(PREGS pContext)
{
    __try {
        ULONG64 mrsp = 0;
        ULONG64 instinfo = 0;
        ULONG64 qualification = 0;
        __vmx_vmread(VMX_INSTRUCTION_INFO, &instinfo);
        __vmx_vmread(EXIT_QUALIFICATION, &qualification);
        __vmx_vmread(GUEST_RSP, &mrsp);
        PINVPCID pinfo = (PINVPCID)&instinfo;
        ULONG64 base = 0;
        ULONG64 index = 0;
        ULONG64 scale = pinfo->scale ? 2 ^ pinfo->scale : 0;
        ULONG64 addr = 0;
        ULONG64 regopt = ((PULONG64)pContext)[pinfo->regOpt];;
        if (!pinfo->baseInvaild)
        {
            if (pinfo->base == 4)
            {
                base = mrsp;
            }
            else
            {
                base = ((PULONG64)pContext)[pinfo->base];
            }

        }
        if (!pinfo->indexInvaild)
        {
            if (pinfo->index == 4)
            {
                index = mrsp;
            }
            else
            {
                index = ((PULONG64)pContext)[pinfo->index];
            }
        }
        if (pinfo->addrssSize == 0)
        {
            addr = *(PSHORT)(base + index * scale + qualification);
        }
        else if (pinfo->addrssSize == 1)
        {
            addr = *(PULONG)(base + index * scale + qualification);
        }
        else
        {
            addr = *(PULONG64)(base + index * scale + qualification);
        }
        _invpcid(regopt, &addr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return TRUE;
}
