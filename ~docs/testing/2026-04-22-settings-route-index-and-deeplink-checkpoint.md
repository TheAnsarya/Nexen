# Settings Route Index and Deep-Link Checkpoint (2026-04-22)

## Purpose

This document is the settings index for route-driven entry points introduced in #1406.

## Route Index

| Route ID | Target Tab | Notes |
| --- | --- | --- |
| `settings.audio` | `Audio` | Top-level audio configuration |
| `settings.emulation` | `Emulation` | Core emulation behavior |
| `settings.video` | `Video` | Rendering and presentation |
| `settings.input` | `Input` | Generic input tab |
| `settings.input.active-system` | active system tab when ROM loaded; else `Input` | Context-first landing path |
| `settings.preferences` | `Preferences` | General app preferences |
| `settings.system.nes` | `Nes` | NES-specific settings |
| `settings.system.snes` | `Snes` | SNES-specific settings |
| `settings.system.gameboy` | `Gameboy` | Game Boy-specific settings |
| `settings.system.gba` | `Gba` | GBA-specific settings |
| `settings.system.pce` | `PcEngine` | PC Engine-specific settings |
| `settings.system.sms` | `Sms` | SMS-specific settings |
| `settings.system.ws` | `Ws` | WonderSwan-specific settings |
| `settings.system.lynx` | `Lynx` | Lynx-specific settings |
| `settings.system.atari2600` | `Atari2600` | Atari 2600-specific settings |
| `settings.system.channelf` | `ChannelF` | Channel F-specific settings |
| `settings.system.other` | `OtherConsoles` | Other consoles tab |

## Deep-Link Contract

- Route IDs are case-insensitive for resolution.
- Unknown IDs must safely resolve to `Audio`.
- Route constants must be sourced from `ConfigRouteIds`.
- Input context route (`settings.input.active-system`) must remain behavior-stable.

## Regression Guardrails

- Unit tests assert stable mappings and reverse route lookup behavior.
- Main menu settings actions now route via route IDs instead of enum-only calls.
- Future settings entry points should add route IDs before adding menu actions.
