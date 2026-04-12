# NWScript for KotOR 1: Functions, Behavior, and Reimplementation Guide for an AI Coder

## Purpose

This document is a practical handoff for an AI coding agent that needs to understand **NWScript in Star Wars: Knights of the Old Republic (KotOR 1)**, especially for:
- quest logic
- area and module events
- conversation-driven scripting
- inventory and merchant logic
- globals, locals, and journal state
- what must be handled in engine code versus script code
- how to reimplement a usable KotOR-style NWScript runtime

This is **not** a byte-perfect language spec. It is an implementation-oriented engineering guide.

---

## 1. What NWScript Is

**NWScript** is BioWare’s C-like gameplay scripting language used in games such as Neverwinter Nights and later in games including **KotOR 1** and **KotOR 2**. In KotOR, scripts are usually written as `.nss` source files and compiled into `.ncs` bytecode files. The language is used to drive gameplay events, quest progression, item rewards, scripted object actions, and conditional logic.  
KotOR uses a game-specific variant of the broader NWScript ecosystem, so not every function known from Neverwinter Nights is available or behaves identically in KotOR.

### Mental model

Think of NWScript in KotOR as:
- a **small embedded gameplay language**
- compiled to **bytecode**
- run by an **engine VM**
- operating on **engine objects** such as creatures, items, doors, placeables, triggers, areas, and modules

The script layer controls game rules and event responses, while the engine owns:
- save/load files
- rendering
- pathfinding
- object lifetime rules
- UI and menus
- low-level game state serialization

---

## 2. Core File Types

### `.nss`
Human-readable NWScript source file.

### `.ncs`
Compiled NWScript bytecode file.

### `nwscript.nss`
The include/reference script containing declarations for many built-in engine functions and constants used by KotOR-style scripting tools and compilers.

### Common script placement/use
Scripts are attached to:
- module events
- area events
- trigger events
- placeables and doors
- conversation nodes
- creature heartbeat / perception / death logic
- item activation or item event scripts

---

## 3. How Scripts Function at Runtime

A typical NWScript execution flow in KotOR is:

1. An engine event happens.
2. The game identifies the script bound to that event.
3. The VM runs that script.
4. The script reads engine state and/or changes engine state.
5. The script exits.

### Typical event sources
- `OnEnter`
- `OnExit`
- `OnHeartbeat`
- `OnUsed`
- `OnOpen`
- `OnClosed`
- `OnDeath`
- `OnSpawn`
- trigger enter/exit
- dialogue action scripts
- dialogue conditional scripts

### Important implication
Scripts are usually **short-lived event handlers**, not constantly running programs. Delayed behavior is typically expressed through engine helpers such as delayed commands rather than long-running loops.

---

## 4. The Most Important Runtime Concepts

## 4.1 Objects

Scripts work with object handles such as:
- creatures
- player characters
- party members
- doors
- placeables
- waypoints
- triggers
- stores/merchants
- modules and areas
- items

Common object-oriented operations include:
- finding objects by tag
- getting the entering or triggering object
- checking validity
- assigning commands to a target object
- reading/writing local state on that object

---

## 4.2 Globals

Globals are named values stored at module/game scope and commonly used for:
- plot progression
- quest flags
- one-time events
- branch state
- world-state changes

Typical categories:
- global booleans
- global numbers/ints
- global strings

Examples:
- `TARIS_SWOOP_RACE_DONE`
- `BASTILA_RESCUE_STAGE`
- `PLAYER_LAST_PLANET`

---

## 4.3 Locals

Locals are values attached to a specific object and used for:
- one-time initialization
- object-specific flags
- per-instance state
- cooldowns
- merchant setup guards
- door or placeable state

Examples:
- `INV_INIT` on a store object
- `HAS_TRIGGERED_CUTSCENE` on a trigger
- `LOCK_BYPASSED` on a door

---

## 4.4 Journal state

Journal entries are a major part of KotOR progression. Scripts often update the quest journal when an important event happens.

Typical uses:
- advance a quest
- close out a quest branch
- expose a new objective
- reflect major story progress

---

## 4.5 Commands and actions

NWScript often works by asking an object to perform an action, or by scheduling work.

