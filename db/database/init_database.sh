#!/bin/bash
set -e

echo "Initializing PostgreSQL database..."

sudo service postgresql start
sleep 2 # Wait for service to start
sudo -u postgres psql -c "CREATE USER ADMIN WITH PASSWORD '123456' SUPERUSER;" || true
sudo -u postgres createdb -O admin enhanced_students || true

if [ -f "dump.sql" ]; then
  echo "Restoring database from dump..."
  psql -h localhost -U admin -d enhanced_students -f dump.sql
else
  echo "No dump file found (Create dump.sql in database/ directory), creating empty database."
fi

echo "Database initialization complete!"
echo "Database: enhanced_students"
echo "User: ADMIN"
echo "Password: 123456"
