# Epic 21 UI Mockups and Interaction Flows (2026-04-21)

## Mockup A: Onboarding Step 1 (Usage Profile)

```text
+-------------------------------------------------------+
| Nexen Setup                                           |
| Choose your primary use                               |
|                                                       |
| ( ) Playing games                                     |
|     Fast startup, cleaner UI defaults                 |
|                                                       |
| ( ) Debugging and development                         |
|     Pansy/CDL enabled defaults, debugger-first UX     |
|                                                       |
| [Back]                                   [Continue]   |
+-------------------------------------------------------+
```

## Mockup B: Settings Home (Context-First)

```text
+---------------------------------------------------------------+
| Settings                                                      |
| [Active System: SNES] [Switch]                               |
|                                                               |
| Quick Actions                                                 |
| [Configure SNES Controller] [Video Preset] [Audio Latency]   |
|                                                               |
| Categories                                                    |
| > Input and Controllers                                       |
| > Video and Audio                                             |
| > Debugger and Integration                                    |
| > Interface and Accessibility                                 |
+---------------------------------------------------------------+
```

## Mockup C: Speed Slider Flyout

```text
+----------------------------------+
| Emulation Speed                  |
|                                  |
| Pause  [----|----|----|----] Max |
|            100%                  |
|                                  |
| Presets: 50% 75% 100% 150% 200%  |
+----------------------------------+
```

## Flow 1: Input Settings Entry

1. User opens Input settings.
2. App detects active platform from loaded game.
3. App lands directly on active platform controller pane.
4. User can back out to all-platform overview.

## Flow 2: Debugger Settings With Dense Content

1. User opens Debugger settings.
2. Each dense tab uses scroll container by default.
3. Keyboard, mouse wheel, and touch scrolling remain coherent.
4. No clipped controls at smaller window heights.

## Annotations

- Minimum button hit target: 44x44 logical pixels.
- Avoid tab color treatment that reduces readability.
- Disabled state must be visually distinct from normal state.
