"""Replace exactly one object in an isolated C++ TCK linker response file.

The caller supplies an existing response file, its old object token, and the
replacement object. No provider library or repository build is chosen here;
the caller still selects the linker working directory when linking.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--old-object", required=True)
    parser.add_argument("--new-object", type=Path, required=True)
    args = parser.parse_args()

    source = args.input.resolve()
    destination = args.output.resolve()
    replacement = args.new_object.resolve()
    if source == destination:
        parser.error("input and output response files must differ")
    if not replacement.is_file():
        parser.error(f"replacement object does not exist: {replacement}")
    content = source.read_text(encoding="utf-8")
    old_token = re.compile(r"(?<!\S)" + re.escape(args.old_object) + r"(?=\s|$)")
    updated, count = old_token.subn(replacement.as_posix(), content)
    if count != 1:
        parser.error(
            "old object must appear as exactly one complete response-file token"
        )
    destination.write_text(updated, encoding="utf-8")
    print(f"replaced {args.old_object} with {replacement} in {destination}")


if __name__ == "__main__":
    main()
