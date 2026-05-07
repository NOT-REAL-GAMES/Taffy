# Taffy Runtime Container Roadmap

This document turns Taffy from "an asset format with some powerful extras" into a
clear runtime-container design for Tremor.

The goal is not to compete with USD as a general-purpose authoring universe. The
goal is to make Taffy excellent at packaging, streaming, loading, and executing
engine-native runtime content.

## Design Principles

These are the rules that should shape every change:

- Every chunk type has a crisp runtime purpose.
- Streaming is first-class, not bolted on.
- Code chunks are sandboxed and versioned.
- Authoring/import stays separate from runtime packaging.
- The file can be partially loaded and partially understood.
- Tools can inspect and rewrite chunks without needing the whole engine alive.

## What Taffy Already Has

Taffy already has the beginnings of the right shape:

- A stable top-level `AssetHeader` and `ChunkDirectoryEntry` model in
  [C:/Projects/Taffy/include/taffy.h](C:/Projects/Taffy/include/taffy.h).
- Strong runtime-facing chunk ideas: `GEOM`, `MTRL`, `SHDR`, `AUDI`, `SCPT`,
  `PHYS`, `OVRL`, `DEPS`.
- A real streaming path in
  [C:/Projects/Taffy/include/taffy_streaming.h](C:/Projects/Taffy/include/taffy_streaming.h),
  even if it is currently audio-oriented.
- Engine-native features that justify a custom runtime package:
  quantized geometry, embedded shaders, overlays, and chunk-level composition.

That means this is not a blank-sheet design. It is a hardening and focusing job.

## Current Gaps

The current implementation also shows where the format needs structure:

- `AssetHeader` has `dependency_count` and `ai_model_count`, but there is no
  concrete dependency directory or model directory contract yet.
- `ChunkDirectoryEntry` is enough for basic loading, but not enough for rich
  streaming and patching. It lacks schema versioning, compression metadata,
  stable chunk identities, and class-of-service hints.
- `Asset::get_chunk_data()` returns a copied `std::vector<uint8_t>`, which is
  convenient, but not the right default for large runtime packages.
- `StreamingTaffyLoader` is conceptually generic but operationally specialized
  toward audio chunk streaming.
- Code-bearing content is not yet described as a capability-bounded runtime
  contract. It is still closer to "some script data exists" than "this package
  executes safely under a known ABI."
- There is no explicit bootstrap story for "mount this file and start the game."

## Product Definition

Taffy should become three things at once:

1. A runtime asset format for individual engine-native assets.
2. A package/container format for scenes, levels, and game bundles.
3. A mountable runtime cartridge for self-contained Tremor content.

That suggests three package profiles instead of one vague mega-format:

- `Asset`: one logical asset plus optional overlays and behavior.
- `Scene`: a composition of assets, startup state, and world metadata.
- `Game`: a bootable package with scene graph, code, audio, UI, and rules.

All three can still use the same outer `.taf` container.

## Separation of Concerns

The clean line to preserve is:

- Authoring formats: USD, glTF, OBJ, WAV, script source, editor-native data.
- Build graph: import, convert, validate, optimize, package.
- Runtime TAF: cooked, indexed, streamable, executable, engine-facing.

Taffy should not become the primary source-of-truth authoring format for every
tool. It should become the canonical runtime packaging target.

That gives us the right pipeline:

`authoring sources -> import graph -> Taffy packager -> runtime .taf`

## Format Direction

The existing `Header + Directory + Chunks` shape is good. Keep it.

The next version should strengthen the directory and make bootstrap metadata
explicit.

### Recommended New Core Chunks

- `MANF`: Package manifest.
  - Package profile: `asset`, `scene`, or `game`
  - Human-readable identity
  - Build provenance
  - Required engine/runtime ABI versions
- `BOOT`: Bootstrap entrypoints.
  - Startup scene
  - Main code chunk
  - Default UI root
  - Initial audio bank or stream group
- `SCEN`: Scene graph / world composition root.
  - Entity templates
  - transforms
  - references to `GEOM`, `MTRL`, `PHYS`, `AUDI`, `SCPT`
