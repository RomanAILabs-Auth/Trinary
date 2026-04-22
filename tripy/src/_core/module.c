/*
 * tripy/_core/module.c — CPython extension binding to libtrinary.
 * Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
 * Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
 * Contact: daniel@romanailabs.com, romanailabs@gmail.com
 * Website: romanailabs.com
 *
 * Exposes:
 *   tripy._core.init()                       -> None (ensures engine init)
 *   tripy._core.version()                    -> str
 *   tripy._core.features()                   -> dict[str, bool]
 *   tripy._core.active_variant(name: str)    -> str
 *   tripy._core.braincore(neurons, iter)     -> dict
 *   tripy._core.harding_gate(count, iter)    -> dict
 *   tripy._core.lattice_flip(bits, iter)     -> dict
 *   tripy._core.run_tri(path: str)           -> int   (0 on success, nonzero on error)
 *
 * Statically linked against libtrinary — no DLL at runtime.
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "trinary/trinary.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static PyObject *TriPyError = NULL;

static PyObject *_ensure_init(void) {
    trinary_v1_status rc = trinary_v1_init();
    if (rc != TRINARY_OK) {
        PyErr_SetString(TriPyError, "trinary_v1_init failed");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *py_init(PyObject *self, PyObject *args) {
    (void)self; (void)args;
    return _ensure_init();
}

static PyObject *py_version(PyObject *self, PyObject *args) {
    (void)self; (void)args;
    return PyUnicode_FromString(trinary_v1_version());
}

static PyObject *py_features(PyObject *self, PyObject *args) {
    (void)self; (void)args;
    uint32_t f = trinary_v1_cpu_features();
    PyObject *d = PyDict_New();
    if (!d) return NULL;
    #define PUT(name, flag) do { \
        PyObject *v = (f & flag) ? Py_True : Py_False; Py_INCREF(v); \
        if (PyDict_SetItemString(d, name, v) < 0) { Py_DECREF(v); Py_DECREF(d); return NULL; } \
        Py_DECREF(v); \
    } while (0)
    PUT("sse2",    TRINARY_CPU_SSE2);
    PUT("sse42",   TRINARY_CPU_SSE42);
    PUT("avx",     TRINARY_CPU_AVX);
    PUT("avx2",    TRINARY_CPU_AVX2);
    PUT("avx512f", TRINARY_CPU_AVX512F);
    PUT("bmi2",    TRINARY_CPU_BMI2);
    PUT("popcnt",  TRINARY_CPU_POPCNT);
    PUT("fma",     TRINARY_CPU_FMA);
    #undef PUT
    return d;
}

static PyObject *py_active_variant(PyObject *self, PyObject *args) {
    (void)self;
    const char *name = NULL;
    if (!PyArg_ParseTuple(args, "s", &name)) return NULL;
    return PyUnicode_FromString(trinary_v1_active_variant(name));
}

static PyObject *py_braincore(PyObject *self, PyObject *args, PyObject *kw) {
    (void)self;
    Py_ssize_t neurons = 4000000;
    Py_ssize_t iter    = 1000;
    unsigned char thresh = 200;
    static char *kwlist[] = {"neurons", "iterations", "threshold", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|nnB:braincore", kwlist,
                                     &neurons, &iter, &thresh)) return NULL;
    if (neurons <= 0 || iter <= 0) {
        PyErr_SetString(PyExc_ValueError, "neurons and iterations must be positive");
        return NULL;
    }
    if (neurons % 32 != 0) neurons = (neurons + 31) / 32 * 32;
    uint8_t *pot = (uint8_t *)trinary_v1_alloc((size_t)neurons, 32);
    uint8_t *inp = (uint8_t *)trinary_v1_alloc((size_t)neurons, 32);
    if (!pot || !inp) {
        if (pot) trinary_v1_free(pot);
        if (inp) trinary_v1_free(inp);
        PyErr_NoMemory();
        return NULL;
    }
    trinary_v1_rng_seed(0);
    trinary_v1_rng_fill(inp, (size_t)neurons);
    for (Py_ssize_t i = 0; i < neurons; ++i) inp[i] %= 50;
    for (Py_ssize_t i = 0; i < neurons; ++i) pot[i] = 0;
    double t0, t1;
    trinary_v1_status rc;
    Py_BEGIN_ALLOW_THREADS
    t0 = trinary_v1_now_seconds();
    rc = trinary_v1_braincore_u8(pot, inp, (size_t)neurons, (size_t)iter, thresh);
    t1 = trinary_v1_now_seconds();
    Py_END_ALLOW_THREADS
    trinary_v1_free(pot); trinary_v1_free(inp);
    if (rc != TRINARY_OK) {
        PyErr_SetString(TriPyError, "braincore failed");
        return NULL;
    }
    return Py_BuildValue(
        "{s:s,s:s,s:n,s:n,s:d,s:d}",
        "kernel", "braincore",
        "variant", trinary_v1_active_variant("braincore"),
        "neurons", neurons,
        "iterations", iter,
        "seconds", t1 - t0,
        "giga_neurons_per_sec", ((double)neurons * (double)iter) / (t1 - t0) / 1e9);
}

static PyObject *py_harding_gate(PyObject *self, PyObject *args, PyObject *kw) {
    (void)self;
    Py_ssize_t count = 16 * 1024 * 1024;
    Py_ssize_t iter  = 64;
    static char *kwlist[] = {"count", "iterations", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|nn:harding_gate", kwlist,
                                     &count, &iter)) return NULL;
    if (count <= 0 || iter <= 0) {
        PyErr_SetString(PyExc_ValueError, "count and iterations must be positive");
        return NULL;
    }
    if (count % 16 != 0) count = (count + 15) / 16 * 16;
    int16_t *a = (int16_t *)trinary_v1_alloc(count * sizeof(int16_t), 32);
    int16_t *b = (int16_t *)trinary_v1_alloc(count * sizeof(int16_t), 32);
    int16_t *o = (int16_t *)trinary_v1_alloc(count * sizeof(int16_t), 32);
    if (!a || !b || !o) {
        if (a) trinary_v1_free(a);
        if (b) trinary_v1_free(b);
        if (o) trinary_v1_free(o);
        PyErr_NoMemory(); return NULL;
    }
    trinary_v1_rng_fill(a, count * sizeof(int16_t));
    trinary_v1_rng_fill(b, count * sizeof(int16_t));
    double t0, t1;
    Py_BEGIN_ALLOW_THREADS
    t0 = trinary_v1_now_seconds();
    for (Py_ssize_t k = 0; k < iter; ++k)
        trinary_v1_harding_gate_i16(o, a, b, count);
    t1 = trinary_v1_now_seconds();
    Py_END_ALLOW_THREADS
    trinary_v1_free(a); trinary_v1_free(b); trinary_v1_free(o);
    return Py_BuildValue(
        "{s:s,s:s,s:n,s:n,s:d,s:d}",
        "kernel", "harding_gate_i16",
        "variant", trinary_v1_active_variant("harding"),
        "elements", count,
        "iterations", iter,
        "seconds", t1 - t0,
        "giga_elements_per_sec", ((double)count * (double)iter) / (t1 - t0) / 1e9);
}

static PyObject *py_lattice_flip(PyObject *self, PyObject *args, PyObject *kw) {
    (void)self;
    Py_ssize_t bits = 1 << 24;  /* 16.7M bits */
    Py_ssize_t iter = 1000;
    static char *kwlist[] = {"bits", "iterations", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|nn:lattice_flip", kwlist,
                                     &bits, &iter)) return NULL;
    if (bits <= 0 || iter <= 0) {
        PyErr_SetString(PyExc_ValueError, "bits and iterations must be positive");
        return NULL;
    }
    Py_ssize_t words = (bits + 63) / 64;
    words = (words + 3) / 4 * 4;
    uint64_t *lat = (uint64_t *)trinary_v1_alloc(words * sizeof(uint64_t), 32);
    if (!lat) { PyErr_NoMemory(); return NULL; }
    memset(lat, 0x55, words * sizeof(uint64_t));
    double t0, t1;
    Py_BEGIN_ALLOW_THREADS
    t0 = trinary_v1_now_seconds();
    trinary_v1_lattice_flip(lat, (size_t)words, (size_t)iter, 0xFFFFFFFFFFFFFFFFULL);
    t1 = trinary_v1_now_seconds();
    Py_END_ALLOW_THREADS
    trinary_v1_free(lat);
    return Py_BuildValue(
        "{s:s,s:s,s:n,s:n,s:d,s:d}",
        "kernel", "lattice_flip",
        "variant", trinary_v1_active_variant("flip"),
        "bits", bits,
        "iterations", iter,
        "seconds", t1 - t0,
        "giga_bits_per_sec", ((double)bits * (double)iter) / (t1 - t0) / 1e9);
}

