/*
 * txiki.js
 *
 * Copyright (c) 2023-present Saúl Ibarra Corretgé <s@saghul.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "_sqlite3.h"

#include "private.h"


static JSClassID tjs_sqlite3_class_id;

typedef struct {
    sqlite3 *handle;
} TJSSqlite3Handle;

static void tjs_sqlite3_finalizer(JSRuntime *rt, JSValue val) {
    TJSSqlite3Handle *h = JS_GetOpaque(val, tjs_sqlite3_class_id);
    if (h && h->handle) {
        sqlite3_close(h->handle);
        h->handle = NULL;
    }
}

static JSClassDef tjs_sqlite3_class = {
    "Handle",
    .finalizer = tjs_sqlite3_finalizer,
};

static JSValue tjs_new_sqlite3(JSContext *ctx, sqlite3 *handle) {
    TJSSqlite3Handle *h;
    JSValue obj;

    obj = JS_NewObjectClass(ctx, tjs_sqlite3_class_id);
    if (JS_IsException(obj))
        return obj;

    h = js_mallocz(ctx, sizeof(*h));
    if (!h) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }

    h->handle = handle;

    JS_SetOpaque(obj, h);
    return obj;
}

static TJSSqlite3Handle *tjs_sqlite3_get(JSContext *ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, tjs_sqlite3_class_id);
}

static JSClassID tjs_sqlite3_stmt_class_id;

typedef struct {
    sqlite3_stmt *stmt;
} TJSSqlite3Stmt;

static void tjs_sqlite3_stmt_finalizer(JSRuntime *rt, JSValue val) {
    TJSSqlite3Stmt *h = JS_GetOpaque(val, tjs_sqlite3_stmt_class_id);
    if (h && h->stmt) {
        sqlite3_finalize(h->stmt);
        h->stmt = NULL;
    }
}

static JSClassDef tjs_sqlite3_stmt_class = {
    "Statement",
    .finalizer = tjs_sqlite3_stmt_finalizer,
};

static JSValue tjs_new_sqlite3_stmt(JSContext *ctx, sqlite3_stmt *stmt) {
    TJSSqlite3Stmt *h;
    JSValue obj;

    obj = JS_NewObjectClass(ctx, tjs_sqlite3_stmt_class_id);
    if (JS_IsException(obj))
        return obj;

    h = js_mallocz(ctx, sizeof(*h));
    if (!h) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }

    h->stmt = stmt;

    JS_SetOpaque(obj, h);
    return obj;
}

static TJSSqlite3Stmt *tjs_sqlite3_stmt_get(JSContext *ctx, JSValueConst obj) {
    return JS_GetOpaque2(ctx, obj, tjs_sqlite3_stmt_class_id);
}

JSValue tjs_throw_sqlite3_errno(JSContext *ctx, int err) {
    JSValue obj;
    obj = JS_NewError(ctx);
    JS_DefinePropertyValueStr(ctx,
                              obj,
                              "message",
                              JS_NewString(ctx, sqlite3_errstr(err)),
                              JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    JS_DefinePropertyValueStr(ctx, obj, "errno", JS_NewInt32(ctx, err), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    if (JS_IsException(obj))
        obj = JS_NULL;
    return JS_Throw(ctx, obj);
}

static JSValue tjs_sqlite3_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *db_name = JS_ToCString(ctx, argv[0]);

    if (!db_name) {
        return JS_EXCEPTION;
    }

    sqlite3 *handle = NULL;
    int r = sqlite3_open_v2(db_name, &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    JS_FreeCString(ctx, db_name);

    if (r != SQLITE_OK) {
        return tjs_throw_sqlite3_errno(ctx, r);
    }

    JSValue obj = tjs_new_sqlite3(ctx, handle);
    if (JS_IsException(obj)) {
        sqlite3_close(handle);
    }

    return obj;
}

static JSValue tjs_sqlite3_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSSqlite3Handle *h = tjs_sqlite3_get(ctx, argv[0]);

    if (!h)
        return JS_EXCEPTION;

    int r = sqlite3_close(h->handle);
    if (r != SQLITE_OK) {
        return tjs_throw_sqlite3_errno(ctx, r);
    }

    h->handle = NULL;

    return JS_UNDEFINED;
}

static JSValue tjs_sqlite3_prepare(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSSqlite3Handle *h = tjs_sqlite3_get(ctx, argv[0]);

    if (!h)
        return JS_EXCEPTION;

    const char *sql = JS_ToCString(ctx, argv[1]);

    if (!sql) {
        return JS_EXCEPTION;
    }

    sqlite3_stmt *stmt = NULL;
    int r = sqlite3_prepare_v2(h->handle, sql, -1, &stmt, NULL);

    JS_FreeCString(ctx, sql);

    if (r != SQLITE_OK) {
        return tjs_throw_sqlite3_errno(ctx, r);
    }

    JSValue obj = tjs_new_sqlite3_stmt(ctx, stmt);
    if (JS_IsException(obj)) {
        sqlite3_finalize(stmt);
    }

    return obj;
}

static JSValue tjs_sqlite3_stmt_finalize(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSSqlite3Stmt *h = tjs_sqlite3_stmt_get(ctx, argv[0]);

    if (!h)
        return JS_EXCEPTION;

    int r = sqlite3_finalize(h->stmt);
    if (r != SQLITE_OK) {
        return tjs_throw_sqlite3_errno(ctx, r);
    }

    h->stmt = NULL;

    return JS_UNDEFINED;
}

static JSValue tjs_sqlite3_stmt_expand(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSSqlite3Stmt *h = tjs_sqlite3_stmt_get(ctx, argv[0]);

    if (!h)
        return JS_EXCEPTION;

    char *sql = sqlite3_expanded_sql(h->stmt);
    if (sql == NULL) {
        return JS_ThrowOutOfMemory(ctx);
    }

    return JS_NewString(ctx, sql);
}

static JSValue tjs_stmt2obj(JSContext *ctx, TJSSqlite3Stmt *h) {
    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
    int count = sqlite3_column_count(h->stmt);

    for (int i = 0; i < count; i++) {
        const char *name = sqlite3_column_name(h->stmt, i);
        JSValue value;

        switch (sqlite3_column_type(h->stmt, i)) {
            case SQLITE_INTEGER: {
                value = JS_NewInt64(ctx, sqlite3_column_int64(h->stmt, i));
                break;
            }
            case SQLITE_FLOAT: {
                value = JS_NewFloat64(ctx, sqlite3_column_double(h->stmt, i));
                break;
            }
            case SQLITE3_TEXT: {
                value = JS_NewString(ctx, sqlite3_column_text(h->stmt, i));
                break;
            }
            case SQLITE_BLOB: {
                value = TJS_NewUint8Array(ctx, (uint8_t *)sqlite3_column_blob(h->stmt, i), sqlite3_column_bytes(h->stmt, i));
                break;
            }
            default: {
                value = JS_NULL;
                break;
            }
        }

        JS_DefinePropertyValueStr(ctx, obj, name, value, JS_PROP_C_W_E);
    }

    return obj;
}

static JSValue tjs_sqlite3_stmt_all(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSSqlite3Stmt *h = tjs_sqlite3_stmt_get(ctx, argv[0]);

    if (!h)
        return JS_EXCEPTION;

    int r = sqlite3_reset(h->stmt);
    if (r != SQLITE_OK) {
        return tjs_throw_sqlite3_errno(ctx, r);
    }

    JSValue result = JS_NewArray(ctx);
    uint32_t i = 0;

    while ((r = sqlite3_step(h->stmt)) == SQLITE_ROW) {
        JS_DefinePropertyValueUint32(ctx, result, i, tjs_stmt2obj(ctx, h), JS_PROP_C_W_E);
        i++;
    }

    if (r != SQLITE_OK && r != SQLITE_DONE) {
        JS_FreeValue(ctx, result);
        return tjs_throw_sqlite3_errno(ctx, r);
    }

    return result;
}

static JSValue tjs_sqlite3_stmt_run(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    TJSSqlite3Stmt *h = tjs_sqlite3_stmt_get(ctx, argv[0]);

    if (!h)
        return JS_EXCEPTION;

    int r = sqlite3_reset(h->stmt);
    if (r != SQLITE_OK) {
        return tjs_throw_sqlite3_errno(ctx, r);
    }

    sqlite3_step(h->stmt);

    return JS_UNDEFINED;
}

static const JSCFunctionListEntry tjs_sqlite3_funcs[] = {
    TJS_CFUNC_DEF("open", 1, tjs_sqlite3_open),
    TJS_CFUNC_DEF("close", 1, tjs_sqlite3_close),
    TJS_CFUNC_DEF("prepare", 2, tjs_sqlite3_prepare),
    TJS_CFUNC_DEF("stmt_finalize", 1, tjs_sqlite3_stmt_finalize),
    TJS_CFUNC_DEF("stmt_expand", 1, tjs_sqlite3_stmt_expand),
    TJS_CFUNC_DEF("stmt_all", 0, tjs_sqlite3_stmt_all),
    TJS_CFUNC_DEF("stmt_run", 0, tjs_sqlite3_stmt_run),
};

void tjs__mod_sqlite3_init(JSContext *ctx, JSValue ns) {
    /* Handle object */
    JS_NewClassID(&tjs_sqlite3_class_id);
    JS_NewClass(JS_GetRuntime(ctx), tjs_sqlite3_class_id, &tjs_sqlite3_class);
    JS_SetClassProto(ctx, tjs_sqlite3_class_id, JS_NULL);

    /* Statement object */
    JS_NewClassID(&tjs_sqlite3_stmt_class_id);
    JS_NewClass(JS_GetRuntime(ctx), tjs_sqlite3_stmt_class_id, &tjs_sqlite3_stmt_class);
    JS_SetClassProto(ctx, tjs_sqlite3_stmt_class_id, JS_NULL);

    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
    JS_SetPropertyFunctionList(ctx, obj, tjs_sqlite3_funcs, countof(tjs_sqlite3_funcs));

    JS_DefinePropertyValueStr(ctx, ns, "_sqlite3", obj, JS_PROP_C_W_E);
}
