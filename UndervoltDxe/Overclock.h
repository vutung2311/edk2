#ifndef __OVERCLOCK_H__
#define __OVERCLOCK_H__

#include <Uefi/UefiBaseType.h>

// SVID fixed VCCIN voltages
#define _DYNAMIC_SVID 0x0 // FIVR-controlled SVID

// Adaptive negative dynamic voltage offsets for all domains
#define _DYNAMIC_FVID 0x0			  // no change to default VID (Vcore)
#define _FVID_MINUS_10_MV 0xFEC00000  // -10 mV  (-0.010 V)
#define _FVID_MINUS_20_MV 0xFD800000  // -20 mV  (-0.020 V)
#define _FVID_MINUS_30_MV 0xFC200000  // -30 mV  (-0.030 V)
#define _FVID_MINUS_40_MV 0xFAE00000  // -40 mV  (-0.040 V)
#define _FVID_MINUS_50_MV 0xF9A00000  // -50 mV  (-0.050 V)
#define _FVID_MINUS_60_MV 0xF8600000  // -60 mV  (-0.060 V)
#define _FVID_MINUS_65_MV 0xF7A00000  // -65 mV  (-0.065 V)
#define _FVID_MINUS_70_MV 0xF7000000  // -70 mV  (-0.070 V)
#define _FVID_MINUS_75_MV 0xF6800000  // -75 mV  (-0.075 V)
#define _FVID_MINUS_80_MV 0xF5C00000  // -80 mV  (-0.080 V)
#define _FVID_MINUS_90_MV 0xF4800000  // -90 mV  (-0.090 V)
#define _FVID_MINUS_95_MV 0xF3E00000  // -95 mV  (-0.095 V)
#define _FVID_MINUS_100_MV 0xF3400000 // -100 mV (-0.100 V)
#define _FVID_MINUS_110_MV 0xF1E00000 // -110 mV (-0.110 V)
#define _FVID_MINUS_120_MV 0xF0A00000 // -120 mV (-0.120 V)
#define _FVID_MINUS_130_MV 0xEF600000 // -130 mV (-0.130 V)
#define _FVID_MINUS_140_MV 0xEE200000 // -140 mV (-0.140 V)
#define _FVID_MINUS_150_MV 0xECC00000 // -150 mV (-0.150 V)

// toolbox for MSR OC Mailbox (experimental)
#define OC_MB_COMMAND_EXEC 0x8000000000000000
#define OC_MB_GET_CPU_CAPS 0x0000000100000000
#define OC_MB_GET_TURBO_RATIOS 0x0000000200000000
#define OC_MB_GET_FVIDS_RATIOS 0x0000001000000000
#define OC_MB_SET_FVIDS_RATIOS 0x0000001100000000
#define OC_MB_GET_SVID_PARAMS 0x0000001200000000
#define OC_MB_SET_SVID_PARAMS 0x0000001300000000
#define OC_MB_GET_FIVR_PARAMS 0x0000001400000000
#define OC_MB_SET_FIVR_PARAMS 0x0000001500000000

#define OC_MB_DOMAIN_IACORE 0x0000000000000000 // IA Core domain
#define OC_MB_DOMAIN_GFX 0x0000010000000000	// GFX domain
#define OC_MB_DOMAIN_CLR 0x0000020000000000	// CLR (CBo/LLC/Ring) a.k.a. Cache/Uncore domain
#define OC_MB_DOMAIN_SA 0x0000030000000000	 // System Agent (SA) domain
#define OC_MB_DOMAIN_AIO 0x0000040000000000	// AIO domain

#define OC_MB_FIVR_FAULTS_OVRD_EN 0x1			   // bit 0
#define OC_MB_FIVR_EFF_MODE_OVRD_EN 0x2			   // bit 1
#define OC_MB_FIVR_DYN_SVID_CONTROL_DIS 0x80000000 // bit 31

#define OC_MB_SUCCESS 0x0
#define OC_MB_REBOOT_REQUIRED 0x7

