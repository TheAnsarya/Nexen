# Lynx Commercial Bank-Addressing ROM Manifest (2026-03-30)

## Scope

Empirical ROM identity manifest for #1105.

Source corpus:

- `C:\~reference-roms\roms\Atari Lynx (GoodLynx v2.01).rar`

Extraction roots used:

- `C:\~reference-roms\extracted\GoodLynx-v2.01`
- `C:\~reference-roms\extracted\GoodLynx-v2.01\Selected1105`

Selection rule:

- Use canonical `.lnx` entries for the 10 matrix titles.
- Exclude bracket-suffixed alternates (`[b1]`, `[o1]`, etc.) and demo variants.

## Canonical Selected Set

| Title | ROM File | Size (bytes) | SHA-256 |
|---|---|---:|---|
| California Games | California Games (1991).lnx | 131136 | daddc150143a09ae792289ab0f1215fe0955f34bdc09fb595e8cf027340e5200 |
| Chip's Challenge | Chip's Challenge (1989).lnx | 131136 | 5a55f01511e382297f86b073f71f7e1b504762d323a0c7e8ee2bbc1e6633ce45 |
| Klax | Klax (1990).lnx | 262208 | ab61cf96743fd1310b74ca9b340e1bfd19f5a9b7077e2e77f519f7964df707c1 |
| Awesome Golf | Awesome Golf (1991).lnx | 262208 | aa1d5930f70673da746ecd905fc682ace4c1e1258d912f4e2ad8e22f790ad4a0 |
| Warbirds | Warbirds (1990).lnx | 131136 | b8d72524f55ff26fa52dad5251761a7298072efdc38554873da930bebc4eda0c |
| Electrocop | Electrocop (1989).lnx | 131136 | 3718fdcc548fdcfcd78584abfa49f0848caed70249a3257bf77be324f3770246 |
| Zarlor Mercenary | Zarlor Mercenary (1990).lnx | 131136 | e723d5dacf3600210374ab3e27da667d36ecb555842679ddc99bddaecd06e8e1 |
| Blue Lightning | Blue Lightning (1989).lnx | 131136 | 148fcef35ad7985d2ba19a5fe7900ca7194e3267a7b4a3e422b07f9a471e7273 |
| Ninja Gaiden | Ninja Gaiden (1990).lnx | 262208 | ab88a92efe52377d0de7f697291b70247a8ff68d4ffedfd3e57d4f29b04186b1 |
| Gauntlet: The Third Encounter | Gauntlet - The Third Encounter (1990).lnx | 131136 | 21a83624a636fde4d6f6a01363ebd7644dd5c3da19013525aae11ff0aeb2eb60 |

## Current Validation Status

- ROM identity is now pinned for all 10 matrix titles.
- Boot/in-game empirical behavior checks remain pending manual execution in Nexen runtime.
- Any reproduced anomaly must be backlinked to deterministic tests before #1105 closure.
