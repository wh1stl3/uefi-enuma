/*
 * enumaPkg 内存映射 —— 结构沿用 Mu-Silicium SM8250（Kona）移植，DDR/FD/stack 段来自
 * 同 SoC 的 pipa（Xiaomi Pad 6, SM8250-AC）。
 *
 * DDR 段已按 enuma 原厂 XBL 内存图核对：来源为设备 xbl_a 分区（sha256
 * 7ac67cbf4c7870a972cb4ffd9cc09d5db1c96b7f4a5abc7a80c0da9cabc65057）内嵌的
 * uefiplat.cfg [MemoryMap]，逐行引用见 DDR 表内的 "cfg L<n>" 注释。
 * 已由该 cfg 佐证：IPC SHM / MPSS EFS / BOOT INFO / AOP CMD DB / XBL Log Buffer /
 * SMEM / TZApps / PIL Reserved / DXE Heap / Sched Heap / Display Reserved /
 * DBI Dump / FV Region / ABOOT FV / UEFI FD / SEC Heap / CPU Vectors /
 * MMU PageTables / UEFI Stack / Log Buffer / Info Blk / Kernel（cfg L33，
 * sheng/nuwa/nabu/garnet/ingres 也保留该行）。
 * HYP（0x80000000/0x5D0000）：cfg 仅以 [Config] UnusableDDRMemoryAtBeginning 描述
 * （无 [MemoryMap] 行）。长度由 pipa 的 0x600000 缩短到 0x5D0000，结束于 IPC SHM
 * （0x805D0000）以消除重叠（用户 2026-11-07 选定）；HobOption/类型/缓存属性取 sheng
 * hyp-region 的 no-map 预留写法（HobOnlyNoCacheSetting + MEM_RES + Reserv），不映射
 * EfiUnusableMemory。以上两项为 sheng 对照取值，未实机验证。
 *
 * UEFI FD 0x9FC00000/0x300000 与 cfg 一致、同 SoC 的 pipa/alioth/nabu 相同（用户 2026-11-08
 * 确认保持），须与 Resources/Configs/enuma.toml 的 [uefi_fd] 一致（预检检查）。
 *
 * DXE Heap 2（0x93200000/0x05700000）：实验性新增（用户 2026-11-08 当轮确认）。原因：2026-11-08
 * 首次上机日志显示 DXE 从 FeatureEnablerDxe 起连续 AllocatePoolPages 失败（8–94 页成对），后续 59 个
 * FV 模块（TzDxe/GpiDxe/I2C/SPI/DisplayDxe/ButtonsDxe/DDRInfoDxe/ConSplitterDxe/GraphicsConsoleDxe/
 * BdsDxe/RamManagerDxe/SmbiosDxe/Usb*、Charger* 等）加载不进去，最终 DxeCore DxeMain.c(581) ASSERT：
 * DXE 堆 0x98900000/0x03300000（51 MiB，XBL cfg 值）先被 SEC 解压的两份 FV（各约 7.2 MiB）+ DxeCore +
 * 厂商驱动（ShmBridgeDxe 等）耗尽。依据 Android /proc/iomem 的 System RAM 92700000-b03fffff 与 PIL
 * Reserved 末端（0x93200000），本行填补 0x93200000–0x98900000（87 MiB），与 DXE Heap 行合并为
 * 0x93200000–0x9BBFFFFF（约 138 MiB）。属性与 DXE Heap 行一致。属实验：该区间是否含 ABL/TZ 活数据
 * 未证明（未上机验证）；nuwa 同断言是靠改大 DXE 堆解决。
 *
 * PENDING-STAGE4：Kernel 预留（0xA0000000/0x10000000，Reserv）与 [payload.adt]
 * owned_memory_size 窗口的关系待阶段 4 实测：DarwinBoot AllocateWindow 在 EfiConventionalMemory
 * 里分配，失败时屏幕显示 "NO WINDOW <size> BIG RUN <bytes> @ <addr>" 与 "FREE ..." 行，据此核对。
 * 来源：enuma 原厂 XBL/UEFI 内存图、实机 FDT /memory 与 /reserved-memory。
 */
#include <Library/MemoryMapLib.h>