Examples:
- tell a creature to walk
- tell a door to open
- destroy an object later
- spawn an item
- delay an operation

---

## 5. Practical Function Families in KotOR 1

The exact function list depends on the KotOR variant and compiler environment, but the following families are the most important to support first in a reimplementation.

## 5.1 Global state functions

Typical examples:
```c
int GetGlobalBoolean(string sName);
void SetGlobalBoolean(string sName, int bValue);

int GetGlobalNumber(string sName);
void SetGlobalNumber(string sName, int nValue);

string GetGlobalString(string sName);
void SetGlobalString(string sName, string sValue);
```

### What they do
These read and write named global values.

### Why they matter
These are the backbone of:
- quest progression
- plot gating
- one-time world changes
- cross-area state sharing

### Engine notes for reimplementation
Implement globals as a persistent dictionary attached to saveable game/module state.

Suggested internal shape:
```txt
globals:
  booleans: map<string, bool>
  numbers: map<string, int>
  strings: map<string, string>
```

---

## 5.2 Local state functions

Typical examples:
```c
int GetLocalBoolean(object oObj, string sName);
void SetLocalBoolean(object oObj, string sName, int bValue);

int GetLocalInt(object oObj, string sName);
void SetLocalInt(object oObj, string sName, int nValue);

string GetLocalString(object oObj, string sName);
void SetLocalString(object oObj, string sName, string sValue);

object GetLocalObject(object oObj, string sName);
void SetLocalObject(object oObj, string sName, object oValue);
```

### What they do
Attach arbitrary script-visible state to a specific object.

### Why they matter
These are essential for:
- one-time initialization
- per-object state tracking
- temporary references
- merchant inventory setup guards
- object-specific quest flags

### Engine notes for reimplementation
Every scriptable object should have a local variable table.

Suggested internal shape:
```txt
object.locals:
  ints: map<string, int>
  floats: map<string, float>
  strings: map<string, string>
  objects: map<string, ObjectId>
  locations: map<string, Location>
```

---

## 5.3 Object lookup and context functions

Typical examples:
```c
object OBJECT_SELF;
object GetObjectByTag(string sTag, int nNth = 0);
object GetEnteringObject();
object GetExitingObject();
object GetLastUsedBy();
object GetNearestObjectByTag(string sTag, object oTarget = OBJECT_SELF);
int GetIsObjectValid(object oObj);
```

### What they do
Let scripts find the objects they need and understand event context.

### Why they matter
Without these, most event scripts cannot function.

### Engine notes for reimplementation
You need:
- stable runtime object IDs
- a tag index
- event context bindings
- a null/invalid object sentinel

---

## 5.4 Journal and quest functions

Typical example:
```c
void AddJournalQuestEntry(string sQuestTag, int nState, int bAllowOverride = FALSE);
```

### What it does
Advances or updates quest journal state.

### Why it matters
KotOR quest flow is heavily journal-driven.

### Engine notes for reimplementation
This should update:
- current quest state
- quest visibility if needed
- completion status if the state marks a finishing branch
- saveable journal data

Suggested shape:
```txt
journal:
  quests:
    quest_tag:
      current_state: int
      discovered: bool
      completed: bool
```

---

## 5.5 Inventory and item functions

Typical examples:
```c
object CreateItemOnObject(string sResRef, object oTarget, int nStackSize = 1);
object GetItemPossessedBy(object oCreature, string sItemTag);
void DestroyObject(object oObject, float fDelay = 0.0);
```

### What they do
Create, find, or destroy item/object instances.

### Why they matter
These are critical for:
- quest rewards
- loot generation
- merchant inventory setup
- scripted object replacement

### Engine notes for reimplementation
You need a distinction between:
- a **template/resource** item definition
- a **runtime instance** item object

When `CreateItemOnObject()` is called:
1. resolve template by resref
2. instantiate runtime item
3. attach it to the target inventory/container
4. return the new object handle

---

## 5.6 Script execution helpers

Typical examples:
```c
void ExecuteScript(string sScript, object oTarget);
void DelayCommand(float fSeconds, action aAction);
void AssignCommand(object oObj, action aAction);
```

