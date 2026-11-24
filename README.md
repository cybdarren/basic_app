
# same54_blinky

## Getting Started

- Press **Ctrl + Shift + B** to build the project.
- Open the Command Palette (**Ctrl + Shift + P**) and search for "MPLAB" to discover other available commands.
- Need more help? Visit the [MPLAB support site](https://www.microchip.com/en-us/support/all) or the [MPLAB website](https://www.microchip.com).
- Found a bug or have feedback? Open the Command Palette (**Ctrl + Shift + P**) and run
  **MPLAB: Report Issue** to let us know.

## Watchdog / fuse settings

The WDT is controlled by the NVM User Page fuses, set in
`config.mcc/src/config/default/initialization.c`:

```
#pragma config WDT_ENABLE   = CLEAR   // WDT disabled  (User Word 1, bit 16)
#pragma config WDT_ALWAYSON = CLEAR   // not forced on (User Word 1, bit 17)
```

The build encodes these correctly: `out/same54_blinky/default.hex` contains
User Word 1 (`0x00804004`) = `0x2AA80080` (WDT off).

### If the board resets about every 32 s

That is the watchdog firing. An erased/default User Page reads `0xFFFFFFFF`,
which is WDT enabled + ALWAYSON + window mode (PER = WINDOW = CYC16384 ≈ 32 s).
Two things both have to be true to clear it:

1. **The User Page must actually be programmed.** A plain flash download over
   the on-board EDBG does **not** write it (a SAM E5x chip-erase leaves the
   User Page untouched). Program it once with **MPLAB IPE**: connect the EDBG,
   load `out/same54_blinky/default.hex`, and **Program** — IPE writes the
   Configuration/User Page region. (OpenOCD's `atsame5 userpage` can also write
   it directly.)

2. **Then fully power-cycle the board.** The WDT/BOD fuses are latched into the
   peripheral only at **power-on reset**. A reset button, a reprogram, an IDE
   "device clear", and even the WDT reset itself are all *warm* resets and do
   **not** reload the User Page — so a freshly programmed fuse does nothing
   until power is physically removed and reapplied. Unplug USB for a few
   seconds, replug, and the watchdog stays off across subsequent warm resets.

A software disable in firmware cannot help once the running peripheral has
loaded ALWAYSON from an erased page: ALWAYSON is read-only-locked until a POR
reloads the (now disabled) fuse. So the fix is fuse + power-cycle, not code.

## Project Structure

- 📦 `_build` — The [CMake build tree](https://cmake.org/cmake/help/latest/manual/cmake.1.html#introduction-to-cmake-buildsystems); safe to delete, regenerated on next build.
- 📦 `cmake` — Generated [CMake](https://cmake.org/) files; can be deleted if `user.cmake` has not been added.
- 🚀 `out` — Final build artifacts.
- ⚙️ `.vscode` — VS Code workspace configuration; see [VS Code docs](https://code.visualstudio.com/docs/getstarted/settings).
- ⚙️ `.vscode\settings.json` — Workspace-specific settings.
- 📝 `.vscode\same54_blinky.mplab.json` — The MPLAB project file. ❗ Should not be deleted.