STATIC
EFI_MEMORY_REGION_DESCRIPTOR
gMemoryDescriptor[] = {
  // Name, Address, Length, HobOption, ResourceType, ResourceAttribute, MemoryType, ArmAttribute

  // DDR Regions
  //   All DDR rows are backed by enuma's own XBL uefiplat.cfg [MemoryMap]
  //   (xbl_a sha256 7ac67cbf4c7870a972cb4ffd9cc09d5db1c96b7f4a5abc7a80c0da9cabc65057);
  //   trailing "cfg L<n>" comments cite that file's line number.
  {"HYP",               0x80000000, 0x005D0000, HobOnlyNoCacheSetting, MEM_RES, UNCACHEABLE, Reserv, UNCACHED_UNBUFFERED_XN},
  {"IPC SHM",           0x805D0000, 0x00020000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN}, // cfg L12
  {"MPSS EFS",          0x80600000, 0x00100000, AddMem, SYS_MEM, SYS_MEM_CAP, Reserv, WRITE_BACK_XN}, // cfg L13
  {"BOOT INFO",         0x80700000, 0x00100000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN}, // cfg L14
  {"AOP CMD DB",        0x80860000, 0x00020000, AddMem, MEM_RES, UNCACHEABLE, Reserv, UNCACHED_UNBUFFERED_XN},
  {"XBL Log Buffer",    0x80884000, 0x00010000, AddMem, SYS_MEM, SYS_MEM_CAP, Reserv, WRITE_BACK_XN}, // cfg L16
  {"SMEM",              0x80900000, 0x00200000, AddMem, MEM_RES, UNCACHEABLE, Reserv, UNCACHED_UNBUFFERED_XN},
  {"TZApps",            0x82400000, 0x03A00000, HobOnlyNoCacheSetting, MEM_RES, UNCACHEABLE, Reserv, UNCACHED_UNBUFFERED_XN},
  {"PIL Reserved",      0x86000000, 0x0D200000, AddMem, MEM_RES, UNCACHEABLE, Reserv, UNCACHED_UNBUFFERED_XN}, // cfg L19
  {"DXE Heap 2",        0x93200000, 0x05700000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK_XN}, // experiment (2026-11-08, see header)
  {"DXE Heap",          0x98900000, 0x03300000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK_XN},
  {"Sched Heap",        0x9BC00000, 0x00400000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN}, // cfg L21
  {"Display Reserved",  0x9C000000, 0x02400000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_THROUGH_XN},
  {"DBI Dump",          0x9E400000, 0x00F00000, NoHob,  MMAP_IO, INITIALIZED, Conv,   UNCACHED_UNBUFFERED_XN}, // cfg L23
  {"FV Region",         0x9F800000, 0x00200000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN}, // cfg L24
  {"ABOOT FV",          0x9FA00000, 0x00200000, AddMem, SYS_MEM, SYS_MEM_CAP, Reserv, WRITE_BACK_XN}, // cfg L25
  {"UEFI FD",           0x9FC00000, 0x00300000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"SEC Heap",          0x9FF00000, 0x0008C000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN}, // cfg L27
  {"CPU Vectors",       0x9FF8C000, 0x00001000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK}, // cfg L28
  {"MMU PageTables",    0x9FF8D000, 0x00003000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN}, // cfg L29
  {"UEFI Stack",        0x9FF90000, 0x00040000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN},
  {"Log Buffer",        0x9FFF7000, 0x00008000, AddMem, SYS_MEM, SYS_MEM_CAP, RtData, WRITE_BACK_XN}, // cfg L31
  {"Info Blk",          0x9FFFF000, 0x00001000, AddMem, SYS_MEM, SYS_MEM_CAP, RtData, WRITE_BACK_XN},
  {"Kernel",            0xA0000000, 0x10000000, AddMem, SYS_MEM, SYS_MEM_CAP, Reserv, WRITE_BACK_XN}, // cfg L33
  // The stock kernel's ramoops area (ramoops_memreserve=4M, console zone 0xB0000000/0x200000): RamoopsLogLib keeps
  // the debug output in the console zone. Reserved so neither RamManagerDxe nor the XNU loader hands it out, and
  // mapped uncached so the firmware's device-memory writes (IoLib) reach DDR and survive a PMIC warm reset.
  {"RAMOOPS",           0xB0000000, 0x00400000, AddMem, MEM_RES, UNCACHEABLE, Reserv, UNCACHED_UNBUFFERED_XN},

  // Other Memory Regions
  {"IMEM Base",         0x14680000, 0x00040000, NoHob,  MMAP_IO, INITIALIZED, Conv,   NS_DEVICE},
  {"IMEM Cookie Base",  0x146BF000, 0x00001000, AddDev, MMAP_IO, INITIALIZED, Conv,   NS_DEVICE},

  // Register Regions
  {"IPC_ROUTER_TOP",    0x00400000, 0x00100000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"SECURITY CONTROL",  0x00780000, 0x00007000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"PRNG_CFG_PRNG",     0x00790000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"QUPV3_2_GSI",       0x00800000, 0x000D0000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"QUPV3_0_GSI",       0x00900000, 0x000D0000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"QUPV3_1_GSI",       0x00A00000, 0x000D0000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"CRYPTO0 CRYPTO",    0x01DC0000, 0x00040000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"TCSR_TCSR_REGS",    0x01FC0000, 0x00030000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"GPU_GMU_CX_BLK",    0x02C7D000, 0x00002000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"GPU_CC",            0x02C90000, 0x0000A000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"QUPV3_SSC_GSI",     0x05A00000, 0x000D0000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"PERIPH_SS",         0x08800000, 0x00200000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"USB30_PRIM",        0x0A600000, 0x0011B000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"USB_RUMI",          0x0A720000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"USB30_SEC",         0x0A800000, 0x0011B000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"AOSS",              0x0B000000, 0x04000000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"TLMM_WEST",         0x0F100000, 0x00300000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"TLMM_SOUTH",        0x0F500000, 0x00300000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"TLMM_NORTH",        0x0F900000, 0x00300000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"SMMU",              0x15000000, 0x000D0000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"APSS_HM",           0x17800000, 0x0d981000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}
};

VOID
GetMemoryMap (
  OUT EFI_MEMORY_REGION_DESCRIPTOR **MemoryDescriptor,
  OUT UINT8                         *MemoryDescriptorCount)
{
  // Pass Data
  *MemoryDescriptor      = gMemoryDescriptor;
  *MemoryDescriptorCount = ARRAY_SIZE (gMemoryDescriptor);
}