- `STRM`: Streaming metadata.
  - Chunk priorities
  - hot/warm/cold residency class
  - prefetch windows
  - optional spatial region mapping
- `CODE`: Optional future split from `SCPT` for executable runtime payloads.
  - VM bytecode modules
  - ABI version
  - capability mask
  - exported entrypoints
- `CATL`: Asset catalog.
  - Stable asset IDs
  - named references
  - aliases
  - mount-local paths

### Modding-Friendly External References

Taffy packages should support both:

- embedded runtime chunks, and
- explicit loose-file references for moddable or replaceable content.

The `DEPS` chunk is the right place for that. A package should be able to say:

- `"use the embedded startup UI"`
- `"load this scene from a sibling loose .taf file"`
- `"treat this external directory as an override/mod source"`

That gives Tremor a clean hybrid model: self-contained when useful, porous when
modding or iteration speed matters more.

Some of these may map onto existing chunks, but the important part is the role,
not the exact fourcc.

### Recommended Chunk Directory v2 Fields

Keep the current directory entry, but evolve it so tools and loaders can reason
about content without loading it:

- `chunk_id`: stable 64-bit identity distinct from name
- `schema_version`
- `codec`: none, zstd, lz4, etc.
- `compressed_size`
- `uncompressed_size`
- `stream_class`: hot, warm, cold, on-demand
- `role_flags`: boot-critical, editor-only, runtime-optional, overlayable
- `dependency_range` or reference into a side table

This can be done either by:

- introducing a versioned directory entry layout in a file format bump, or
- keeping the current directory entry and adding a mandatory manifest side table
  chunk for richer metadata.

I prefer the second path first because it keeps older assets readable.

## Streaming as a First-Class System

Right now, streaming exists mostly as "audio chunk loading." The next step is to
turn that into generic package streaming.

### Runtime Rules

- The loader must be able to read only:
  - header
  - directory
  - manifest
  - bootstrap chunk
- Any non-bootstrap chunk must be loadable independently by offset.
- Chunks should be physically laid out so boot-critical content is early in the
  file and cold content is later.
- A package should remain useful even if only some chunk classes are loaded.

### Loader API Direction

Evolve `StreamingTaffyLoader` toward a general `MountedTaffyPackage` concept:

- `openPackage(path)`
- `readHeader()`
- `getDirectory()`
- `getChunkInfo(chunk_id or name)`
- `mapChunk(...)` or `readChunk(...)`
- `prefetch(set of chunk ids)`
- `evict(set of chunk ids)`
- `resolveBootstrap()`

Audio streaming can stay on top of that as one client of the generic package API.

### Residency Classes

Every runtime chunk should declare one of:

- `Boot`: must be available at mount/startup.
- `Resident`: should stay loaded while package is active.
- `Streamed`: loaded on demand and evictable.
- `Debug`: optional in shipping builds.

This is the difference between a pleasant container and a giant binary blob.

## Sandboxed Code Chunks

If Taffy is going to carry executable behavior, code cannot just be "some bytes
that the engine runs."

### Code Chunk Contract

Every code-bearing chunk should declare:

- `vm_kind`: q3vm, wasm, tremor-vm, etc.
- `abi_version`
- `entrypoints`
- `imported_capabilities`
- `resource_bindings`
- `determinism_flags`
- `save_state_policy`

### Capability Model

A code chunk should not get ambient authority. It should explicitly request
capabilities like:

- read package-local asset data
- spawn entities
- play audio
- emit UI events
- access save data
- perform networked actions
- call AI services

The host decides whether those capabilities are granted.

### Versioning

Each code chunk should be valid against a declared VM ABI version. That lets the
packager reject stale modules early instead of producing mysterious runtime behavior.

## Partial Understanding

One of the best targets for Taffy is: tools should be able to answer useful
questions without needing the whole runtime.

Examples:

- What is this package?
- Is it a scene or a game?
- What chunks does it contain?
- Which chunk is the startup world?
- Which code runtime does it require?
- Which external dependencies does it reference?
- Which chunks are streamable?
- Which overlays can target it?

