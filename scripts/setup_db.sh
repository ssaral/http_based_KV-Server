#!/bin/bash

# Setup PostgreSQL database for KV server
echo "Setting up PostgreSQL database..."

# Step 1: Create the database if it doesn't exist
DB_EXISTS=$(psql -U postgres -h localhost -tAc "SELECT 1 FROM pg_database WHERE datname='kvstore'")

if [ "$DB_EXISTS" != "1" ]; then
    echo "Creating database kvstore..."
    createdb -U postgres -h localhost kvstore
else
    echo "Database kvstore already exists."
fi

# Step 2: Create tables and grant privileges
psql -U postgres -h localhost -d kvstore <<EOF
CREATE TABLE IF NOT EXISTS kv_store (
    id SERIAL PRIMARY KEY,
    key VARCHAR(255) NOT NULL UNIQUE,
    value TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_key ON kv_store(key);

GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO postgres;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO postgres;
EOF

echo "Database setup complete!"
