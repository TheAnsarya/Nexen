# Nexen Documentation

This directory contains user-facing documentation for Nexen.

## 📖 User Documentation

| Document | Description |
| ---------- | ------------- |
| [TAS Editor Manual](TAS-Editor-Manual.md) | Complete user guide for TAS movie editing |
| [Release Guide](RELEASE.md) | How to build and publish releases |

## 🎮 TAS System

| Document | Description |
| ---------- | ------------- |
| [TAS Developer Guide](TAS-Developer-Guide.md) | Technical documentation for TAS internals |
| [TAS Architecture](TAS_ARCHITECTURE.md) | High-level architecture overview |
| [Movie Format Spec](NEXEN_MOVIE_FORMAT.md) | NMV file format specification |

## 🔧 API Documentation

API documentation is generated using Doxygen from source code comments.

### Generating Documentation

```bash
# From repository root
doxygen Doxyfile

# Output: docs/api/html/index.html
```

### Prerequisites

- **Doxygen** 1.9.0+
- **Graphviz** (optional, for diagrams)

### Installation

**Windows:**

- [Doxygen Downloads](https://www.doxygen.nl/download.html)
- [Graphviz Downloads](https://graphviz.org/download/)

**macOS:**

```bash
brew install doxygen graphviz
```

**Linux:**

```bash
sudo apt install doxygen graphviz
```

## 📁 Directory Structure

```text
docs/
├── README.md                  # This file
├── RELEASE.md                 # Release guide
├── TAS-Editor-Manual.md       # TAS user guide
├── TAS-Developer-Guide.md     # TAS developer docs
├── TAS_ARCHITECTURE.md        # TAS architecture
├── NEXEN_MOVIE_FORMAT.md      # Movie format spec
└── api/                       # Generated API docs
    └── html/                  # HTML output
```

## 📚 Developer Documentation

For development and contribution documentation, see:

| Location | Description |
| ---------- | ------------- |
| [~docs/](../~docs/README.md) | Developer documentation index |
| [~docs/ARCHITECTURE-OVERVIEW.md](../~docs/ARCHITECTURE-OVERVIEW.md) | System architecture |
| [~docs/CPP-DEVELOPMENT-GUIDE.md](../~docs/CPP-DEVELOPMENT-GUIDE.md) | C++23 coding guide |
| [~docs/pansy-export-index.md](../~docs/pansy-export-index.md) | Pansy export docs |

## 🔗 Related Files

| File | Description |
| ------ | ------------- |
| [COMPILING.md](../COMPILING.md) | Build instructions |
| [FORMATTING.md](../FORMATTING.md) | Code style guide |
| [Doxyfile](../Doxyfile) | Doxygen configuration |

## 📝 Documentation Style

### Code Comments

Nexen uses XML-style documentation comments:

```cpp
/// <summary>
/// Brief description of the function.
/// </summary>
/// <param name="param">Parameter description.</param>
/// <returns>Return value description.</returns>
void ExampleFunction(int param);
```

See [~docs/CODE-DOCUMENTATION-STYLE.md](../~docs/CODE-DOCUMENTATION-STYLE.md) for full guidelines.
