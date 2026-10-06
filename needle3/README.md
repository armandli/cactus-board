# Needle 3

Prebuilt [Cactus Needle 3](https://cactuscompute.com/needle) engine used to turn English
commands into board tool calls. Fetched from Hugging Face
([Cactus-Compute/needle3](https://huggingface.co/Cactus-Compute/needle3)) by:

```sh
make needle
```

Platform is auto-detected (`macos-arm64`, `linux-x86_64`, etc.).

| File | Source | Purpose |
|------|--------|---------|
| `needle.h` | `<platform>/needle.h` | C API (`needle_load`, `needle_init`, `needle_complete`, ...) |
| `libneedle.a` | `<platform>/libneedle.a` | Static engine, linked as `needle::needle` (see `cmake/Needle.cmake`) |
| `needle3.cact` | `needle3.cact` | Model weights (~35 MB), loaded at runtime |

These files are gitignored. Override the weights path at runtime with `NEEDLE_MODEL=/path/to/model.cact`.
