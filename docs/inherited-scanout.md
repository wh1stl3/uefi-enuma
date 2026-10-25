# Inherited scanout: offline video-mode adaptation

The companion docs/operations/2026-10-25-inherited-gop-offline.md is the authoritative milestone and remaining qualification list.

Platforms/Xiaomi/enumaPkg/Library/InheritedScanoutLib adapts only the pure cas snapshot decoder, with a267-callback-read capture adapter. It has no hardware pointer access, register/pixel write, timer, allocation or GOP publication. No DSC/FDF selects it yet. This is not a restored GOP or execution-qualified firmware image.

The enuma saved default K81 panel is dual-CPHY video mode; per-link timing is800x2560. A read-only Android DRM query reports1600x2560 video modes. These are distinct evidence from actual UEFI scanout. Cas's command-only policy and CTL_START kick are not imported. Width/height/splash bounds/mode and per-pipe local-to-panel origins are explicit inputs; unknown origin masks are refused. Origins must never be inferred merely from LM index.

Successful layout decoding leaves activity, blend/occlusion, CPU address relationship, refresh and full-panel topology requirements set. The backend may not clear those bits to obtain a protocol. Failure clears layout output; capture failure preserves the entire output snapshot. New callback reads remain within relative0..0x73830 and total267; an eventual MMIO caller must separately enforce the approved R-004 aperture and exact DT limits.

The production C has host ASan/UBSan coverage for28 layout scenarios and all267 callback failure positions, plus freestanding AArch64 compile and undefined-symbol checks. Tests use synthetic memory only and write artifacts to the companion out/inherited-scanout directory. See tests/inherited_scanout.c and scripts/check_inherited_scanout.py in the companion.

The next work is the real GOP adapter and its live qualification, reusing existing FrameBufferBltLib rather than importing cas physical writers. Local-to-global dual-link geometry, active/stable route, mapped splash intersection and refresh policy must be closed first. The existing R-004 staging mapping is not an assumption that every new firmware build maps MDP. No new image was built or booted for this library milestone; no approval is being requested by this document.
