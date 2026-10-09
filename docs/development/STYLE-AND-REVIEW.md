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
- Put pointer and reference qualifiers with the declarator (`T *value` and
  `T &value`), because they bind to the variable rather than to the type.
- For read-only strings, prefer `std::string const &value`/`std::wstring const &value`;
  use string views only when the callee borrows text for the call and does not
  require a NUL-terminated buffer. Take by value when the callee owns the
  string. Treat a move as the source's last useful use: finish reads and
  validation first, and document only non-obvious ownership/lifetime handoffs.
- Give semantically distinct private handles and unique IDs symbolic C++ types.
  Prefer strong wrappers when accidental mixing or arithmetic must be blocked;
  a descriptive `using` alias is a useful minimum but does not itself provide
  type safety. Allow arithmetic only where its domain meaning is intentional,
  and express it through named operations when that prevents accidental use.
  Convert to raw integers only at explicit serialization, protocol, or binding
  boundaries. Keep standard-mandated public handle types unchanged and keep
  2010 and 2025 handle domains separate.
- Add a focused test in cpp/tests/ and a fixture in cpp/tests/data/ only when
  the behavior needs a distinct model input.
- Follow [FILE-SIZE-AND-SPLITTING.md](FILE-SIZE-AND-SPLITTING.md) for file-size
  targets, semantic split boundaries, and the legacy-growth guard.

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
only after its configuration matches the established source style. The default
lint route also checks the source-size ratchet: files over 3,000 lines are
review warnings; code over 5,000 lines gets a deliberate search for a real
seam, and the practical code ceiling is 10,000 expanded lines. Do not add
one-off `.inc` fragments to game the physical count. A hand-maintained `.inc`
is acceptable as a reuse boundary only when at least two distinct owning
source/header files include that exact fragment. Repeated includes by one
owner and chains of single-consumer fragments are not reuse. This includes
class-scoped declarations: if only one header includes them, keep them in the
header. Do not introduce single-consumer `.inc` splits; fold existing ones
into their owning source/header when safely in scope. Generated or
tool-mandated textual includes need a documented origin. The checker counts
local `.inc` contents with the owning source/header. If no safe source seam
exists, keep the cohesive unit together or record an explicit size exception
rather than making an artificial split. Documentation may reach 6,000 lines.
Recorded exceptions may only shrink.