static PyObject *py_run_tri(PyObject *self, PyObject *args) {
    (void)self;
    const char *path = NULL;
    if (!PyArg_ParseTuple(args, "s", &path)) return NULL;
    trinary_v1_status rc;
    Py_BEGIN_ALLOW_THREADS
    rc = trinary_v1_run_file(path);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong((long)rc);
}

#define TRIPY_KW_METHOD(fn) ((PyCFunction)(void (*)(void))(fn))

static PyMethodDef TriPyMethods[] = {
    {"init",            py_init,            METH_NOARGS,                 "Initialize the engine."},
    {"version",         py_version,         METH_NOARGS,                 "Engine version string."},
    {"features",        py_features,        METH_NOARGS,                 "CPU feature dict."},
    {"active_variant",  py_active_variant,  METH_VARARGS,                "Best variant for a kernel name."},
    {"braincore",       TRIPY_KW_METHOD(py_braincore),    METH_VARARGS|METH_KEYWORDS, "Run braincore kernel."},
    {"harding_gate",    TRIPY_KW_METHOD(py_harding_gate), METH_VARARGS|METH_KEYWORDS, "Run harding-gate kernel."},
    {"lattice_flip",    TRIPY_KW_METHOD(py_lattice_flip), METH_VARARGS|METH_KEYWORDS, "Run lattice-flip kernel."},
    {"run_tri",         py_run_tri,         METH_VARARGS,                "Run a .tri file."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef tripy_module = {
    PyModuleDef_HEAD_INIT, "tripy._core",
    "Trinary engine bindings (machine-code kernels).",
    -1, TriPyMethods,
    NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit__core(void) {
    PyObject *m = PyModule_Create(&tripy_module);
    if (!m) return NULL;
    TriPyError = PyErr_NewException("tripy._core.TriPyError", NULL, NULL);
    Py_XINCREF(TriPyError);
    if (PyModule_AddObject(m, "TriPyError", TriPyError) < 0) {
        Py_DECREF(TriPyError); Py_DECREF(m); return NULL;
    }
    trinary_v1_init();
    PyModule_AddStringConstant(m, "__version__", trinary_v1_version());
    return m;
}
