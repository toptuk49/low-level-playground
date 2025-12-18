# PostgreSQL C Integration

A practical exploration of C and PostgreSQL integration for system programming education.

## 🚀 Quick Start

### Automated Setup (Recommended)

```sh
# 1. Install Mise (if not already installed)
curl https://mise.run | sh    # Linux/macOS
# OR via package manager
brew install mise             # Homebrew
winget install jdx.mise       # Windows

# 2. Run full development workflow
mise run dev

# 3. Run the application
cd build
./src/main
./src/sql_injection
```

This will:

- Clean any previous builds

- Install Conan dependencies

- Configure CMake

- Build the project

- Initialize the database

### Manual Setip

#### 1. Prerequisites

**macOS:**

```sh
brew install postgresql@17 cmake conan
brew link --force postgresql@17
```

**Linux/WSL/Ubuntu:**

```sh
sudo apt update
sudo apt install postgresql postgresql-client cmake
pip install conan
```

**Verify installations:**

```sh
psql --version # Should show PostgreSQL 17.x
cmake --version # Should be >= 3.15
conan --version # Should be 2.x
```

#### 2. Initialize Database

```sh
# Make script executable
chmod +x database/init_database.sh

# Run initialization
cd database
./init_database.sh

# Verify database is accessible
psql -h localhost -U admin -d enhanced_students -c "SELECT version();"
```

#### 3. Build and run

```sh
# Install dependencies
conan install . --output-folder=build --build=missing

# Configure and build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
make

# Run the application
./src/main
./src/sql_injection
```

## 🔧 Development Commands

| **Command**          | **Description**                                           |
| -------------------- | --------------------------------------------------------- |
| `mise run init`      | Initialize\reset database                                 |
| `mise run install`   | Install Conan dependencies                                |
| `mise run configure` | Configure CMake project                                   |
| `mise run build`     | Build the project                                         |
| `mise run clean`     | Clean build directory                                     |
| `mise run dev`       | Clean, install, configure, build, and initialize database |
| `mise run info`      | Print project information                                 |

## 📊 Database Schema

The project uses a sample `enhanced_students` database with the following structure:

```sql
-- Example tables (from dump.sql if provided)
CREATE TABLE students (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    email VARCHAR(100),
    age INTEGER
);
```

**Default Credentials:**

- **Database**: `enhanced_students`

- **User**: `admin`

- **Password**: `123456`

- **Host**: `localhost`

- **Port:** `5432`

## 🛠️ Troubleshooting

### Common Issues

#### 1. "CMake.app needs to be moved to the bin" on macOS

- Go to **System Settings $\arrowright$ Privacy & Security**

- Scroll down and click "Open Anyway" for CMake.app

- Alternatively: `sudo xattr -d com.apple.quarantine /Applications/CMake.app`

#### 2. PostgreSQL service fails to start

```sh
# macOS
brew services restart postgresql@17

# Linux
sudo service postgresql restart
```

#### 3. Conan GnuPG key issues

```sh
# Import the missing key
curl -sSL https://keyserver.ubuntu.com/pks/lookup?op=get&search=0xB2508A90102F8AE3B12A0090DEACCAAEDB78137A | gpg --import
gpg --list-keys
```

#### 4. "Could not find PostgreSQL" error

```sh
# Ensure Conan dependencies are installed
rm -rf build
mkdir build && cd build
conan install .. --build=missing
```
