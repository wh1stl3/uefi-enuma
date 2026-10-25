# Inherited scanout source

Decode.c adapts the pure snapshot algorithm from [ht94911/uefi-cas](https://github.com/ht94911/uefi-cas/blob/a491522d8f5fecd57be706df1e42e8bd3ca79b20/Platforms/Xiaomi/casPkg/Library/CasDisplayLib/DisplayDecode.c). The casPkg implementation is unchanged from hardware-tested893f5025. Its file BSD-2-Clause-Patent SPDX is retained; the upstream repository license/copyright text is retained in LICENSE.upstream. See sources.json and the enuma repository LICENSE.enuma for adaptation terms.

This component has explicit board bounds, video/command policy and per-pipe local-to-panel origins. Unknown origins are rejected. Capture.c is a callback-only adapter. There is no direct MMIO/pixel pointer, CTL write, refresh timer, GOP publication or allocation. The callback read limit is267; failure preserves the caller snapshot. A successful decode is candidate metadata with all five external requirements still set. Origins must be established from real topology, never guessed LM order. Never clear Requirements just to obtain a GOP.

Capture.c is now offline-only. The video runtime uses VideoCapture.c/Evidence.c,
up to330 reads over three samples with explicit observation masks and no command
refresh, PP or unused pipe accesses. The source-alpha-enabled format is rejected:
Mu Blt does not provide cas's opaque reserved-byte writer. These changes do not
import kernel hardware-control implementation or establish physical GOP success.

NativeObserve.c/.h is a separate unbound diagnostic. It records interface
activity, Timing/Enable stability and native frame movement independently, with
no Enable==3 admission gate. It is not listed in an INF or runtime provider.
See docs/native-display-observe.md; production Evidence.c remains unchanged.
