#!/bin/bash
set -e

echo "Initializing PostgreSQL database..."

SYSTEM_NAME=$(uname)

if [[ $SYSTEM_NAME == "Darwin" ]]; then
  brew services start postgresql
  sleep 2 # Wait for service to start
  PSQL_USER=$(whoami)
elif [[ $SYSTEM_NAME == "Linux" ]]; then
  sudo service postgresql start
  sleep 2 # Wait for service to start
  PSQL_USER="postgres"
fi

# Create user and database
if [[ $SYSTEM_NAME == "Darwin" ]]; then
  echo "Creating admin user and database..."
  psql -c "CREATE USER admin WITH PASSWORD '123456' SUPERUSER;" postgres 2>/dev/null || echo "User admin already exists or failed to create"
  createdb -O admin enhanced_students 2>/dev/null || echo "Database enhanced_students already exists or failed to create"
else
  sudo -u postgres psql -c "CREATE USER admin WITH PASSWORD '123456' SUPERUSER;" || true
  sudo -u postgres createdb -O admin enhanced_students || true
fi

# Give time to create user and database
sleep 1

if [ -f "dump.sql" ]; then
  echo "Restoring database from dump..."
  PGPASSWORD=123456 psql -h localhost -p 5432 -U admin -d enhanced_students -f dump.sql
else
  echo "No dump file found (Create dump.sql in database/ directory), creating empty database."
fi

echo "Database initialization complete!"
echo "Database: enhanced_students"
echo "User: admin"
echo "Password: 123456"
echo "Host: localhost"
echo "Port: 5432"
