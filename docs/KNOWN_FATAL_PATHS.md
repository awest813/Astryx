# Known Fatal Paths

This document tracks execution paths that historically lead to hard crashes due to fatal assumptions. All paths documented here must be replaced with structured error returns and graceful fallbacks.

## 1. Resource Management 
- **Null Resources:** Dereferencing resources that failed to load without checking.
- **Failures:** Animation, audio, or video load failures previously caused fatal errors or uncaught exceptions.

## 2. Area & Module Transitions
- **Missing Area Files:** Attempting to transition to an area with missing `.are`, `.git`, or `.gic` files.
- **Bad Transitions:** Incomplete module state updates during mid-transition.

## 3. Scripting Engine
- **Missing Script Functions:** Calling an unimplemented NWScript function resulting in script halt or engine crash.

## 4. Object Handling
- **Invalid Object Handles:** Attempting to read or update objects that have been destroyed in memory (e.g. during combat or area transitions).

## Remediation Requirements
Any path documented here must use `warning()` instead of `error()` (or equivalent engine crash constructs), returning structured error codes or gracefully bailing out where possible.
