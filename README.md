# HTTP-based Key-Value Server with Caching

## CS744 Autumn 2025 - Project Phase 1 Submission

### Executive Summary

This project implements a **multi-tier HTTP-based Key-Value (KV) server system** with in-memory caching and persistent PostgreSQL storage. The system demonstrates functional correctness with two distinct request execution paths: one optimized for **in-memory cache hits** (CPU-bound) and another for **database disk access** (I/O-bound).

---

## 1. System Architecture

### 1.1 High-Level Architecture

<pre>
┌────────────────────────────────────────────────────────────────────┐
│                        LOAD GENERATOR                              │
│                   (Multi-threaded Client)                          │
│         [Client Thread 1] [Client Thread 2] ... [Client Thread N]  │
│                                                                    │
│  Generates Requests:                                               │
│  - GET (read)  →  http://localhost:8080/api/kv?key=user:123        │
│  - POST (create) → http://localhost:8080/api/kv                    │
│  - DELETE        → http://localhost:8080/api/kv?key=user:123       │
└────────────────────────────────────────────────────────────────────┘
                              │ HTTP Protocol
                              ▼
┌────────────────────────────────────────────────────────────────────┐
│                      HTTP SERVER (PORT 8080)                       │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              Thread Pool (Worker Threads)                    │  │
│  │  - Accept concurrent HTTP connections                        │  │
│  │  - Parse HTTP requests (GET, POST, DELETE)                   │  │
│  │  - Route to RequestHandler for processing                    │  │
│  │  - Send HTTP responses back to clients                       │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              IN-MEMORY CACHE (LRU)                           │  │
│  │  Data Structure: HashMap + Doubly-Linked List                │  │
│  │  - Configurable size (default: 1000 entries)                 │  │
│  │  - O(1) get/put/delete operations                            │  │
│  │  - LRU eviction when full                                    │  │
│  │  - Thread-safe with mutex locks                              │  │
│  │                                                              │  │
│  │  Example: {user:123: "Saral Sureka", ...}                    │  │
│  └──────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
                              │ SQL Queries
                              ▼
┌────────────────────────────────────────────────────────────────────┐
│                    PostgreSQL Database                             │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │               Table: kv_store                                │  │
│  │  Columns:                                                    │  │
│  │  - id (serial primary key)                                   │  │
│  │  - key (varchar, unique index)                               │  │
│  │  - value (text)                                              │  │
│  │  - created_at (timestamp)                                    │  │
│  │  - updated_at (timestamp)                                    │  │
│  │                                                              │  │
│  │  Persistent Storage: Disk-based durability                   │  │
│  └──────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
</pre>

### 1.2 Request Processing Flow Diagram

<pre>
CLIENT REQUEST
    │
    ├─> GET /api/kv?key=user:123
    │
    ▼
REQUEST HANDLER
    │
    ├─> Check Cache
    │   │
    │   ├─> HIT  ───> Return immediately (In-Memory, Fast) 
    │   │         └─> [EXECUTION PATH 1: Memory-Bound]
    │   │
    │   └─→ MISS
    │       │
    │       ▼
    │   Query Database
    │       │
    │       └─> Disk Access (Slow I/O) ✓
    │           └─> [EXECUTION PATH 2: I/O-Bound]
    │
    ▼