### What they do
These allow scripts to:
- invoke other scripts
- schedule delayed work
- ask other objects to perform actions

### Why they matter
These functions make event-driven scripting practical.

### Engine notes for reimplementation
These require:
- script loading by name
- action queue support
- event scheduler/timer wheel or equivalent

For a first usable implementation:
- `ExecuteScript()` can be direct.
- `DelayCommand()` can enqueue a closure-like VM command record.
- `AssignCommand()` can push behavior onto the target object’s action queue.

---

## 5.7 Utility/control functions

Typical examples:
```c
void SpeakString(string sText);
void SendMessageToPC(object oPC, string sText);
int Random(int nMax);
```

These are not the deepest systems, but they are useful for debugging and for gameplay scripting quality of life.

---

## 6. Merchant Inventory Persistence: What Script Can Do vs What Engine Must Do

This is a very common trap.

## What script can do
A script can prevent duplicate initialization by setting a local or global flag:

```c
void main() {
    object oStore = OBJECT_SELF;

    if (GetLocalBoolean(oStore, "INV_INIT")) {
        return;
    }

    SetLocalBoolean(oStore, "INV_INIT", TRUE);

    CreateItemOnObject("medpac", oStore, 5);
    CreateItemOnObject("g_w_blstrrfl001", oStore, 1);
}
```

This is enough for:
- first-time stock setup
- basic one-time restock rules
- quest-based unlock stock

## What script cannot guarantee by itself
If your engine reconstructs merchant inventories from blueprints/templates each load, then script-level guards alone will not preserve exact inventory state.

## Engine requirement
To make merchant inventories truly persistent, your save/load system must serialize:
- the merchant/store runtime inventory contents
- stack counts
- item instance state
- equipment if applicable
- any local variables that influence inventory generation

---

## 7. Save/Load: What Belongs to the Engine

NWScript is **not** the full save system.

The engine must own serialization for:
- creature stats
- HP/FP/current condition
- equipped items
- inventory contents
- known powers/feats
- journal states
- global variables
- object local variables
- doors/placeables state
- area/module state
- spawned/despawned instance identity
- active party composition
- save metadata and save slot UI

### Clean rule
If the player should quit the game and resume later with the same world state, the engine must serialize that state.

The script layer should only be responsible for:
- setting/changing gameplay values
- querying current state
- reacting to events

---

## 8. Recommended Internal Architecture for Reimplementation

## 8.1 Layer split

### Layer A: resource layer
Responsible for:
- reading `.nss`, `.ncs`, templates, 2DA/UTI/UTC/etc. as needed
- compiling or loading scripts
- resolving resrefs and tags

### Layer B: VM layer
Responsible for:
- script bytecode execution
- stack, locals, globals, parameters
- calling native engine functions
- maintaining script context (`OBJECT_SELF`, event source, etc.)

### Layer C: game binding/native API layer
Responsible for implementing native functions such as:
- `GetGlobalBoolean`
- `CreateItemOnObject`
- `AddJournalQuestEntry`
- `GetObjectByTag`

### Layer D: simulation layer
Responsible for:
- objects and inventories
- areas/modules
- action queues
- AI/behavior hooks
- delayed commands

### Layer E: persistence layer
Responsible for:
- full save serialization
- load reconstruction
- object ID remapping
- restoring globals, locals, journal, inventories, creatures, and areas

---

## 8.2 Suggested VM-native call interface

Model script built-ins as native calls with a common signature.

Pseudo-interface:
```cpp
struct ScriptValue {
    enum Type { Int, Float, String, Object, Location, Effect, Void };
    // payload...
};

using NativeFn = ScriptValue(*)(ScriptVM& vm, const std::vector<ScriptValue>& args);
```

Then register:
```cpp
RegisterNative("GetGlobalBoolean", Native_GetGlobalBoolean);
RegisterNative("SetGlobalBoolean", Native_SetGlobalBoolean);
RegisterNative("CreateItemOnObject", Native_CreateItemOnObject);
RegisterNative("AddJournalQuestEntry", Native_AddJournalQuestEntry);
```

---

## 8.3 Object identity model

Use stable runtime IDs.

Suggested:
```txt
ObjectId = uint64
```

