const kStorageMap = Symbol('kStorageMap');

class Storage {
    constructor() {
        this[kStorageMap] = new Map();
    }

    getItem(key) {
        const stringKey = String(key);

        if (this[kStorageMap].has(key)) {
            return this[kStorageMap].get(stringKey);
        }

        return null;
    }

    setItem(key, val) {
        this[kStorageMap].set(String(key), String(val));
    }

    removeItem(key) {
        this[kStorageMap].delete(String(key));
    }

    clear() {
        this[kStorageMap].clear();
    }

    key(i) {
        if (typeof i === 'undefined') {
            throw new TypeError('Failed to execute \'key\' on \'Storage\': 1 argument required, but only 0 present.');
        }

        const keys = Array.from(this[kStorageMap].keys());

        return keys[i];
    }

    get length() {
        return this[kStorageMap].size;
    }

    get [Symbol.toStringTag]() {
        return 'Storage';
    }
}

const storageProxyHandler = {
    set: function (target, prop, value) {
        target.setItem(prop, value);

        return true;
    },
    get: function (target, prop) {
        if (prop in Storage.prototype) {
            if (typeof target[prop] === 'function') {
                return (...args) => target[prop].apply(target, args);
            }

            return target[prop];
        }

        return target.getItem(prop);
    }
};

const _sessionStorage = new Proxy(new Storage(), storageProxyHandler);

Object.defineProperty(globalThis, 'sessionStorage', {
    enumerable: true,
    configurable: true,
    writable: true,
    value: _sessionStorage
});

const kStorageFile = Symbol('kStorageFile');

class PersistentStorage extends Storage {
    constructor() {
        super();

        this[kStorageFile] = null;
    }

    setItem(key, val) {
        super.setItem(key, val);

        // TODO.
    }

    removeItem(key) {
        super.removeItem(key);

        // TODO.
    }

    clear() {
        super.clear();

        // TODO.
    }
}

const _localStorage = new Proxy(new PersistentStorage(), storageProxyHandler);

Object.defineProperty(globalThis, 'localStorage', {
    enumerable: true,
    configurable: true,
    writable: true,
    value: _localStorage
});
