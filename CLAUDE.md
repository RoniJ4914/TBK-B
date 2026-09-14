# The Black Knight: Beginnings

UE 5.8.2, C++ with Blueprint reserved for content-facing systems only.
Dark fantasy/sci-fi action game: a fallen god of shadows and death, stripped
of power and amnesiac, exiled to the mortal world. AC-style structure (hub +
main quest spine + side content), not a physics-sim space game. This doc
exists so a fresh session can continue the work without re-deriving decisions
already made and verified.

## Working agreement
- Small, verifiable increments. After each milestone: compile, list what
  changed, state what is NOT yet implemented, stop for review before the next
  milestone.
- Don't silently reduce scope. If a milestone is too large for one pass, say
  so and propose a split.
- Confirm before hard-to-reverse/outward-facing actions (force-closing the
  editor, discarding uncommitted work, etc.) — see the Live Coding gotcha
  below, which has already bitten this project once.
- Prefer engine-native systems over custom reinventions (Enhanced Input,
  World Partition, GAS if combat complexity grows — flag it if warranted).
- Ask before adding third-party plugins.
- Target: PC only, 60fps @ 1080p on a mid-range reference GPU (~RTX 3060).
  Note: `Config/DefaultEngine.ini` currently has RayTracing + Substrate +
  Lumen at "Maximum" scalability, which fights that target — unaddressed,
  flagged for a later pass.

## Environment
- Engine: UE 5.8.2, installed at `C:\Program Files\Epic Games\UE_5.8`.
- Project: `C:\Users\JacksPC\Documents\Unreal Projects\MyProject\BlackKnight.uproject`.
- Shell: PowerShell for anything invoking Epic's batch files/exes (Bash's
  `'C:\Program Files\...'` invocation fails — quoting issue with the launcher,
  use `PowerShell` tool instead). Bash is fine for file/git work.
- Compile (editor target):
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" BlackKnightEditor Win64 Development -project="C:\Users\JacksPC\Documents\Unreal Projects\MyProject\BlackKnight.uproject" -waitmutex
  ```
  Run it backgrounded (`run_in_background: true`) — a full rebuild (incl. the
  ALS plugin) takes ~9-10 min; incremental is seconds.
- **Gotcha**: if the editor (`UnrealEditor.exe`) is open, Live Coding holds a
  lock and the build fails with "Unable to build while Live Coding is active."
  Check `tasklist | grep -i unreal` first. Don't force-close it without
  asking — it may hold unsaved editor state; ask the user to close it, or get
  explicit confirmation to `taskkill` it.
- Headless smoke test (verifies LoadMap/GameMode/pawn init without opening a
  window — use NullRHI, launch detached, sleep, then taskkill; a `-game` run
  never exits on its own):
  ```powershell
  Start-Process -FilePath "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -ArgumentList @('"<uproject path>"','-game','-NullRHI','-nosplash','-unattended') -PassThru -RedirectStandardOutput <out> -RedirectStandardError <err>
  ```
  Then read `Saved/Logs/BlackKnight.log` for `LogLoad: LoadMap`, `Game class
  is '...'`, and grep for `Ensure condition failed` (ALS's `ALS_ENSURE` macros
  fire here if a settings/mesh/anim asset failed to resolve — this is how the
  ALS asset wiring in `BKPlayerCharacter`/`BKCameraComponent` was validated
  without a human opening the editor).
- **Git LFS is NOT installed on this machine** (`git lfs` → command not
  found). `.gitattributes` already declares LFS filters for binary asset
  types, but until `git lfs install` is run, `Content/` and `Plugins/` are
  **intentionally left untracked** — do not `git add` them or binary assets
  land as plain blobs in history. Small text files under `Content/` don't
  exist yet either way. Flag this to the user again if content work is about
  to start in earnest.
- `.claude/` and `.mcp.json` are gitignored (local tooling, not project
  files).

## Architecture decisions already made (don't re-litigate)
- **Module renamed** `MyProject` → `BlackKnight` (Build.cs, Target.cs,
  `BLACKKNIGHT_API`, log category, `.uproject`, engine redirects in
  `DefaultEngine.ini`). Old `MyProject*.cpp/.h` template files (character,
  game mode, controller, rifle, variants) still exist under
  `Source/BlackKnight/` unrenamed internally — they're template leftovers,
  not part of the active game, and haven't been touched or removed yet.
- **Player character built on ALS** (`Plugins/ALS`, Advanced Locomotion
  System Refactored — already installed, `EnabledByDefault` in its
  `.uplugin`, now also explicitly listed in `BlackKnight.uproject`).
  `ABKPlayerCharacter : AAlsCharacter`, `UBKCameraComponent :
  UAlsCameraComponent`. Rationale from the original scoping conversation:
  ALS's traversal quality (gait/stance/lean/foot IK/mantling) outweighs the
  cost of threading Milestone 2 combat through its state machine, versus
  building movement from a stock `ACharacter`.
  - All ALS asset defaults (mesh, anim BP, character/movement settings,
    camera mesh/anim/settings, input mapping context + actions) are wired in
    **C++ constructors via `ConstructorHelpers`**, not a Blueprint subclass —
    both classes are playable with zero Blueprint work. Asset paths are
    collected in the `BKPlayerCharacterAssets` namespace at the top of
    `BKPlayerCharacter.cpp` — check there first if an asset needs swapping
    for the art pass.
  - `UEditorLoadingAndSavingUtils::NewBlankMap`/`SaveMap` vs
    `FEditorFileUtils` — easy to get wrong, they're different classes in
    `FileHelpers.h`; only the former pair matches by signature.
- **Game framework classes**: `ABKGameMode` (sets default
  pawn/controller/state classes in C++), `ABKGameState`, `ABKPlayerState`,
  `ABKPlayerController` — the latter three are intentionally thin until
  Milestones 3/5 need them (currency/equipment on PlayerState, quest log/
  vendor UI on PlayerController).
- **Test level** `/Content/Levels/L_TestLevel`, generated (not hand-built) by
  an editor-only commandlet: `Source/BlackKnight/Editor/
  BKCreateTestLevelCommandlet.cpp` (`WITH_EDITOR`-guarded; `UnrealEd` linked
  only when `Target.bBuildEditor`, so it never touches the shipping Game
  target). Run it with:
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<uproject>" -run=BKCreateTestLevel -unattended -nopause -nosplash
  ```
  It instantiates `/Engine/Maps/Templates/Template_Default` (floor,
  DirectionalLight, SkyLight, SkyAtmosphere, ExponentialHeightFog,
  VolumetricCloud, PlayerStart already included) rather than a truly blank
  map — **a prior version used `NewBlankMap()`, which produced a lightless,
  floorless void and looked like a black-screen bug to the user**. Don't
  regress to blank-map generation. It also explicitly clears any per-level
  `WorldSettings->DefaultGameMode` override so the level inherits
  `GlobalDefaultGameMode` from `DefaultEngine.ini` rather than hardcoding one.
  - Note: the project's original template levels (`Lvl_ThirdPerson`, ALS's
    own `L_Als_Grid`) hard-override their game mode to ALS's
    `B_Als_GameMode` at the World Settings level — that's why they weren't
    reused for `L_TestLevel`.
