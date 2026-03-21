# Crash Triage Matrix

This document defines the central crash triage matrix for the engine. It categorizes the primary crash types, potential causes, and graceful fallbacks required.

## Triage Matrix

| Crash Type | Common Triggers | Defensive Strategy | Graceful Fallback |
| :--- | :--- | :--- | :--- |
| **Boot Crash** | Missing configuration, unsupported opengl context, missing base archives | Structured error returns in main bootstrap; check archives before reading. | Display user-friendly error dialog/log and exit cleanly. |
| **Load Crash** | Corrupt/missing BIF or KEY files, invalid ERF formatting | Guardrails around `null` resource reads and failed loads. | Log failure, fallback to default texture/model, or abort load cleanly to main menu. |
| **Menu Crash** | Missing UI templates, invalid font files | Validate UI element pointers and font handles before dereferencing. | Render default text/shapes or disable the faulty UI element without crashing. |
| **Area-Entry Crash** | Missing `.are`/`.git`/`.gic` files, bad module transitions | Guard on module transition states, validate area files before entering. | Cancel transition, log warning, and return to main menu or previous state. |
| **Combat Crash** | Invalid object handles, missing animation resources | Verify object handles (source/target). Guard animation/sound loads. | Cancel the action, drop the combat turn gracefully with log, play default animation. |
| **Dialogue Crash** | Missing `.dlg` files, broken script conditions | Guard on `.dlg` file read. Handle missing conditional scripts as `false`. | End dialogue gracefully via fallback exit node, avoid script halting. |
| **Save/Load Crash**| Corrupt save file, missing version data | Validate save state chunk formats before applying. | Abort save load, log warning, return to menu with "Corrupt Save" message. |
