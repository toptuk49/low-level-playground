<a id="readme-top"></a>

<!-- PROJECT LOGO -->
<br />
<div align="center">
  <h3 align="center">Low-Level Playground</h3>

  <p align="center">
    💻 A playground for exploring low-level programming concepts: command-line tools, concurrency, networking, encoding, assembly and database systems.<br />
    <br />
    <a href="#about-the-project"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="#usage">Usage</a>
    ·
    <a href="https://github.com/toptuk49/low-level-playground/issues">Report Bug</a>
    ·
    <a href="https://github.com/toptuk49/low-level-playground/issues">Request Feature</a>
  </p>
</div>

<!-- TABLE OF CONTENTS -->
<details>
  <summary>📖 Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">🏛 About The Project</a></li>
    <li><a href="#built-with">🛠 Built With</a></li>
    <li>
      <a href="#getting-started">🚀 Getting Started</a>
      <ul>
        <li><a href="#prerequisites">📋 Prerequisites</a></li>
        <li><a href="#installation">💾 Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">💡 Usage</a></li>
    <li><a href="#roadmap">🛣 Roadmap</a></li>
    <li><a href="#contributing">🤝 Contributing</a></li>
    <li><a href="#license">📄 License</a></li>
    <li><a href="#contact">✉️ Contact</a></li>
  </ol>
</details>

<!-- ABOUT THE PROJECT -->

## 🏛 About The Project

This repository collects my experiments with **low-level programming**.  
It started as a set of university labs, but I decided to transform it into a single "playground" that grows together with my skills.

Main topics covered:

- 🖥 Command-line tools & argument parsing
- 📂 File operations, hex representation, compression & encoding
- 🧵 Processes, threads, synchronization & deadlocks
- 🌐 Client-server communication (TCP, UDP, simple HTTP-like)
- ⚙️ Computer architecture & Assembly (mixing with C)
- 🗃️ Database systems - PostgreSQL integration, C API

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### 🛠 Built With

This project relies on **C**, **Assembly**, and standard Linux toolchain.

- **GCC / Clang** - C compilation
- **GNU Make** - Build automation
- **POSIX APIs** - fork, exec, pthreads, sockets
- **NASM / GAS** - Assembly programming
- **PostgreSQL + libpq** - Database connectivity
- **CMake** - Advanced build configuration

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->

## 🚀 Getting Started

Follow these steps to run any experiment locally.

### 📋 Prerequisites

You need a Linux environment (native, WSL, or VM) with:

- `gcc` / `clang`
- `make`
- `git`

### 💾 Installation

1. Clone the repo:

```sh
git clone git@github.com:toptuk49/low-level-playground.git
cd low-level-playground
```

2. Navigate to a specific module, e.g.:

```sh
cd concurrency_and_networking/lab4/src
```

3. Build:

```sh
# If module contains Makefiles:
make

# If module contains CMakeLists.txt:
mkdir -p build && cd build
cmake ..
make
```

4. Run:

```sh
./build/path/to/executable
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->

## 💡 Usage

Each directory contains an independent experiment with its own Makefile.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- ROADMAP -->

## 🛣 Roadmap

- [x] Import files from other repositories

- [ ] Add CI for automatic build testing

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTRIBUTING -->

## 🤝 Contributing

If you have suggestions, feel free to fork and submit a PR, or open an issue with the enhancement tag.

1. Fork the Project

2. Create your Feature Branch (git checkout -b feature/\<feature-name\>)

3. Commit your Changes (git commit -m 'feat: add \<feature-name\>')

4. Push to the Branch (git push origin feature/\<feature-name\>)

5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->

## 📄 License

Distributed under the GNU General Public License. See LICENSE for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->

## ✉️ Contact

Anton Kuznetsov - <a href="https://github.com/toptuk49">GitHub</a>

Project Link: https://github.com/toptuk49/low-level-playground

<p align="right">(<a href="#readme-top">back to top</a>)</p>