Objects should expose:
- type
- tag
- resref/template origin
- area/module ownership
- local variables
- inventory/equipment
- transform/location
- active state flags

Never let scripts hold raw pointers directly. Let them hold object handles or IDs that resolve through the world state.

---

## 8.4 Event context model

Each script invocation should receive a context object, for example:

```cpp
struct ScriptContext {
    ObjectId objectSelf;
    ObjectId triggeringObject;
    ObjectId enteringObject;
    ObjectId exitingObject;
    ObjectId lastUsedBy;
    ObjectId moduleObject;
    ObjectId areaObject;
};
```

Many native calls should reference this context.

---

## 9. Minimal Feature Set to Implement First

If the goal is "scripts that feel meaningfully KotOR-like," implement in this order:

## Phase 1: basic VM + data types
- ints
- floats
- strings
- objects
- function calls
- return values
- conditionals
- basic arithmetic/comparison

## Phase 2: context and object lookup
- `OBJECT_SELF`
- `GetObjectByTag`
- `GetEnteringObject`
- `GetExitingObject`
- `GetIsObjectValid`

## Phase 3: globals and locals
- `Get/SetGlobalBoolean`
- `Get/SetGlobalNumber`
- `Get/SetGlobalString`
- `Get/SetLocalBoolean`
- `Get/SetLocalInt`
- `Get/SetLocalString`
- `Get/SetLocalObject`

## Phase 4: inventory/journal
- `CreateItemOnObject`
- `GetItemPossessedBy`
- `DestroyObject`
- `AddJournalQuestEntry`

## Phase 5: script flow helpers
- `ExecuteScript`
- `AssignCommand`
- `DelayCommand`

## Phase 6: save/load integration
- serialize globals
- serialize object locals
- serialize journal
- serialize inventories and equipment
- restore script-visible object identity

This phase order gives maximum practical value quickly.

---

## 10. Common Implementation Pitfalls

## Pitfall 1: treating script persistence as save persistence
Wrong:
- "I set globals and locals, so I have save/load."

Correct:
- globals and locals are only persistent if the engine serializes them.

## Pitfall 2: no stable object identity
If object IDs are unstable across load, saved local-object references and script lookups may break.

## Pitfall 3: tag lookup only
Tags are useful, but not enough. Runtime object instances need real IDs.

## Pitfall 4: reconstructing inventories from templates on load
This breaks merchant persistence and item state.

## Pitfall 5: no delayed command scheduler
A lot of script behavior expects delayed execution.

## Pitfall 6: making natives too literal too early
Build a clean host API first, then refine edge-case fidelity.

---

## 11. Example Script Patterns the Engine Should Support

## 11.1 One-time quest trigger
```c
void main() {
    if (GetGlobalBoolean("QUEST_FOO_STARTED")) {
        return;
    }

    SetGlobalBoolean("QUEST_FOO_STARTED", TRUE);
    AddJournalQuestEntry("quest_foo", 10);
}
```

## 11.2 Merchant initialization
```c
void main() {
    object oStore = OBJECT_SELF;

    if (!GetLocalBoolean(oStore, "INV_INIT")) {
        SetLocalBoolean(oStore, "INV_INIT", TRUE);
        CreateItemOnObject("medpac", oStore, 5);
        CreateItemOnObject("g_w_blstrrfl001", oStore, 1);
    }
}
```

## 11.3 Delayed object removal
```c
void main() {
    object oTarget = GetObjectByTag("temp_npc");
    if (GetIsObjectValid(oTarget)) {
        DestroyObject(oTarget, 2.0);
    }
}
```

## 11.4 Simple plot gate
```c
void main() {
    if (GetGlobalNumber("PLOT_STAGE") >= 30) {
        SpeakString("The path is open.");
    } else {
        SpeakString("You are not ready.");
    }
}
```

---

## 12. Suggested Save Format Contents

Your save file should include at minimum:

```txt
SaveGame
  metadata
    save_name
    timestamp
    area_name
    play_time
    screenshot_ref (optional)

  globals
    booleans
    numbers
    strings

  journal
    quests and states

  party
    members
    stats
    powers
    inventory
    equipment

  world
    modules
    areas
    placed objects
    object locals
    destroyed/spawned state
    merchant inventories
    doors/placeables state

  scheduler
    delayed commands/events (if supported)

  script references
    active scripts or resumable event state if needed
```

