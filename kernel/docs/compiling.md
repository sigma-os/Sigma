# Meson Options
Sigma has a few options that disable or enable features at compile time, these should be passed to the meson configure command
This is not a definitive list, that is `meson_options.txt`

- `sigma_compile_ubsan` enables UBSan support

# Internal Defines
Sigma uses a few defines for enabling certain features at compile time, so they don't take up space when they're not used.

These should not be used manually but only serve as a helpful list of what they do

- `SIGMA_UBSAN` Compiles in UBSan support