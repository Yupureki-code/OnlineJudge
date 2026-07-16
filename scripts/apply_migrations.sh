#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
MIGRATIONS_DIR=${OJ_MIGRATIONS_DIR:-"$ROOT_DIR/sql/migrations"}
MYSQL_BIN=${MYSQL_BIN:-mysql}

DB_HOST=${OJ_DB_HOST:-localhost}
DB_PORT=${OJ_DB_PORT:-3306}
DB_USER=${OJ_DB_USER:-oj_server}
DB_NAME=${OJ_DB_NAME:-myoj}
MYSQL_PWD=${OJ_DB_PASS:-password}
export MYSQL_PWD

[[ "$DB_PORT" =~ ^[0-9]+$ ]] || { printf 'Invalid OJ_DB_PORT\n' >&2; exit 2; }
[[ -n "$DB_HOST" && -n "$DB_USER" && -n "$DB_NAME" ]] || {
    printf 'Database host, user, and name must not be empty\n' >&2
    exit 2
}
[[ -d "$MIGRATIONS_DIR" ]] || { printf 'Migration directory not found: %s\n' "$MIGRATIONS_DIR" >&2; exit 2; }
command -v "$MYSQL_BIN" >/dev/null 2>&1 || { printf 'MySQL client not found: %s\n' "$MYSQL_BIN" >&2; exit 2; }

MYSQL_ARGS=(
    --protocol=TCP
    --host="$DB_HOST"
    --port="$DB_PORT"
    --user="$DB_USER"
    --database="$DB_NAME"
    --default-character-set=utf8mb4
    --show-warnings
)

mysql_run() {
    if [[ -n ${lock_fd:-} ]]; then
        "$MYSQL_BIN" "${MYSQL_ARGS[@]}" "$@" {lock_fd}>&-
    else
        "$MYSQL_BIN" "${MYSQL_ARGS[@]}" "$@"
    fi
}

mysql_run --execute="
CREATE TABLE IF NOT EXISTS schema_migrations (
  version VARCHAR(255) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  checksum CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  applied_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (version)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE IF NOT EXISTS schema_migration_lock_owner (
  lock_id TINYINT UNSIGNED NOT NULL,
  owner_token VARCHAR(128) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  connection_id BIGINT UNSIGNED NOT NULL,
  acquired_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (lock_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
" >/dev/null

lock_holder_pid=''
lock_token=''
lock_dir=''
lock_fd=''
release_migration_lock() {
    if [[ -n "$lock_fd" ]]; then
        printf "DO RELEASE_LOCK('oj_schema_migration');\nDELETE FROM schema_migration_lock_owner WHERE lock_id=1 AND owner_token='%s';\n" \
            "$lock_token" >&$lock_fd 2>/dev/null || true
        exec {lock_fd}>&-
    fi
    if [[ -n "$lock_holder_pid" ]]; then
        wait "$lock_holder_pid" >/dev/null 2>&1 || true
        lock_holder_pid=''
    fi
    [[ -z "$lock_dir" ]] || rm -rf -- "$lock_dir"
}

if [[ ${OJ_MIGRATION_TEST_SKIP_LOCK:-0} != 1 ]]; then
    lock_token="migration-$PPID-$$-$RANDOM"
    lock_dir=$(mktemp -d)
    mkfifo "$lock_dir/session"
    mysql_run <"$lock_dir/session" >/dev/null 2>&1 &
    lock_holder_pid=$!
    exec {lock_fd}>"$lock_dir/session"
    printf "
SET @lock_acquired = GET_LOCK('oj_schema_migration', 300);
INSERT INTO schema_migration_lock_owner (lock_id, owner_token, connection_id)
SELECT 1, '%s', CONNECTION_ID() WHERE @lock_acquired = 1
ON DUPLICATE KEY UPDATE owner_token=VALUES(owner_token), connection_id=VALUES(connection_id), acquired_at=CURRENT_TIMESTAMP(6);
" "$lock_token" >&$lock_fd
    trap release_migration_lock EXIT INT TERM

    lock_acquired=0
    for _ in {1..3000}; do
        owner=$(mysql_run --batch --skip-column-names \
            --execute="SELECT owner_token FROM schema_migration_lock_owner WHERE lock_id=1" 2>/dev/null || true)
        if [[ "$owner" == "$lock_token" ]]; then
            lock_acquired=1
            break
        fi
        if ! kill -0 "$lock_holder_pid" >/dev/null 2>&1; then
            break
        fi
        sleep 0.1
    done
    if [[ $lock_acquired != 1 ]]; then
        printf 'Could not acquire migration lock\n' >&2
        exit 1
    fi
fi

checksum_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | cut -d' ' -f1
    else
        printf 'A SHA-256 checksum utility is required\n' >&2
        return 1
    fi
}

export LC_ALL=C
shopt -s nullglob
migrations=("$MIGRATIONS_DIR"/*.sql)
shopt -u nullglob
((${#migrations[@]} > 0)) || { printf 'No migration files found in %s\n' "$MIGRATIONS_DIR" >&2; exit 2; }

for migration in "${migrations[@]}"; do
    version=${migration##*/}
    [[ "$version" =~ ^[0-9]{8}_[0-9]{2}_[A-Za-z0-9_]+\.sql$ ]] || {
        printf 'Invalid migration filename: %s\n' "$version" >&2
        exit 2
    }

    checksum=$(checksum_file "$migration")
    applied_checksum=$(mysql_run --batch --skip-column-names \
        --execute="SELECT checksum FROM schema_migrations WHERE version = '$version' LIMIT 1;")

    if [[ -n "$applied_checksum" ]]; then
        if [[ "$applied_checksum" != "$checksum" ]]; then
            printf 'Migration checksum mismatch: %s\n' "$version" >&2
            exit 1
        fi
        printf 'Already applied: %s\n' "$version"
        continue
    fi

    printf 'Applying: %s\n' "$version"
    {
        printf 'SET autocommit=0;\nSTART TRANSACTION;\n'
        printf '%s\n' "-- Applying $version"
        command cat -- "$migration"
        printf '\nINSERT INTO schema_migrations (version, checksum) VALUES ('"'"'%s'"'"', '"'"'%s'"'"');\n' "$version" "$checksum"
        printf 'COMMIT;\n'
    } | mysql_run
    printf 'Applied: %s\n' "$version"
done

printf 'Database migrations are current.\n'
release_migration_lock
trap - EXIT INT TERM
