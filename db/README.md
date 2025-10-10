# PostgreSQL C Integration

A practical exploration of C and PostgreSQL integration for system programming education.

## 🚀 Quick Start (Linux/WSL)

### Prerequisites

```sh
sudo apt update

# Install PostgreSQL
sudo apt install postgresql postgresql-client

# Install CMake
sudo apt install cmake

# Verify installation
psql --version
cmake --version
```

### Database setup

```sh
# 1. Make the init script executable
chmod +x database/init_database.sh

# 2. Initialize the database (creates user, database, and restores data)
cd database && ./init_database.sh

# 3. Verify the database is working
psql -h localhost -U admin -d enhanced_students -c "SELECT version();"
```

### Build and run

```sh
# Configure and build
mkdir -p build && cd build
cmake ..
make

# Run the application
./src/db
```
