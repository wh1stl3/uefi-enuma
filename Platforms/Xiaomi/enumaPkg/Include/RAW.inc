# enuma 厂商 RAW 文件（自由格式 FREEFORM 段）。
#
# 依据：enuma 实机 xbl_a 导出的 9 个 RAW 配置（GUID 与 RAW.inc 全部吻合），位于
# Binaries/enuma 的 RawFiles 目录。原 pipa 派生的 37 个面板 XML 全部位于
# `!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1` 块内，而此前 USE_CUSTOM_DISPLAY_DRIVER = 0
# （使用 SimpleFbDxe），面板资源被屏蔽、构建不需要，已整体移除（NO-SRC，不伪造）。
#
# enuma 面板 XML（实验分支）：两个面板 XML 取自 enuma 实机 xbl_a（FILE GUID / UI 名
# 见 EnumaInputs/Vendor.raw.fdf.inc）。USE_CUSTOM_DISPLAY_DRIVER = 1 时随 DisplayDxe
# 进 FV，供 DisplayDxe 按板 ID/面板 ID 选屏；默认 =0 时仍屏蔽。
#
!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  FILE FREEFORM = 2962BFBA-D7E7-4F37-A4C6-E28F7B5A685A {
    SECTION UI = "Panel_K81_42_02_0a_dualdsi_cphy_lcd_vid.xml"
    SECTION RAW = Binaries/enuma/RawFiles/Panel_K81_42_02_0a_dualdsi_cphy_lcd_vid.xml
  }

  FILE FREEFORM = C3F9FA09-0B18-B0F8-7AA7-10F761A6C699 {
    SECTION UI = "Panel_K81_35_02_0b_dualdsi_cphy_lcd_vid.xml"
    SECTION RAW = Binaries/enuma/RawFiles/Panel_K81_35_02_0b_dualdsi_cphy_lcd_vid.xml
  }
!endif

  FILE FREEFORM = A1E235DE-E825-4591-9623-C43175811826 {
    SECTION UI = "SecParti.cfg"
    SECTION RAW = Binaries/enuma/RawFiles/SecParti.cfg
  }

  FILE FREEFORM = 21E9BDD9-6C3F-4F10-84A5-BBEC322741F1 {
    SECTION UI = "uefipil.cfg"
    SECTION RAW = Binaries/enuma/RawFiles/uefipil.cfg
  }

  FILE FREEFORM = 45FE4B7C-150C-45DA-A021-4BEB2048EC6F {
    SECTION UI = "QcomChargerCfg.cfg"
    SECTION RAW = Binaries/enuma/RawFiles/QcomChargerCfg.cfg
  }

  FILE FREEFORM = F780C779-DD7C-47CD-BD1A-5EB414C51704 {
    SECTION UI = "BATTERY.PROVISION"
    SECTION RAW = Binaries/enuma/RawFiles/BATTERY.PROVISION
  }

  FILE FREEFORM = 2580262C-6CFD-4BF0-BDD9-2CE2BA612FB9 {
    SECTION UI = "GY.BATTERY.PROVISION"
    SECTION RAW = Binaries/enuma/RawFiles/GY.BATTERY.PROVISION
  }

  FILE FREEFORM = B538D218-A2A5-4073-A725-8D9AE349D041 {
    SECTION UI = "NVT.BATTERY.PROVISION"
    SECTION RAW = Binaries/enuma/RawFiles/NVT.BATTERY.PROVISION
  }

  FILE FREEFORM = 984FBC3C-3860-4282-BAF5-87513350E29D {
    SECTION UI = "BATTERY.PROVISION1"
    SECTION RAW = Binaries/enuma/RawFiles/BATTERY.PROVISION1
  }

  FILE FREEFORM = 73315170-E849-4E57-BD96-F9107B85B1AE {
    SECTION UI = "BATTERY.PROVISION2"
    SECTION RAW = Binaries/enuma/RawFiles/BATTERY.PROVISION2
  }

  FILE FREEFORM = FC41D4C6-31F0-450D-A666-FE4738C20F8A {
    SECTION UI = "SUNWODA.M82.BATTERY.PROVISION"
    SECTION RAW = Binaries/enuma/RawFiles/SUNWODA.M82.BATTERY.PROVISION
  }
