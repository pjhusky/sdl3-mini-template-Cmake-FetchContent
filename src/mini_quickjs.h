#include <iostream>

#include <quickjs.h>

// 1. Include QuickJS companion utility headers for system environment functions

// Force the C++ linker to look for standard unmangled C symbols
extern "C" {
    #include <quickjs-libc.h>
}

#include "mini_native_functions.h"



// QuickJS-specific wrappers — always defined when this header is included,
// regardless of include order with mini_sol3.h/mini_gravity.h/mini_pawn.h.
// nativeC_intAdd / nativeC_floatAdd are guaranteed available via MINI_NATIVE_C_FUNCS.
namespace {
    // QuickJS wrapper for intAdd(a, b)
    static JSValue js_intAdd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
        if (argc < 2) return JS_ThrowTypeError(ctx, "intAdd expects 2 arguments");
        int32_t a, b;
        if (JS_ToInt32(ctx, &a, argv[0])) return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &b, argv[1])) return JS_EXCEPTION;
        return JS_NewInt32(ctx, nativeC_intAdd(a, b));
    }

    // QuickJS wrapper for floatAdd(a, b)
    static JSValue js_floatAdd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
        if (argc < 2) return JS_ThrowTypeError(ctx, "floatAdd expects 2 arguments");
        double a, b;
        if (JS_ToFloat64(ctx, &a, argv[0])) return JS_EXCEPTION;
        if (JS_ToFloat64(ctx, &b, argv[1])) return JS_EXCEPTION;
        return JS_NewFloat64(ctx, nativeC_floatAdd((float)a, (float)b));
    }
}


int mini_quickjs_example() {
    // 1. Initialize the QuickJS Runtime and Context
    JSRuntime *rt = JS_NewRuntime();
    if (!rt) {
        std::cerr << "Failed to create JS runtime\n";
        return 1;
    }

    JSContext *ctx = JS_NewContext(rt);
    if (!ctx) {
        std::cerr << "Failed to create JS context\n";
        JS_FreeRuntime(rt);
        return 1;
    }

    // 2. CRITICAL: Inject std, os, and console handlers into the global sandbox
    // This connects JavaScript's console.log directly to standard native stdout (printf)
    js_std_init_handlers(rt);
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");
    js_std_add_helpers(ctx, 0, nullptr); // This explicitly adds console.log and print()

    // 2b. Register intAdd/floatAdd as global native functions callable from JS
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "intAdd", JS_NewCFunction(ctx, js_intAdd, "intAdd", 2));
    JS_SetPropertyStr(ctx, global_obj, "floatAdd", JS_NewCFunction(ctx, js_floatAdd, "floatAdd", 2));
    JS_FreeValue(ctx, global_obj);

    // 2. Run a script that prints a string using standard console.log
    std::cout << "--- Running Print Script ---\n";
    const char *print_script = "console.log('Hello quickjs-ng world!');";
    
    JSValue print_result = JS_Eval(ctx, print_script, strlen(print_script), "print_script.js", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, print_result); // Always free evaluated results to prevent leaks

    // 3. Run a script that performs math and returns the value to C++
    std::cout << "\n--- Running Math Script ---\n";
    const char *math_script = "const a = 10;\n"
                              "const b = 32;\n"
                              "a + b;"; // The last expression is returned

    JSValue math_result = JS_Eval(ctx, math_script, strlen(math_script), "math_script.js", JS_EVAL_TYPE_GLOBAL);

    // 4. Convert the JS value back into a native C++ integer
    if (JS_IsException(math_result)) {
        std::cerr << "JavaScript exception occurred!\n";
    } else {
        int32_t sum = 0;
        if (JS_ToInt32(ctx, &sum, math_result) == 0) {
            std::cout << "Result of 10 + 32 calculated by QuickJS: " << sum << "\n";
        }
    }
    JS_FreeValue(ctx, math_result);

    // 4b. Run a script that calls the native C functions from JS
    std::cout << "\n--- Running Call-Native-C Script ---\n";
    const char *native_script = "const resFloat = floatAdd(18.5, 44.25);\n"
                                "console.log(\"quick-js>> resFloat = \" + resFloat);\n"
                                "const resInt = intAdd(14, 22);\n"
                                "console.log(\"quick-js>> resInt = \" + resInt);\n"
                                "resInt;"; // The last expression is returned

    JSValue native_result = JS_Eval(ctx, native_script, strlen(native_script), "native_script.js", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(native_result)) {
        std::cerr << "JavaScript exception occurred!\n";
        js_std_dump_error(ctx);
    } else {
        int32_t resInt = 0;
        if (JS_ToInt32(ctx, &resInt, native_result) == 0) {
            std::cout << "QuickJS execution success. intAdd(14, 22) returned: " << resInt << "\n";
        }
    }
    JS_FreeValue(ctx, native_result);

    // 5. Clean up resource memory
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    return 0;
}
