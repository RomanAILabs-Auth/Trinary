# Security Policy

## Supported versions

The latest minor release line receives security fixes.

## Reporting a vulnerability

Email `security@romanailabs.com` with:
- Reproduction steps (ideally a minimal `.tri` or `.py` file).
- Affected version(s).
- Impact assessment.

We respond within 72 hours and target fix-and-disclosure within 90 days.

## Threat model (summary)

- `libtrinary` executes user-supplied code from `.tri` files and Python ASTs
  via a JIT that writes to executable pages. Treat every input as untrusted.
- The compiler and IR validator are the primary fuzzing targets.
- The kernel ABI is stable (`trinary_v1_*`); ABI-breaking fixes require a major
  version bump.
