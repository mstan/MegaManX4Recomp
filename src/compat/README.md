# Native text compatibility helper

`native_text.h`, `native_text.cpp` and `tests/test_native_text.cpp` retain the
pure native-text implementation from psxrecomp revision
`9868d30c604d146c9a150d01f60cb1bf4025d180`, the framework revision used by X4's
preserved source. The original paths are `runtime/include/native_text.h`,
`runtime/src/native_text.cpp` and `runtime/tests/test_native_text.cpp`.

The parser comes from `runtime/third_party/json11` at that same revision.
Only the namespaces (`Mmx4NativeText`, `Mmx4Json11`) and include paths change.
Catalog parsing,
UTF-8 validation, native glyph/control encoding, wrapping and indexed-font
composition retain their original algorithms. They use standard C++ and the
original json11 parser, retained in `json11/`. There are no runtime hooks or
optional platform dependencies. A title namespace also avoids duplicate symbols
when building against a framework that still provides its original helper.

The runtime, optional co-simulation target and localization asset test compile
this same implementation. Both helper files participate in X4's existing co-op
compatibility fingerprint along with the parser sources. The localization
feature and its resource checks
remain enabled and unchanged. Shared `text_xlate` implements a separate guest
string hook API and is not a replacement for this authoring utility.

The native-text helper retains the original framework's PolyForm Noncommercial
1.0.0 license (see the pinned framework's `LICENSE`). json11 retains its Dropbox
MIT notice in `json11/LICENSE.txt`; release packaging includes that notice as
`licenses/json11-LICENSE.txt`.

With an ordinary configured X4 build, build `mmx4_native_text_test` and
`mmx4_localization_assets_test`, then run CTest with the exact expression
`^(mmx4_native_text_test|mmx4_localization_assets_test)$` and `--no-tests=error`.
The pure text test checks catalogs, malformed UTF-8, line/page controls,
unsupported glyphs and Spanish/Portuguese font composition. The no-argument
asset test checks archive/movie-pack structure; real-disc language combinations
and native visual/audio playback remain separate existing qualifications.
