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
    "$MYSQL_BIN" "${MYSQL_ARGS[@]}" "$@"
}

mysql_run --execute="
CREATE TABLE IF NOT EXISTS schema_migrations (
  version VARCHAR(255) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  checksum CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  applied_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (version)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
" >/dev/null

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
done

printf 'Database migrations are current.\n'