- `DefaultEngine.ini`: `GameDefaultMap`/`EditorStartupMap` →
  `/Game/Levels/L_TestLevel`, `GlobalDefaultGameMode` → `BKGameMode`.

## Status
- **Milestone 1 (project foundation): done**, committed
  (`d2b4c3b`, black-screen fix `742dab3`). Compiles clean (0 errors, 0
  warnings). Verified headlessly (LoadMap succeeds, `Game class is
  'BKGameMode'`, no ALS ensures fire — mesh/anim/settings/camera all
  resolved). **Not yet confirmed by the user in an actual Play-in-Editor
  session** — last known state was mid-fix (black screen root-caused and
  patched, user hadn't retried when this doc was written). Confirm that
  before treating M1 as fully closed.
- Milestones 2-6 (see original brief, summarized below): not started.

## Next: Milestone 2 — Combat core ("simple but effective")

Scope from the original brief:
- Lock-on melee combat: light/heavy attack, dodge/roll with i-frames,
  block **or** parry (pick one — present tradeoffs to the user before
  building, don't decide unilaterally), stamina resource.
- One enemy AI base class (Behavior Tree + Blackboard) with patrol/chase/
  attack states, usable as a parent for future enemy types.
- Acceptance: player can kill a test dummy enemy in `L_TestLevel`; damage,
  health, and death are functional end-to-end.

Suggested build order:
1. Stamina resource component on the player character (regen/drain rules).
2. Dodge/roll with i-frames — replace ALS's placeholder roll
   (`ABKPlayerCharacter::Input_OnRoll` → `StartRollingGrounded`) with a
   combat-aware version: i-frame window, stamina cost.
3. **Decision point**: block vs. parry — ask the user, present tradeoffs,
   before implementing either.
4. Lock-on targeting: target selection + camera behavior layered onto
   `UBKCameraComponent`.
5. Light/heavy attack combo: animation-driven hit windows, damage dealing.
6. Damage/health pipeline: shared interface so player and future enemies use
   the same system (check if ALS or the template's `Variant_Combat` folder —
   `CombatDamageable`, `CombatAttacker` interfaces already exist there from
   the original template — has something reusable before building fresh;
   these are unrenamed template leftovers, evaluate before reusing).
7. Enemy AI base class: Behavior Tree + Blackboard, patrol → chase → attack.
8. Test dummy enemy: minimal concrete subclass proving the pipeline
   end-to-end.
9. Compile clean, verify headlessly where possible, report back before M3.

## Later milestones (context only, not started)
- **M3 Progression/economy**: single currency, data-table inventory, one
  weapon equipment slot affecting combat stats, SaveGame persistence.
- **M4 World structure**: one hub level with World Partition streaming, 3-5
  interior spaces, travel point to a second smaller level (proves the
  inter-level travel pattern before scaling to more hubs). Recommend and
  justify streaming vs. loading-screen transition for 5.8.2.
- **M5 Quest framework**: data-driven quests, objective tracking, quest log
  UI; one full main-quest chain (3-4 objectives) + one side quest.
- **M6 Boss encounter**: multi-phase fight extending the M2 enemy base
  class, tied to the main quest chain's end.

**Explicitly out of scope** until further notice: additional
planets/hub levels beyond the two in M4, full city density ("every building
enterable"), space flight/travel simulation, marketplace/high-fidelity art
(placeholder assets only until systems are validated).
