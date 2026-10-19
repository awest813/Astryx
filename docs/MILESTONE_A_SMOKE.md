# Milestone A Smoke Verification

This is a fast verification path for the Endar Spire acceptance slice in `MILESTONE.md`.

## Scripted Smoke Check

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File tests/smoke/milestone_a_smoke.ps1 -BuildDir build
```

This runs a compact regression set:
- `test_endar_spire_golden`
- `test_combat`
- `crash_regression`

## Manual Acceptance Walkthrough (Endar Spire)

Use this when validating behavior against live game data:

- [ ] Boot to main menu.
- [ ] Create a character (class, attributes, skills, appearance).
- [ ] Loot the opening footlocker and equip gear from inventory.
- [ ] Walk through Endar Spire without movement softlocks.
- [ ] Complete Trask intro dialogue and verify first sealed door opens.
- [ ] Open the second door independently.
- [ ] Resolve at least one mock combat round (hit/miss + HP change).
- [ ] Reach escape pod and trigger module-exit transition.

### Cinematics and camera (added 2026-10-18)

Check these on the way; note the DLG line or script when one is off.

- [ ] New Game plays the opening movie (`01A`, started by the Endar Spire's
      OnEnter script) once after character creation, without having to
      press ESC, then fades in on Trask's wake-up conversation.
- [ ] Saving in the first Endar Spire module and loading that save does not
      replay the movie or the wake-up conversation, and the footlocker does
      not get a second set of starting gear.
- [ ] Trask's wake-up conversation: the camera frames the speaker over the
      listener's shoulder (not the floor, not 90° off), and cuts per line.
- [ ] Lines with only `[CONTINUE]` advance by themselves when the voice-over
      ends; lines with choices wait for you.
- [ ] Participants play their talk/gesture animations during lines.
- [ ] Static cutscene cameras (fixed shots of a room) point at the action.
- [ ] Animated camera shots (if any play) move smoothly; if a shot looks
      wrong, note the conversation's `CameraModel` and the line.
- [ ] After every conversation/cutscene the gameplay camera returns behind
      the party at the normal distance, pitch and field of view.
- [ ] Scripted walk-ins during conversations finish before the
      conversation continues (ActionPauseConversation).
- [ ] Tutorial pop-ups appear once each, outside conversations, and close
      with OK. The movement-keys tutorial (from `k_pend_pctut`) should be
      the first; if a pop-up shows the wrong text, note its number from the
      log (`--debug=ELogic:1`, "ShowTutorialWindow: N").
- [ ] Actors placed on waypoints by scripts face the right way.

## Scope Guard

For this smoke pass, triage only:
- `crash`
- `progression blocker`

for the supported Endar Spire path (`end_m01aa`). Defer logic/visual/parity/QoL issues unless they block progression.