For a first implementation, you may choose **not** to persist in-progress VM stacks and instead only persist stable world state.

That is usually a much better tradeoff.

---

## 13. AI Coder Implementation Checklist

## Goal
Implement a KotOR-style NWScript runtime that is practical for gameplay and persistence.

### Compiler/runtime assumptions
- `.nss` source compiles to `.ncs` bytecode
- VM can execute bytecode and invoke registered native functions
- objects are represented by stable engine handles
- native calls mutate/query real world state

### Must-have systems
- bytecode loader
- VM stack and call frames
- object registry
- tag lookup index
- global variable store
- per-object local variable store
- journal subsystem
- item template registry
- runtime item instancing
- action queue
- delayed event scheduler
- save/load serializer

### Minimal native set
- `GetGlobalBoolean`
- `SetGlobalBoolean`
- `GetGlobalNumber`
- `SetGlobalNumber`
- `GetGlobalString`
- `SetGlobalString`
- `GetLocalBoolean`
- `SetLocalBoolean`
- `GetLocalInt`
- `SetLocalInt`
- `GetLocalString`
- `SetLocalString`
- `GetLocalObject`
- `SetLocalObject`
- `GetObjectByTag`
- `GetEnteringObject`
- `GetExitingObject`
- `GetLastUsedBy`
- `GetIsObjectValid`
- `CreateItemOnObject`
- `GetItemPossessedBy`
- `DestroyObject`
- `AddJournalQuestEntry`
- `ExecuteScript`
- `AssignCommand`
- `DelayCommand`
- `SpeakString`
- `SendMessageToPC`
- `Random`

### First acceptance tests
1. A trigger script sets a global and updates the journal.
2. A merchant initializes stock exactly once.
3. Merchant inventory survives save/load.
4. A delayed destruction script removes an object after a timer.
5. A dialogue action script changes a plot flag that another script later reads.
6. Object local variables survive save/load.

---

## 14. Recommended Engineering Strategy

### Strategy A: "useful before perfect"
Do not start by chasing full bytecode fidelity in every obscure corner case.

Instead:
1. get VM execution working for ordinary scripts
2. implement the native host functions that matter most
3. wire save/load persistence
4. validate against real quest and merchant scripts
5. add edge cases later

### Strategy B: "script/runtime split is sacred"
Never let scripts become the persistence layer.
Scripts express rules.
The engine owns durable state.

### Strategy C: "test with real gameplay scenarios"
Best test cases:
- one-time cutscene trigger
- persistent merchant stock
- quest journal update
- area transition state
- save, quit, reload, and verify world continuity

---

## 15. Bottom Line

For KotOR 1, NWScript is the **gameplay logic language**, not the entire game-state persistence system.

A good reimplementation should:
- run KotOR-style event scripts
- expose core native functions for globals, locals, object lookup, inventory, and journal updates
- maintain stable runtime object identity
- serialize all long-play state in the engine save system
- treat merchant persistence as a save/load problem, not just a script problem

If you get those foundations right, you can support a large amount of real KotOR quest and gameplay scripting without needing to perfectly clone every obscure behavior on day one.

---

## 16. Reference Notes for the AI Coder

Use these as grounding points while implementing:

- NWScript is a C-like scripting language used in Neverwinter Nights and later BioWare-engine games including KotOR.
- KotOR tools and modding references center around `.nss`, `.ncs`, and `nwscript.nss`.
- KotOR scripting tools commonly expose browsable NWScript function and constant lists.
- Community tutorials and reverse-engineering references emphasize delayed commands, bytecode execution, and the separation between the scripting language and the engine’s host/runtime systems.
- xoreos reverse-engineering work is useful for understanding engine-side structure and script/runtime separation.


---

## 17. Feature Expansion Notes: Cinematics, the Ebon Hawk, and Minigames

This section translates three higher-level KotOR-style features into concrete implementation requirements for an AI coder:
- cinematic fidelity
- Ebon Hawk interior flow
- Pazaak logic and persistence

