# DBMS - Database Management System

A C++/Qt-based Database Management System with full CRUD operations, SQL parsing, and user authentication.

## Features

- **User Authentication**: SHA256 password encryption, login/register
- **Database Management**: Create/drop databases, create/drop/alter tables
- **Data Operations**: Full CRUD with type validation (INT/TEXT/DOUBLE/BOOLEAN)
- **Binary Storage**: .trd binary format for efficient data storage
- **Schema Management**: .tdf JSON files for table structure persistence
- **SQL Parsing**: DDL statement parsing (CREATE/DROP DATABASE/TABLE)
- **UI Interface**: Tree view for databases/tables, table view for data display

## Project Structure

```
DBMS/
├── main.cpp              # Application entry point
├── mainwindow.cpp/h      # Main UI window
├── mainwindow.ui         # Qt UI designer file
├── storagemanager.cpp/h  # Storage management (red circle A)
├── authmanager.cpp/h     # User authentication (orange circle B)
├── schemamanager.cpp/h   # Schema management (orange circle B)
├── recordmanager.cpp/h   # Record management (blue circle C)
├── sqlparser.cpp/h       # SQL parser (orange circle B)
├── common.h              # Common type definitions
├── CMakeLists.txt        # CMake build configuration
└── users.json            # User credentials (auto-generated)
```

## Build & Run

### Requirements
- Qt 6.10+
- CMake 3.16+
- C++17 compatible compiler

### Build Steps
```bash
# Clone and open project
cmake -S . -B build
cmake --build build

# Run (GUI mode)
./build/DBMS

# Run tests only
# Set RUN_TESTS_ONLY = true in main.cpp, then rebuild
```

## Data Storage

```
./data/
├── {username}/
│   ├── {dbName}/
│   │   ├── {tableName}.tdf   # Table schema (JSON)
│   │   ├── {tableName}.trd   # Data records (binary)
│   │   ├── {dbName}.tb       # Table block header
│   │   └── ruanko.log        # Operation logs
│   └── users.json            # User credentials
```

## Phases

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | ✅ | Basic: Database folder, login, JSON data |
| 2 | ✅ | Advanced: Table files, schema, binary storage |
| 3 | ✅ | Advanced: Drop/alter, validation, full CRUD |
| 4 | ✅ | SQL parsing: DDL statement execution |

## Default Credentials

- **Username**: admin
- **Password**: 123456

## License

MIT License
