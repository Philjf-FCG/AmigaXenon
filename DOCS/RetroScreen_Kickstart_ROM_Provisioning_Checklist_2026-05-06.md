# Kickstart ROM Provisioning Checklist

Date: 2026-05-06
Issue: AmigaXenon-5qx.1.3

## Scope
Ensure a user-supplied, legally obtained Kickstart ROM is available at runtime and documented for local validation without distributing copyrighted firmware.

## Required Inputs
- User-provided Kickstart ROM file (not committed to repository)
- Local absolute path to ROM on developer machine
- Provenance note (owner/source) maintained by user

## Recommended Local Layout
- EmulatorData/Kickstart/
- Example local filename: EmulatorData/Kickstart/kick13.rom

## Configuration Hook Points
- Standalone harness:
  - --kickstart <path>
  - --option key=value
- UE manager config (Saved/RetroScreen.ini):
  - LibretroCorePath
  - LibretroRomPath
  - Additional core options via environment-variable callback path as needed

## Validation Steps
1. Confirm ROM file exists at intended local path.
2. Confirm ROM file is excluded from source control and not embedded in distributables.
3. Launch standalone harness with kickstart option and verify core initialization succeeds.
4. Launch UE manager path with equivalent configuration and verify initialization succeeds.
5. Record success/failure and path alias in local QA notes.

## Compliance Notes
- No Kickstart ROM binary is to be committed to this repository.
- Project documentation should continue to instruct users to provide their own legal ROM.
- Any sample paths in docs must be placeholders only.