These systems often look script-driven from the outside, but in practice they require a careful split between:
- script-exposed native functions
- engine sequencing/state
- UI integration
- save/load persistence

---

## 18. Cinematic Fidelity

KotOR-style cinematics are not just dialogue lines with a camera pointed at characters. They depend on a layer of script-triggerable cinematic controls backed by engine systems.

### Required outcomes
- camera changes feel authored, not random
- cutscene attacks look believable
- scripted reveals and battles can be staged consistently
- the same scene reproduces correctly after load or replay

---

## 18.1 Dynamic Camera (`SetCameraMode`-style behavior)

### Goal
Implement real camera transitions for modes such as:
- close-up
- medium shot
- wide shot
- over-the-shoulder
- isometric / tactical overview
- reveal sweep / establishing shot

### What script should be able to do
Scripts should be able to request cinematic camera states such as:
```c
SetCameraMode(CAMERA_MODE_CLOSEUP, oTarget);
SetCameraMode(CAMERA_MODE_WIDE, oFocus);
SetCameraMode(CAMERA_MODE_ISOMETRIC, OBJECT_SELF);
```

Depending on your exact runtime, this may be a literal native function or a more flexible API such as:
```c
SetCameraPreset(string sPreset, object oPrimary, object oSecondary);
```

### What the engine must do
The engine must translate script requests into actual camera behavior:
- select camera anchor(s)
- compute framing
- interpolate transform
- resolve collisions/occlusion
- preserve continuity across cuts
- restore gameplay camera when the scene ends

### Recommended internal model
Use a camera controller with:
- active mode
- source transform
- target transform
- transition duration
- tracked subject(s)
- optional look-at anchor
- optional rail/spline motion

Suggested structure:
```txt
CameraState
  mode
  position
  rotation
  fov
  target_object
  secondary_target_object
  transition_time
  flags
```

### Practical presets
#### Close-up
Frame one face or upper torso.
Use for dramatic reveals, emotional lines, threat beats.

#### Wide
Frame multiple actors and environment.
Use for room reveals like an Ancient Ruins entrance.

#### Isometric
High-angle tactical shot.
Use for showing spatial context, enemy placement, or ritual spaces.

### Ancient Ruins reveal example
For a ruins reveal:
1. fade or lock player control
2. switch to wide establishing shot
3. sweep toward focal structure
4. cut to close-up reaction shot
5. resume scene logic or dialogue

### Acceptance test
A scripted scene in the ruins:
- starts with player control disabled
- transitions to a wide camera over 1.5 seconds
- tracks the ruin entrance
- cuts to a party close-up
- restores normal camera afterward

---

## 18.2 Choreography Flags for `CutsceneAttack`

### Goal
Make scripted combat sequences look cinematic rather than mechanically simulated.

### Problem
If cutscene attacks use pure gameplay combat resolution, scenes often:
- miss at the wrong moment
- kill targets too early
- look stiff
- break dramatic timing

### Needed solution
Provide a cutscene-specific action API with modifiers controlling presentation rather than purely mechanical combat outcome.

Possible exposed native:
```c
CutsceneAttack(object oAttacker, object oDefender, int nFlags);
```

Possible flags:
```c
CUTSCENE_ATTACK_FORCE_HIT
CUTSCENE_ATTACK_FORCE_MISS
CUTSCENE_ATTACK_NO_DAMAGE
CUTSCENE_ATTACK_PLAY_REACTION
CUTSCENE_ATTACK_KNOCKBACK
CUTSCENE_ATTACK_DELAY_IMPACT
CUTSCENE_ATTACK_FINISHING_BLOW
```

### Recommended behavior model
Split attack choreography into layers:

#### Layer A: animation intent
- attack animation
- facing
- timing window

#### Layer B: impact presentation
- hit spark / VFX / SFX
- defender flinch
- knockback / stumble
- delayed collapse

#### Layer C: gameplay result
- actual HP damage or none
- aggro changes or none
- death trigger or none

This lets you support cinematic flags such as:
- “show a clean hit but do no real damage”
- “play a miss animation for dramatic dodge”
- “force impact after wind-up delay”

