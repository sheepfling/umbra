# Style and review

Umbra prefers small, reviewable changes over broad cleanup. The project has no
automatic source formatter yet, so consistent local style and a narrow diff are
the current enforcement mechanism.

## Formatting baseline

[.editorconfig](../../.editorconfig) defines the editor defaults:

- C++ and CMake use two spaces.
- Python and PowerShell use four spaces.
- Text is UTF-8 with LF line endings and a final newline.
- Markdown preserves intentional trailing spaces for hard line breaks.

Follow the surrounding code when a file has a more specific established style.
Do not mix a behavioral change with an unrelated whole-file reformat.

## C++ rules

- Keep public helper headers under cpp/include/umbra/ and private state under
  the appropriate cpp/src/internal/ domain.
- Use full internal include paths so ownership is visible at the call site.
- Keep standard-binding entry points in cpp/src/ thin; move detailed state
  machines to their owning internal domain.
- Add a focused test in cpp/tests/ and a fixture in cpp/tests/data/ only when
  the behavior needs a distinct model input.

## Python and Java rules

- Keep the public Python contract provider-neutral.
- Put provider-specific behavior in its provider package and preserve the
  package's src layout.
- Keep Java TCK sources provider-neutral; JNI configuration belongs in the
  bridge or adapter layer, not the TCK.

## Review checklist

- The change has one clear ownership location.
- The nearest README still describes the folder accurately.
- Tests cover the changed behavior or the reason they cannot is stated.
- Generated source and vendored inputs were not hand-edited.
- Build output, local evidence, secrets, and vendor binaries are absent from
  the diff.
- Requirement mappings were updated when a named source path moved.

## Automated checks

The Windows CI workflow runs the same default preset described in
[CONTRIBUTING.md](../../CONTRIBUTING.md). Local contributors should run that
baseline before review. Additional format or lint automation should be added
only after its configuration matches the established source style.
