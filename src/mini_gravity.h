#include <gravity_compiler.h>
#include <gravity_macros.h>
#include <gravity_core.h>
#include <gravity_vm.h>
#include <gravity_value.h>

#include "mini_native_functions.h"

// Gravity-specific wrappers — always defined when this header is included,
// regardless of include order with mini_pawn.h.
// nativeC_intAdd / nativeC_floatAdd are guaranteed available via MINI_NATIVE_C_FUNCS.
namespace {
    // Gravity wrapper for intAdd(a, b)
    static bool gravity_intAdd(gravity_vm *vm, gravity_value_t *args, uint16_t nargs, uint32_t rindex) {
        // args[0] is the receiver (MathBridge), args[1] is 'a', args[2] is 'b'
        if (nargs < 3) return false;
        int a = (int)VALUE_AS_INT(args[1]);
        int b = (int)VALUE_AS_INT(args[2]);
        gravity_vm_setslot(vm, VALUE_FROM_INT(nativeC_intAdd(a, b)), rindex);
        return true;
    }

    // Gravity wrapper for floatAdd(a, b)
    static bool gravity_floatAdd(gravity_vm *vm, gravity_value_t *args, uint16_t nargs, uint32_t rindex) {
        if (nargs < 3) return false;
        float a = (float)VALUE_AS_FLOAT(args[1]);
        float b = (float)VALUE_AS_FLOAT(args[2]);
        gravity_vm_setslot(vm, VALUE_FROM_FLOAT(nativeC_floatAdd(a, b)), rindex);
        return true;
    }
}


// error reporting callback called by both compiler or VM
static void report_error (gravity_vm *vm, error_type_t error_type,
                          const char *description, error_desc_t error_desc, void *xdata) {
    printf("%s\n", description);
    exit(0);
}

int mini_gravity_example_simple() {
    const static char *const pSOURCE = "func main() {var a = 10; var b=20; return a + b}";
    // configure a VM delegate
    gravity_delegate_t delegate = {.error_callback = report_error};
    
    // compile Gravity source code into bytecode
    gravity_compiler_t *compiler = gravity_compiler_create(&delegate);
    gravity_closure_t *closure = gravity_compiler_run(compiler, pSOURCE, strlen(pSOURCE), 0, true, true);
    
    // sanity check on compiled source
    if (!closure) {
        // an error occurred while compiling source code and it has already been reported by the report_error callback
        gravity_compiler_free(compiler);
        return 1;
    }
    
    // create a new VM
    gravity_vm *vm = gravity_vm_new(&delegate);
    
    // transfer objects owned by the compiler to the VM (so they can be part of the GC)
    gravity_compiler_transfer(compiler, vm);
    
    // compiler can now be freed
    gravity_compiler_free(compiler);
    
    // run main closure inside Gravity bytecode
    if (gravity_vm_runmain(vm, closure)) {
        printf("Expected value: 30\n");
        // print result (INT) 30 in this simple example
        gravity_value_t result = gravity_vm_result(vm);
        gravity_value_dump(vm, result, NULL, 0);
    }
    
    // free VM memory and core libraries
    gravity_vm_free(vm);
	gravity_core_free();
    
    return 0;    
}

#if 1
int mini_gravity_example_callNativeC() {
    static const char *const pSOURCE = "\
        extern var MathBridge; \
        func main() { \
        var a = 14; \
        var b = 22; \
        var resFloat = MathBridge.floatAdd(18.5, 44.25); \
        System.print(\"gravity>> resFloat = \" + resFloat); \
        var resInt = MathBridge.intAdd(a, b); \
        System.print(\"gravity>> resInt = \" + resInt); \
        return resInt; \
    }";

    // configure a VM delegate
    gravity_delegate_t delegate = {.error_callback = report_error};
    
    // compile Gravity source code into bytecode
    gravity_compiler_t *compiler = gravity_compiler_create(&delegate);
    gravity_closure_t *closure = gravity_compiler_run(compiler, pSOURCE, strlen(pSOURCE), 0, true, true);
    
    // sanity check on compiled source
    if (!closure) {
        gravity_compiler_free(compiler);
        return 1;
    }
    
    // create a new VM
    gravity_vm *vm = gravity_vm_new(&delegate);
    
    // 3. Register native functions inside a bridge class BEFORE transferring/running
    gravity_class_t *bridge = gravity_class_new_pair(vm, "MathBridge", NULL, 0, 0);
    gravity_class_t *metaBridge = gravity_class_get_meta(bridge);
    
    // // Bind functions as static/class methods to MathBridge
    // gravity_class_bind(metaBridge, "intAdd", NEW_CLOSURE_VALUE(gravity_intAdd));
    // gravity_class_bind(metaBridge, "floatAdd", NEW_CLOSURE_VALUE(gravity_floatAdd));
    
    // Explicitly create the native closures using the standard API functions
    // Explicitly use gravity_function_new_special
    // 1. Create closures using gravity_function_new_internal
    gravity_closure_t* closureInt = gravity_closure_new(vm, gravity_function_new_internal(vm, "intAdd", gravity_intAdd, 2));
    gravity_class_bind(metaBridge, "intAdd", VALUE_FROM_OBJECT(closureInt));

    gravity_closure_t* closureFloat = gravity_closure_new(vm, gravity_function_new_internal(vm, "floatAdd", gravity_floatAdd, 2));
    gravity_class_bind(metaBridge, "floatAdd", VALUE_FROM_OBJECT(closureFloat));
    
    // Register the class globally inside the VM
    gravity_vm_setvalue(vm, "MathBridge", VALUE_FROM_OBJECT(bridge));
    
    // transfer objects owned by the compiler to the VM
    gravity_compiler_transfer(compiler, vm);
    gravity_compiler_free(compiler);
    
    // run main closure inside Gravity bytecode
    if (gravity_vm_runmain(vm, closure)) {
        // 4. Retrieve and print result from VM
        gravity_value_t result = gravity_vm_result(vm);
        if (VALUE_ISA_INT(result)) {
            printf("Gravity execution success. Main returned: %lld\n", VALUE_AS_INT(result));
        }
        
        // free VM and resources
        gravity_vm_free(vm);
        gravity_core_free();
        return 0;
    }
    
    gravity_vm_free(vm);
    gravity_core_free();
    return 1;    
}
#endif

int mini_gravity_example() {
    const int ret_simple = mini_gravity_example_simple();

    const int ret_callNativeC = mini_gravity_example_callNativeC();

    return 0;
}