### Impact and miss modifiers
For scripted battles to look “real,” add flags or parameters such as:
- forced hit
- forced miss
- suppressed random roll
- presentation-only damage
- flinch severity
- stagger duration
- reaction override

### Example
```c
CutsceneAttack(oSith, oGuard, CUTSCENE_ATTACK_FORCE_HIT | CUTSCENE_ATTACK_NO_DAMAGE | CUTSCENE_ATTACK_PLAY_REACTION);
```

This should:
- play attack animation
- cause the guard to visibly react
- avoid changing core combat state if desired

### Engine requirements
You need:
- cutscene-safe action queueing
- animation event timing hooks
- optional decoupling from combat RNG
- optional decoupling from gameplay damage
- deterministic playback for scripted scenes

### Acceptance test
A scripted duel should include:
- one forced hit
- one forced miss
- one knockback reaction
- no random combat divergence

---

## 19. The Ebon Hawk

The Ebon Hawk is not just a module. It is a persistent narrative hub with:
- expanded party conversations
- destination selection flow
- hub-state progression
- ship-local triggers and unlocks

This means it needs strong support for:
- interior scripting
- party state checks
- travel UI
- persistent ship module state

---

## 19.1 Interior scripting

### Goal
Support on-board conversations and ship-state scripting with the full or expanded party, including companions such as T3-M4 and others encountered after Dantooine.

### What scripts need to do
- check whether a party member is recruited
- check whether they are aboard
- gate conversations by plot progress
- fire one-time ambient conversations
- enable/disable interactables in the ship
- track relationship or companion-specific story flags

### Suggested native/API support
Examples:
```c
int GetIsPartyMemberAvailable(object oCreature);
int GetIsPartyMemberInParty(object oCreature);
int GetGlobalBoolean(string sName);
void StartConversation(object oSpeaker, object oListener, string sDialog);
```

If exact KotOR-native names differ in your runtime, the underlying needs remain the same.

### Common ship scripting patterns
#### Ambient companion scene
- player enters ship
- check plot state
- check whether companion scene already fired
- trigger companion conversation

#### Post-planet debrief
- after completing a major planet objective
- enable new onboard dialogue branches
- unlock ship-based exposition scenes

#### Companion interjection sequence
- queue multiple speakers
- move them to staging markers if needed
- switch cameras between speakers

### Engine requirements
- ship interior module must persist local state
- recruited companion state must persist
- conversations must support speaker switching
- camera and staging support should integrate with dialogue scenes

### Save/load considerations
Persist:
- who is recruited
- who is currently active in party
- which onboard scenes have fired
- local state on ship interactables
- companion-specific plot flags

---

## 19.2 Galaxy Map GUI

### Goal
Implement the UI flow for selecting the next destination:
- Tatooine
- Manaan
- Kashyyyk
- Korriban
- and any other unlocked destinations

### This is not only a script problem
The galaxy map requires a script + UI + travel-state pipeline.

### Script responsibilities
Scripts can:
- unlock destinations
- check whether a destination is allowed
- respond to a selected destination
- fire travel setup logic

Possible script-level function ideas:
```c
int GetDestinationUnlocked(string sPlanetTag);
void SetDestinationUnlocked(string sPlanetTag, int bUnlocked);
void BeginTravelToDestination(string sPlanetTag);
```

### UI responsibilities
The UI must:
- show planet buttons/icons
- indicate locked/unlocked states
- let the player choose
- confirm travel
- pass selected destination back to the game state

### Engine responsibilities
The engine must:
- transition from ship interior to travel sequence
- set next module/planet state
- fire arrival scripts
- update ship and galaxy progression state

### Suggested internal data shape
```txt
GalaxyMapState
  destinations:
    tatooine:
      unlocked: true
      visited: false
    manaan:
      unlocked: true
      visited: false
    kashyyyk:
      unlocked: false
      visited: false
    korriban:
      unlocked: true
      visited: true
  selected_destination: "tatooine"
```

### Acceptance test
From the Ebon Hawk galaxy map:
- only unlocked worlds are selectable
- clicking a world updates the selected destination
- confirming travel loads the correct destination flow
- unlock/visit state persists across save/load

---

## 20. Minigames

KotOR-style minigames should not be treated as throwaway overlays if they have progression consequences. They need real state, UI, and persistence.

