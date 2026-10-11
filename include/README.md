# include/

PlatformIO adds this directory to the include path of every environment.

| File | Purpose |
| --- | --- |
| `secrets.example.h` | Template for Wi-Fi credentials and the webhook URL. Copy it to `secrets.h`. |
| `secrets.h` | **Git-ignored.** Your real credentials. Optional: without it the firmware runs offline. |

Project headers live next to their sources under `src/` and are included with
layer-qualified paths such as `#include "core/BrewSession.h"`.
