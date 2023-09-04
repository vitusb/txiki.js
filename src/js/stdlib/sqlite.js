/* global tjs */

const { core } = tjs[Symbol.for('tjs.internal')];
const sqlite3 = core._sqlite3;

const kSqlite3Handle = Symbol('kSqlite3Handle');

class Database {
    constructor(dbName = ':memory:') {
        this[kSqlite3Handle] = sqlite3.open(dbName);
    }

    close() {
        if (this[kSqlite3Handle]) {
            sqlite3.close(this[kSqlite3Handle]);
        }
    }

    prepare(sql) {
        if (!this[kSqlite3Handle]) {
            throw new Error('Invalid DB');
        }

        return new Statement(sqlite3.prepare(this[kSqlite3Handle], sql));
    }
}

const kSqlite3Stmt = Symbol('kSqlite3Stmt');

class Statement {
    constructor(stmt) {
        this[kSqlite3Stmt] = stmt;
    }

    finalize() {
        sqlite3.stmt_finalize(this[kSqlite3Stmt]);
    }

    toString() {
        return sqlite3.stmt_expand(this[kSqlite3Stmt]);
    }

    all() {
        return sqlite3.stmt_all(this[kSqlite3Stmt]);
    }

    run() {
        sqlite3.stmt_run(this[kSqlite3Stmt])
    }
}


export { Database };
