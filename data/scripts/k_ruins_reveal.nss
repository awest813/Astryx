// ancient_ruins_reveal.nss
// Script for the Ancient Ruins reveal sequence on Dantooine.
// Uses custom cinematic natives implemented in Phase 6.3.

void main() {
    object oPC       = GetFirstPC();
    object oEntrance = GetObjectByTag("ancient_ruins_entrance");
    object oCamStart = GetObjectByTag("ruins_cam_start");
    object oCamMid   = GetObjectByTag("ruins_cam_mid");
    object oCamEnd   = GetObjectByTag("ruins_cam_end");

    // 1. One-time guard
    if (GetGlobalBoolean("ANCIENT_RUINS_REVEAL_DONE")) {
        return;
    }

    // 2. Safety checks
    if (!GetIsObjectValid(oPC) || !GetIsObjectValid(oEntrance)) {
        return;
    }
    if (!GetIsObjectValid(oCamStart) || !GetIsObjectValid(oCamEnd)) {
        return;
    }

    SetGlobalBoolean("ANCIENT_RUINS_REVEAL_DONE", TRUE);

    // 3. Freeze normal gameplay
    SetPlayerInputEnabled(FALSE);
    SetCutsceneMode(TRUE);
    ClearAllActions(oPC);

    // 4. SHOT 1: Wide establishing shot
    SetCameraMode(3, oCamStart); // CAMERA_MODE_WIDE = 3
    SetCameraTarget(oEntrance);
    CameraTransitionToTarget(1.5);

    // 5. Build anticipation with audio
    DelayCommand(1.6, PlayMusicStinger("mysterious_reveal"));

    // 6. SHOT 2: Slow sweep toward the ruins
    DelayCommand(1.8, CameraMoveAlongPath(oCamStart, oCamMid, 2.6));
    DelayCommand(1.8, SetCameraTarget(oEntrance));

    // 7. SHOT 3: Push closer for emphasis
    DelayCommand(4.6, SetCameraMode(3, oCamMid));
    DelayCommand(4.6, CameraMoveAlongPath(oCamMid, oCamEnd, 1.8));

    // 8. SHOT 4: Hold on the entrance
    DelayCommand(6.6, CameraHold(1.5));

    // 9. SHOT 5: Optional reaction shot on the player
    DelayCommand(8.2, SetCameraMode(0, oPC); // CAMERA_MODE_CLOSEUP = 0 in some presets, but check my enum
    // Actually kCameraModeCloseup was 0 in my types.h update.
    DelayCommand(8.2, CameraLookAtObject(oEntrance, 0.8));

    // 10. Restore normal gameplay
    DelayCommand(10.0, RestoreGameplayCamera(1.0));
    DelayCommand(10.0, SetCutsceneMode(FALSE));
    DelayCommand(10.0, SetPlayerInputEnabled(TRUE));
}