// Model Specific Registers
#define MSR_IA32_BIOS_SIGN_ID 0x08B
#define MSR_PLATFORM_INFO 0x0CE
#define MSR_OC_MAILBOX 0x150
#define MSR_FLEX_RATIO 0x194
#define MSR_TURBO_RATIO_LIMIT 0x1AD
#define MSR_TURBO_RATIO_LIMIT1 0x1AE
#define MSR_TURBO_RATIO_LIMIT2 0x1AF
#define MSR_UNCORE_RATIO_LIMIT 0x620

// constants
#define AP_EXEC_TIMEOUT 1000000 // 1 second
#define CPUID_BRAND_STRING_BASE 0x80000002
#define CPUID_VERSION_INFO 0x1
#define CPUID_BRAND_STRING_LEN 0x30
#define MSR_FLEX_RATIO_OC_LOCK_BIT 0x100000				 // bit 20, set to lock MSR 0x194
#define MSR_TURBO_RATIO_SEMAPHORE_BIT 0x8000000000000000 // set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1AF
#define MSR_BIOS_NO_UCODE_PATCH 0x0

// build options
#define BUILD_RELEASE_VER L"v1.0"				  // build version
#define BUILD_TARGET_CPU_DESC L"\"Kaby Lake i7\"" // target CPU description (codename)
#define BUILD_TARGET_CPUID_SIGN 0xFFFFFFFF		  // target CPUID, set 0xFFFFFFFF to bypass checking

// driver settings
static const BOOLEAN CPU_SET_FIXED_VCCIN = FALSE;		 // set fixed VCCIN (reboot required)
static const BOOLEAN CPU_SET_OC_LOCK = FALSE;			 // set Overlocking Lock at completion of programming
static const BOOLEAN UNCORE_PERF_PLIMIT_OVRD_EN = FALSE; // disable Uncore P-states (i.e. force max frequency)
static const BOOLEAN FIVR_FAULTS_OVRD_EN = FALSE;		 // disable FIVR Faults
static const BOOLEAN FIVR_EFF_MODE_OVRD_EN = FALSE;		 // disable FIVR Efficiency Mode
static const BOOLEAN FIVR_DYN_SVID_CONTROL_DIS = FALSE;  // disable FIVR SVID Control ("PowerCut"), forces CPU_SET_FIXED_VCCIN = TRUE

// Serial Voltage Identification (SVID) fixed voltages per package, adjust as needed
static const UINT32 SVID_FIXED_VCCIN = _DYNAMIC_SVID;

// Domain 0 (IA Core) dynamic voltage offsets per package, adjust as needed
static const UINT32 IACORE_ADAPTIVE_OFFSET = _FVID_MINUS_150_MV;

// Domain 1 (GFX) dynamic voltage offsets per package, adjust as needed
static const UINT32 GFX_ADAPTIVE_OFFSET = _FVID_MINUS_50_MV;

// Domain 2 (CLR) dynamic voltage offsets per package, adjust as needed
static const UINT32 CLR_ADAPTIVE_OFFSET = _FVID_MINUS_150_MV;

// Domain 3 (SA) dynamic voltage offsets per package, adjust as needed
static const UINT32 SA_ADAPTIVE_OFFSET = _DYNAMIC_FVID;

// Domain 4 (AIO) dynamic voltage offsets per package, adjust as needed
static const UINT32 AIO_ADAPTIVE_OFFSET = _DYNAMIC_FVID;

// function prototypes
BOOLEAN EFIAPI IsValidPackage(VOID);
VOID EFIAPI ProgramPackage(VOID);
BOOLEAN RebootRequired;

UINT64 EFIAPI OC_MAILBOX_DISPATCH(UINT64 ProgramBuffer);
UINT64 EFIAPI OC_MAILBOX_REQUEST(VOID);
BOOLEAN EFIAPI OC_MAILBOX_CLEANUP(UINT64 ResponseBuffer);

#endif