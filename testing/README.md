<a id="readme-top"></a>

# Testing Frameworks Comparison

A sandbox for exploring C/C++ testing frameworks with examples and automation.

<!-- TABLE OF CONTENTS -->
<details>
  <summary>📖 Table of Contents</summary>
  <ol>
      <li><a href="#quick-start">Quick Start</a></li>
      <li><a href="#feature-comparison">Feature Comparison</a></li>
      <li><a href="#use-case-recommendations">Use Case Recommendations</a></li>
      <li>
         <a href="#development-workflow"> Development Workflow</a>
         <ul>
            <li><a href="#prerequisites">Prerequisites</a></li>
            <li><a href="#automation-commands">Automation commands</a></li>
            <li><a href="#manual-commands-if-mise-not-available">Manual commands (if Mise not available)</a></li>
         </ul>
      </li>
      <li><a href="#why-conan">Why Conan?</a></li>
  </ol>
</details>

<!-- QUICK START -->

## Quick Start

```sh
# Install dependencies and build
mise run dev

# Or step by step
mise run conan-install    # Install dependencies
mise run configure        # Configure CMake
mise run build           # Build project
mise run run-tests       # Run all tests
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- FEATURE COMPARISON -->

## Feature Comparison

| Criterion    | Google Test          | Unity          | CMocka            |
| ------------ | -------------------- | -------------- | ----------------- |
| Language     | C++                  | C              | C                 |
| License      | BSD-3                | MIT            | Apache 2.0        |
| Mock support | ✅ Yes (Google Mock) | ❌ No          | ✅ Yes (built-in) |
| Requires C++ | ✅ Yes               | ❌ No          | ❌ No             |
| Size         | Large (~1MB)         | Small (~100KB) | Medium (~300KB)   |

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USE CASE RECOMMENDATIONS -->

## Use Case Recommendations

| Framework   | Best For                           | Example Project Type                                |
| ----------- | ---------------------------------- | --------------------------------------------------- |
| Google Test | C++ projects, enterprise testing   | Large C++ applications, cross-platform libraries    |
| Unity       | Embedded systems, pure C projects  | Microcontrollers, resource-constrained environments |
| CMocka      | Pure C projects with mocking needs | System libraries, OS development, C applications    |

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- DEVELOPMENT WORKFLOW -->

## Development Workflow

### Prerequisites

- Conan 2.x

- CMake 3.10+

- Mise (for automation) or manual command execution

### Automation commands

```bash
# Development workflow (clean → install → build → test)
mise run dev

# Individual steps
mise run conan-install    # Install Conan dependencies
mise run configure        # Configure with CMake
mise run build           # Build the project
mise run run-tests       # Run all test suites

# Framework-specific testing
mise run test-cmocka     # Run only CMocka tests
mise run test-unity      # Run only Unity tests
mise run test-gtest      # Run only Google Test tests

# Maintenance
mise run clean          # Clean build directory
mise run info           # Show project information
```

### Manual commands (if Mise not available)

```bash
# Conan dependency management
conan install . --output-folder=build --build=missing

# CMake configuration
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=./build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

# Building and testing
make
./tests/tests  # Run tests
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- WHY CONAN -->

## Why conan?

- Cross-platform focus (unlike vcpkg's Windows bias)

- CMake and Bazel integration

- Active community and ConanCenter repository

- Professional dependency management features

<p align="right">(<a href="#readme-top">back to top</a>)</p>
