# Coding & Naming Conventions

## C++ Guidelines

### Naming Conventions
- **Member Variables / Fields**: Must use the `m_` prefix with camelCase (e.g., `m_ledPin`, `m_request`, `m_buttonPin`).
- **Classes and Structs**: PascalCase (e.g., `LedController`).
- **Methods and Functions**: camelCase (e.g., `turnOff()`, `isButtonPressed()`, `toggle()`).
- **Local Variables**: camelCase (e.g., `ledPin`, `buttonPin`, `stopRequested`).
- **Constants**: UPPER_SNAKE_CASE or camelCase (e.g., `DEFAULT_PIN`, `ledPin`).
- **Namespaces**: snake_case or lowercase.

### Style & Practices
- Prefer modern C++ idioms.
- Use `#pragma once` for header guards.
