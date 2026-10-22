# Early Metronome service

The fixed OEM XBL APRIORI lists MetronomeDxe at ordinal 8, before ArmTimerDxe
and hardware drivers including UFSDxe (OEM ordinal 35). The previous enuma
APRIORI included Timer but delayed the already-packaged Mu Metronome until the
normal dependency scan after all vendor APRIORI entries.

In fixed Mu_Basecore cfb93724, `MdeModulePkg/Core/Dxe/Misc/Stall.c:57`
returns EFI_NOT_AVAILABLE_YET when gMetronome is NULL. The real
`EmbeddedPkg/MetronomeDxe` has TRUE dependency and installs the architectural
protocol; its WaitForTick uses the existing TimerLib.

`patches/apriori-metronome.patch` moves that existing driver into early
dispatch before Timer and vendor entries. It only patches the ignored staging
copy, preserving the original migration file. No stub, MMIO constant, memory
map or additional vendor image is introduced. All original INF entries retain
their relative order. Actual ordinals after the insertion shift by one.

This corrects a static early-service gap. It is a hypothesis for the observed
UFSDxe boundary until before/after hardware probes demonstrate a change; it
must not be reported as a proven UFS or GOP fix from source checks alone.