That means the manifest and directory need to be enough for package introspection.

## Tooling Requirements

The package should be manipulable by offline tools without spinning up Tremor.

Required tooling operations:

- inspect package metadata
- list chunks
- extract a chunk
- replace a chunk
- append a chunk
- rewrite the directory
- validate checksums and schema versions
- diff two packages
- build overlays from chunk-level edits

This is where Taffy stops being mystical and becomes healthy.

## Recommended Packaging Pipeline

### Stage 1: Import

Import authoring sources into typed intermediate data:

- geometry
- materials
- textures
- scripts
- audio streams
- scene descriptions

### Stage 2: Cook

Transform imported data into engine-native runtime payloads:

- quantize geometry
- generate meshlets
- compile shaders
- encode audio chunks
- compile VM code
- bake scene references

### Stage 3: Package

Assemble runtime chunks into `.taf`:

- write manifest and bootstrap
- assign stable chunk IDs
- compute directory and checksums
- order chunks by streaming policy
- optionally compress cold data

### Stage 4: Validate

Run structural validation:

- required chunks present
- schema versions known
- offsets valid
- dependencies resolvable
- code capabilities declared
- boot targets valid

## Phased Engineering Plan

### Phase 1: Harden the Container

Goal: Make Taffy a clean runtime package even before "whole games in one file."

- Add a manifest chunk and bootstrap chunk.
- Add generic package inspection tooling.
- Add a zero-copy read API next to the current copying API.
- Generalize `StreamingTaffyLoader` from audio-first to package-first.
- Make dependency records real instead of implied by header counts.

Deliverable:

- A `.taf` that can describe itself and be partially inspected without full load.

### Phase 2: Define Runtime Package Profiles

Goal: Distinguish `asset`, `scene`, and `game` packages.

- Add package profile to the manifest.
- Add startup scene and entrypoint metadata.
- Add catalog/reference chunks for internal asset lookup.
- Teach Tremor to mount a package and resolve bootstrap assets from it.

Deliverable:

- A `.taf` scene package that Tremor can mount directly.

### Phase 3: First-Class Streaming

Goal: Make runtime chunk streaming generic.

- Add stream class metadata.
- Add prefetch/eviction APIs.
- Physically order chunks by load phase.
- Extend streaming beyond audio to geometry, textures, and scene regions.

Deliverable:

- A `.taf` package where cold content is streamed rather than eagerly loaded.

### Phase 4: Sandboxed Runtime Code

Goal: Make executable content safe and versioned.

- Formalize VM ABI metadata in script/code chunks.
- Add declared capability imports.
- Validate ABI compatibility at package mount time.
- Separate code from serialized save-state or live runtime state.

Deliverable:

- A `.taf` package with bootable scripted behavior under a known capability model.

### Phase 5: Whole-Game Cartridge

Goal: Allow a self-contained Tremor game package.

- Package startup scene, UI, audio, code, materials, geometry, and AI config.
- Support mount -> bootstrap -> run from one file.
- Keep overlays and patching chunk-level, not blob-level.

Deliverable:

- A single-file Tremor cartridge package.

## Recommended Immediate Changes

If we want to start engineering this now, these are the highest-leverage first moves:

1. Add a `MANF` chunk and define a small manifest schema.
2. Add a `BOOT` chunk for startup entrypoints.
3. Refactor `StreamingTaffyLoader` into a generic package loader API.
4. Add non-copying chunk access for runtime paths.
5. Define a versioned script/code chunk ABI contract.
6. Build a small CLI inspector that prints header, manifest, directory, and chunk summary.

Those six items would do more to make Taffy "real" than adding ten futuristic chunk types.

## A Practical North Star

The right mental model for Taffy is not:

> "a custom format that someday does everything"

It is:

> "Tremor's mountable runtime package, with explicit streaming, explicit execution,
> and explicit composition."

If we stay disciplined about that, then "a whole game in one `.taf`" stops sounding
grandiose and starts sounding like the natural end state of the format.