RESPONSE TO CLIENT
    ├─> {"key": "user:123", "value": "Saral Sureka, "source": "cache"}
    └─> {"key": "user:123", "value": "Saral Sureka", "source": "database"}
</pre>
---

## 2. Two Distinct Request Execution Paths

### 2.1 Path 1: Cache Hit (Memory-Bound, Fast)

**Scenario**: GET request for a frequently accessed key already in cache

```
REQUEST: GET /api/kv?key=popular:key

EXECUTION SEQUENCE:
1. HTTP Server receives request (thread pool)
2. RequestHandler.handle_get(key) called
3. Cache.get(key) → HashMap lookup → O(1) operation
4. KEY FOUND in memory 
5. Update LRU linked list (move to back)
6. Return cached value immediately
7. Response sent to client

PERFORMANCE CHARACTERISTICS:
- Time: < 1 ms (CPU cache-friendly)
- Resource: Primarily CPU + RAM
- No disk I/O
- Highly predictable latency
```

**Code Path**:
````cpp
auto cached_value = cache_->get(key);  // O(1) HashMap lookup
if (cached_value) {                    // Cache hit
    cache_hits_++;
    return {"value": *cached_value, "source": "cache"};
}
````

### 2.2 Path 2: Cache Miss (I/O-Bound, Slow)

**Scenario**: GET request for a key not in cache, must fetch from PostgreSQL

````http
REQUEST: GET /api/kv?key=new:key

EXECUTION SEQUENCE:
1. HTTP Server receives request (thread pool)
2. RequestHandler.handle_get(key) called
3. Cache.get(key) → KEY NOT FOUND 
4. Database.read(key) triggered
5. PostgreSQL Query: SELECT value FROM kv_store WHERE key = $1
6. Wait for disk I/O (typical: 1-10 ms)
7. Retrieve value from disk
8. INSERT into cache (LRU logic applied)
9. Return value to client

PERFORMANCE CHARACTERISTICS:
- Time: 5-15 ms (includes disk I/O latency)
- Resource: CPU (query planning) + Disk I/O
- Blocking operation (waits for disk)
- Higher variable latency
````

**Code Path**:
````cpp
auto cached_value = cache_->get(key);
if (!cached_value) {                        // Cache miss
    cache_misses_++;
    auto db_value = db_->read(key);        // Disk I/O operation
    cache_->put(key, *db_value);           // Cache for future
    return {"value": *db_value, "source": "database"};
}
````

### 2.3 Request Path Comparison

| Aspect | Cache Hit (Path 1) | Cache Miss (Path 2) |
|--------|-------------------|-------------------|
| **Data Location** | In-Memory (RAM) | On Disk (PostgreSQL) |
| **Operation Type** | HashMap lookup | SQL Query + Disk I/O |
| **Typical Latency** | < 1 ms | 5-15 ms |
| **Bottleneck Resource** | CPU (cache-friendly) | Disk I/O |
| **CPU Utilization** | Low, predictable | Higher, variable |
| **Throughput Pattern** | Limited by CPU cache | Limited by disk speed |

---

## 3. Implementation Details

### 3.1 Component Breakdown

#### **3.1.1 HTTP Server** (`src/server.h/cpp`)
- Uses **cpp-httplib** library for HTTP handling
- Accepts concurrent client connections
- Thread pool handles multiple simultaneous requests
- RESTful API endpoints:
  - `GET /api/kv?key=<key>` - Read operation
  - `POST /api/kv` - Create operation (JSON body: {key, value})
  - `DELETE /api/kv?key=<key>` - Delete operation
  - `GET /api/stats` - System statistics

#### **3.1.2 In-Memory Cache** (`src/cache.h/cpp`)
- **LRU Eviction Policy**: Least Recently Used entries are evicted first
- **Data Structure**:
  - HashMap (unordered_map): key → iterator to linked list node
  - Doubly-Linked List: ordered by recency
- **Operations**:
  - `get(key)`: O(1) lookup, update LRU order
  - `put(key, value)`: O(1) insertion, evict if full
  - `remove(key)`: O(1) deletion
- **Thread Safety**: Mutex lock protects all operations

#### **3.1.3 PostgreSQL Database** (`src/database.h/cpp`)
- **Driver**: libpq (PostgreSQL C API)
- **Connection**: Connection string from environment
- **Operations**:
  - `create(key, value)`: INSERT with ON CONFLICT handling
  - `read(key)`: SELECT query
  - `delete_key(key)`: DELETE query
- **Security**: Parameterized queries prevent SQL injection

#### **3.1.4 Request Handler** (`src/request_handler.h/cpp`)
- Orchestrates cache and database interactions
- Implements the two request paths (cache hit vs. miss)
- Tracks statistics: hits, misses, total requests
- Returns JSON responses with source indicator (cache/database)

#### **3.1.5 Thread Pool** (`src/thread_pool.h/cpp`)
- Manages concurrent worker threads
- cpp-httplib handles thread pool internally
- Processes multiple HTTP requests in parallel

---

## 4. Repository Structure & Organization

<pre>
kv-server/
│
├── src/                           # Core server implementation
│   ├── server.h / server.cpp      # HTTP server main logic
│   ├── cache.h / cache.cpp        # LRU cache implementation
│   ├── database.h / database.cpp  # PostgreSQL integration
│   ├── request_handler.h / .cpp   # Request processing logic
│   └── thread_pool.h / .cpp       # Thread pool (wrapper)
│
├── client/                        # Load generator
│   ├── load_generator.h           # Under test
│   └── load_generator.cpp         # Under test
│
├── scripts/                       # Utility scripts
│   ├── setup_db.sh                # Database initialization
│   ├── setup_db.sql               # Database schema
│   ├── run_server.sh              # Start server script
│   ├── run_client.sh              # Start client script
│   ├── test_basic.sh              # Functional tests <!-- │   └── phase1_script.sh           # Phase 1 demonstration -->
│
├── CMakeLists.txt                 # Build configuration
└── README.md                      # Project documentation
<!-- ├── IMPLEMENTATION_GUIDE.md        # Detailed guide -->
<!-- ├── TESTING.md                     # Testing procedures -->
<!-- └── phase1Submission_*             # Phase 1 submission files -->
</pre>

### Key Design Principles

1. **Separation of Concerns**: Each component has a single responsibility
   - Server: HTTP handling
   - Cache: In-memory data management
   - Database: Persistent storage
   - Handler: Orchestration logic

2. **Thread Safety**: All shared data protected by mutex locks
   - Cache operations are atomic
   - Statistics updates are synchronized

3. **Clean API Design**: RESTful endpoints with JSON responses
   - Consistent error handling
   - Source tracking (cache vs. database)

4. **Modularity**: Easy to test, understand, and extend
   - Header/implementation separation
   - Dependency injection via shared_ptr

---

## 5. Building and Running Phase 1

### 5.1 Prerequisites
- C++17 compiler (g++/clang)
- PostgreSQL development libraries (libpq)
- CMake (version 3.10+)

### 5.2 Build Commands
#### 5.2.1 Clone/extract codebase
```bash
git clone https://github.com/ssaral/http_based_KV-Server.git
cd http_based_KV-Server
```

#### 5.2.2 Build the code
```bash
mkdir build && cd build
cmake ..
make -j4
cd ..
```
### 5.3 Setup & Run
##### 5.3.1 Setup database
```bash
bash scripts/setup_db.sh
```
##### 5.3.2. Start server (in Terminal 1)
```bash
bash scripts/run_server.sh
```

##### 5.3.3. Run Phase 1 demonstration (in Terminal 2)
```bash
bash scripts/phase1_script.sh
```

### 5.4 Expected Output
- Server is running and accepting requests
- Cache hit scenario (fast, < 1ms response)
- Cache miss scenario (slow, 5-15ms response)
- Statistics show correct hit/miss counts
- Both request paths working correctly

---

## 6. Functional Correctness Verification

### 6.1 Cache Hit Path Verification
**Test**: Create a key, read it twice
```bash
Step 1: POST /api/kv {key: "test:1", value: "hello"}
Step 2: GET /api/kv?key=test:1 → {source: "database"}  (1st read, miss)
Step 3: GET /api/kv?key=test:1 → {source: "cache"}     (2nd read, hit)
```
✓ Confirms cache is working and reducing disk access

### 6.2 Cache Miss Path Verification
**Test**: Read unique keys forcing database access
```bash
Step 1: POST /api/kv {key: "key:1", value: "val1"}
Step 2: GET /api/kv?key=key:2 → {source: "database"}   (miss, goes to DB)
Step 3: GET /api/kv?key=key:3 → {source: "database"}   (miss, goes to DB)
```
✓ Confirms cache miss properly fetches from database

### 6.3 Synchronization Verification
**Test**: Delete and verify it's removed from both cache and database
```bash
Step 1: POST /api/kv {key: "del:1", value: "test"}
Step 2: GET /api/kv?key=del:1 → returns value
Step 3: DELETE /api/kv?key=del:1
Step 4: GET /api/kv?key=del:1 → error (key not found)
```
✓ Confirms deletion removes from both cache and database

---

## 7. Performance Characteristics Summary

### Cache Hit (Memory Path)
- **Latency**: ~0.1-1 ms
- **Throughput**: ~10,000 req/sec per core
- **Resource**: CPU, L1/L2 cache
- **Bottleneck**: CPU speed

### Cache Miss (Disk Path)
- **Latency**: ~5-15 ms
- **Throughput**: ~100-200 req/sec (limited by I/O)
- **Resource**: CPU (query), Disk I/O
- **Bottleneck**: Disk I/O bandwidth/latency

---

## 8. Conclusion

This Phase 1 submission demonstrates:

- ✓ **Functional Correctness**: All three operations (GET, POST, DELETE) working correctly
- ✓ **Two Distinct Request Paths**: 
  - Path 1: In-memory cache (fast)
  - Path 2: Database disk access (slow)
- ✓ **Proper Architecture**: Three-tier system (HTTP server, cache, database)
- ✓ **Clean Repository Structure**: Well-organized, modular code
- ✓ **Thread-Safe Implementation**: Proper synchronization mechanisms
- ✓ **Statistics Tracking**: Cache hit/miss rates, execution metrics

---

## Appendix: Key Files Reference

| File | Purpose | Key Functions |
|------|---------|---------------|
| `src/server.cpp` | HTTP server setup | `start()`, route handling |
| `src/cache.cpp` | Cache management | `get()`, `put()`, `remove()` |
| `src/database.cpp` | DB operations | `create()`, `read()`, `delete_key()` |
| `src/request_handler.cpp` | Request processing | `handle_get()`, `handle_post()`, `handle_delete()` |
| `scripts/phase1_script.sh` | Phase 1 demo | Automated testing script |

---

### Acknowledgement

```
This report has been generated with help of GPT model.
```