---

## 20.1 Pazaak

### Goal
Implement both the **game logic** and the **UI** for Pazaak, including:
- deck building
- side deck selection
- match rules
- persistence of owned cards and configured side deck

### Systems involved
Pazaak requires:
- rules engine
- inventory/collection model for owned cards
- deck builder UI
- in-match UI
- save/load persistence
- optional wager/economy integration

---

## 20.2 Pazaak data model

Suggested structures:

```txt
PazaakCardDefinition
  id
  name
  type
  value
  modifiers
  artwork_ref

PazaakCollectionState
  owned_cards: list<CardId>
  side_deck_config: list<CardId>
  starter_deck_version
```

For a minimal version, persist:
- which cards the player owns
- which cards are in the current side deck
- any unlocked special cards
- whether a tutorial/introduction match was completed

---

## 20.3 Pazaak rules engine

The rules engine should be completely separate from the UI.

### Recommended split
#### Core rules model
Responsible for:
- turn order
- deck draw
- side deck play
- stand/end-turn logic
- bust resolution
- winner determination

#### Presentation layer
Responsible for:
- card animations
- UI prompts
- button states
- highlighting playable side cards
- scoreboard updates

### Core runtime state
```txt
PazaakMatchState
  player_total
  opponent_total
  player_field_cards
  opponent_field_cards
  player_side_hand
  opponent_side_hand
  turn_owner
  player_standing
  opponent_standing
  match_over
  winner
```

### Needed features
- deterministic rule evaluation
- ability to serialize in-progress matches if desired
- side-deck legality checking
- AI opponent behavior

---

## 20.4 Deck building and side-deck persistence

### Goal
The player should be able to:
- own Pazaak cards
- configure a side deck
- keep that configuration between sessions

### Required persistence
Persist:
- card ownership
- selected side deck
- any bans/locks if your rules variant uses them
- optional NPC-specific deck unlock progression

### UI needs
Deck builder should support:
- owned card list
- selected side deck slots
- add/remove interactions
- legality validation
- save/apply behavior

### Acceptance test
1. Player acquires a new card.
2. Player edits side deck.
3. Save the game.
4. Reload.
5. New card ownership and selected side deck remain intact.

---

## 20.5 Pazaak AI

A minimal opponent AI can be rule-based:
- stand if total is strong enough
- prefer side cards that avoid busting
- use negative or positive modifiers situationally

This should be implemented as a pure game-logic decision layer, not embedded in UI code.

---

## 20.6 Script integration for Pazaak

Scripts should be able to:
- start a Pazaak match
- set wager or stakes
- check result
- award cards or credits
- gate progression behind a win if desired

Possible native/API ideas:
```c
int StartPazaakMatch(object oOpponent, int nWager);
int GetLastPazaakResult();
void AwardPazaakCard(string sCardId);
```

### Important implementation note
These are orchestration calls. The actual Pazaak rules and UI should live in dedicated systems, not inside NWScript.

---

## 21. Priority Order for Implementing These Features

Recommended order:

### Phase A: cinematic foundation
- camera controller
- preset camera transitions
- cutscene-safe action queue
- `CutsceneAttack` presentation flags

### Phase B: Ebon Hawk hub support
- persistent ship module state
- companion scene gating
- conversation triggers
- travel selection state

### Phase C: galaxy map flow
- unlock logic
- UI selection
- destination travel transition
- arrival hooks

### Phase D: Pazaak
- rules engine
- card collection persistence
- deck builder UI
- in-match UI
- scripting integration for rewards and stakes

---

## 22. Final Rule for the AI Coder

These features should be built with the following discipline:

- **Cinematics** are not just animation calls; they need camera and choreography systems.
- **The Ebon Hawk** is not just a room; it is a persistent narrative hub with branching ship-state logic.
- **The Galaxy Map** is not just a menu; it is a UI-to-game-state travel pipeline.
- **Pazaak** is not just a minigame overlay; it is a separate persistent subsystem with rules, UI, collection state, and save integration.

If these are implemented as isolated hacks, they will become brittle quickly.  
If they are implemented as first-class engine subsystems with script orchestration on top, they will scale cleanly.
