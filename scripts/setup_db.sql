-- Create database
CREATE DATABASE IF NOT EXISTS kvstore;

-- Connect to kvstore database
\c kvstore;

-- Create kv_store table
CREATE TABLE IF NOT EXISTS kv_store (
    id SERIAL PRIMARY KEY,
    key VARCHAR(255) NOT NULL UNIQUE,
    value TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create index on key for faster lookups
CREATE INDEX IF NOT EXISTS idx_key ON kv_store(key);

-- Grant permissions to postgres user
GRANT ALL PRIVILEGES ON DATABASE kvstore TO postgres;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO postgres;
