#!/bin/bash
set -e

echo "Initializing PostgreSQL database..."

SYSTEM_NAME=$(uname)

if [[ $SYSTEM_NAME == "Darwin" ]]; then
  echo "Starting PostgreSQL on macOS..."
  if ! pg_isready -q; then
    brew services run postgresql@17
    echo -n "Waiting for PostgreSQL to start"
    for i in {1..30}; do
      if pg_isready -q; then
        echo " OK"
        break
      fi
      echo -n "."
      sleep 1
      if [ "$i" -eq 30 ]; then
        echo " ERROR: PostgreSQL failed to start"
        exit 1
      fi
    done
  else
    echo "PostgreSQL is already running"
  fi
fi

if [[ $SYSTEM_NAME == "Linux" ]]; then
  echo "Starting PostgreSQL on Linux..."
  sudo service postgresql start || sudo systemctl start postgresql
  sleep 2
fi

# Создаем пользователя и БД с учетом ОС
if [[ $SYSTEM_NAME == "Darwin" ]]; then
  psql -c "CREATE USER admin WITH PASSWORD '123456' SUPERUSER;" 2>/dev/null || true
  createdb -O admin enhanced_students 2>/dev/null || true
elif [[ $SYSTEM_NAME == "Linux" ]]; then
  sudo -u postgres psql -c "CREATE USER admin WITH PASSWORD '123456' SUPERUSER;" 2>/dev/null || true
  sudo -u postgres createdb -O admin enhanced_students 2>/dev/null || true
fi

if [ -f "dump.sql" ]; then
  echo "Restoring database from dump..."
  PGPASSWORD='123456' psql -h localhost -U admin -d enhanced_students -f dump.sql
else
  echo "No dump file found (Create dump.sql in database/ directory), creating empty database."
fi

echo "Database initialization complete!"
echo "Database: enhanced_students"
echo "User: admin"
echo "Password: 123456"
echo "Host: localhost"
echo "Port: 5432"
