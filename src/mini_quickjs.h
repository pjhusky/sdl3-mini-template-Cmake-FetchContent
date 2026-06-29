#include <iostream>

#include <quickjs.h>

// 1. Include QuickJS companion utility headers for system environment functions

// Force the C++ linker to look for standard unmangled C symbols
extern "C" {
    #include <quickjs-libc.h>
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

    // 5. Clean up resource memory
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    return 0;
}
